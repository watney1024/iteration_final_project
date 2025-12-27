#include <cmath>
#include <fstream>
#include <iostream>
#include <vector>
#include <chrono>
#include <time.h> 
#include <random>
#include <cstring>
#include <algorithm>
#include <omp.h>

#if defined(_WIN32)
#define PATH_SEPARATOR "\\\\"
#else
#define PATH_SEPARATOR "/"
#endif

#include <vector>

struct Mat
{
public:
    std::vector<float> tensor;

    int dim;
    int channel;
    int height;
    int width;

    Mat() : dim(1), channel(3), height(150), width(150) {
        tensor.resize(dim * channel * height * width);
    }

    // 多态构造函数
    Mat(int d, int c, int h, int w) : dim(d), channel(c), height(h), width(w) {
        tensor.resize(d * c * h * w);
    }

    float& operator[](size_t index)
    {
        return tensor[index];
    }

    const float& operator[](size_t index) const
    {
        return tensor[index];
    }
};



bool readBinaryFile(const std::string& filepath, std::vector<float>& buffer)
{
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open())
    {
        std::cerr << "Failed to open file: " << filepath << std::endl;
        return false;
    }

    file.seekg(0, std::ios::end);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    size_t numFloats = size / sizeof(float);
    buffer.resize(numFloats);
    if (file.read(reinterpret_cast<char*>(buffer.data()), size))
    {
        return true;
    }
    else
    {
        std::cerr << "Failed to read file: " << filepath << std::endl;
        return false;
    }
}

Mat conv2_input(1, 3, 150, 150); 
Mat conv2_output(1, 32, 150, 150);


std::vector<int> conv_kernel_size = { 5,5 };
std::vector<int> conv_stride = { 1,1 };
int padding = 2;

void pretensor( Mat& input)
{
    for (int i = 0; i < input.channel; ++i)
    {
        for (int j = 0; j < input.height; ++j)
        {
            for (int k = 0; k < input.width; ++k)
            {
                int index = i * input.height * input.width + j * input.width + k;
                float value = std::sin(static_cast<float>(index));
                input[index] = value;
            }
        }
    }
}

void printMat(Mat& mat)
{
    for (int d = 0; d < mat.dim; ++d)
    {
        for (int c = 0; c < mat.channel; ++c)
        {
            for (int h = 0; h < mat.height; ++h)
            {
                for (int w = 0; w < mat.width; ++w)
                {
                    int index =
                        d * mat.channel * mat.height * mat.width + c * mat.height * mat.width + h * mat.width + w;
                    printf("%.5lf ", mat[index]);
                }
                std::puts("");
            }
            std::puts("");
        }

        std::puts("");
    }
    std::puts("");
}

double get_current_time()
{
    auto now = std::chrono::high_resolution_clock::now();
    auto usec = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch());
    return usec.count() / 1000.0;
}

// 优化：并行化的padding函数
Mat padd(const Mat input, int this_padding)
{
    if(this_padding == 0)
        return input;
    int new_height = input.height + 2*this_padding;
    int new_width = input.width + 2*this_padding;
    Mat new_mat(input.dim, input.channel, new_height, new_width);
    
    // 先将整个矩阵填充为0（可以并行）
    std::fill(new_mat.tensor.begin(), new_mat.tensor.end(), 0);
    
    // 并行复制数据：按通道并行
    #pragma omp parallel for
    for (int c = 0; c < input.channel; ++c)
    {
        int src_channel_offset = c * input.height * input.width;
        int dst_channel_offset = c * new_height * new_width;
        
        for (int h = 0; h < input.height; ++h)
        {
            // 使用memcpy批量复制每一行，比逐个元素复制快
            int src_row_offset = src_channel_offset + h * input.width;
            int dst_row_offset = dst_channel_offset + (h + this_padding) * new_width + this_padding;
            memcpy(&new_mat.tensor[dst_row_offset], 
                   &input.tensor[src_row_offset], 
                   input.width * sizeof(float));
        }
    }
    return new_mat;
}

double conv2d(const Mat &input, Mat &output, const std::vector<float> &weight, const std::vector<float> &bias,
              const std::vector<int> &conv_kernel_size, const std::vector<int> &conv_stride, int conv_padding)
{
    double start = get_current_time();
    
    // 并行化的padding
    Mat padded_mat = padd(input, conv_padding);
    
    int out_h = output.height;
    int out_w = output.width;
    int in_h = padded_mat.height;
    int in_w = padded_mat.width;
    int kernel_h = conv_kernel_size[0];
    int kernel_w = conv_kernel_size[1];
    int stride_h = conv_stride[0];
    int stride_w = conv_stride[1];
    int channel_out = output.channel;
    int channel_in = padded_mat.channel;
    int kernel_max = kernel_h * kernel_w;
    
    // 二维空间并行：任务数 = 150 × 150 = 22,500
    // 每个线程处理一个输出位置的所有通道，避免false sharing
    #pragma omp parallel for collapse(2) schedule(static)
    for (int oh = 0; oh < out_h; ++oh) {
        for (int ow = 0; ow < out_w; ++ow) {
            int h_start = oh * stride_h;
            int w_start = ow * stride_w;
            
            // 每个输出点独立计算所有输出通道
            for (int oc = 0; oc < channel_out; ++oc) {
                float sum = 0.0f;
                
                // 遍历所有输入通道
                for (int ic = 0; ic < channel_in; ++ic) {
                    const float* input_ptr = &padded_mat.tensor[
                        ic * in_h * in_w + h_start * in_w + w_start];
                    const float* weight_ptr = &weight[oc * channel_in * kernel_max + ic * kernel_max];
                    
                    // 5×5卷积核完全手动展开（25项）
                    sum += weight_ptr[0] * input_ptr[0];
                    sum += weight_ptr[1] * input_ptr[1];
                    sum += weight_ptr[2] * input_ptr[2];
                    sum += weight_ptr[3] * input_ptr[3];
                    sum += weight_ptr[4] * input_ptr[4];
                    
                    sum += weight_ptr[5] * input_ptr[in_w];
                    sum += weight_ptr[6] * input_ptr[in_w + 1];
                    sum += weight_ptr[7] * input_ptr[in_w + 2];
                    sum += weight_ptr[8] * input_ptr[in_w + 3];
                    sum += weight_ptr[9] * input_ptr[in_w + 4];
                    
                    sum += weight_ptr[10] * input_ptr[2 * in_w];
                    sum += weight_ptr[11] * input_ptr[2 * in_w + 1];
                    sum += weight_ptr[12] * input_ptr[2 * in_w + 2];
                    sum += weight_ptr[13] * input_ptr[2 * in_w + 3];
                    sum += weight_ptr[14] * input_ptr[2 * in_w + 4];
                    
                    sum += weight_ptr[15] * input_ptr[3 * in_w];
                    sum += weight_ptr[16] * input_ptr[3 * in_w + 1];
                    sum += weight_ptr[17] * input_ptr[3 * in_w + 2];
                    sum += weight_ptr[18] * input_ptr[3 * in_w + 3];
                    sum += weight_ptr[19] * input_ptr[3 * in_w + 4];
                    
                    sum += weight_ptr[20] * input_ptr[4 * in_w];
                    sum += weight_ptr[21] * input_ptr[4 * in_w + 1];
                    sum += weight_ptr[22] * input_ptr[4 * in_w + 2];
                    sum += weight_ptr[23] * input_ptr[4 * in_w + 3];
                    sum += weight_ptr[24] * input_ptr[4 * in_w + 4];
                }
                
                // 优化：直接加bias再写入，避免后续额外遍历
                output.tensor[oc * out_h * out_w + oh * out_w + ow] = sum + bias[oc];
            }
        }
    }
    
    double end = get_current_time();
    return (end - start);
}




int main(int argc, char* argv[])
{
    // 从命令行参数获取线程数，默认为20
    int num_threads = 20;
    if (argc > 1)
    {
        num_threads = std::atoi(argv[1]);
        if (num_threads <= 0)
        {
            std::cerr << "Invalid thread count. Using default: 20" << std::endl;
            num_threads = 20;
        }
    }
    omp_set_num_threads(num_threads);
    //std::cout << "Using " << num_threads << " threads" << std::endl;
    //std::cout << "2D spatial parallelism + Optimized padding + Fused bias" << std::endl;
    
    std::vector<float> conv2_weight(32 * 3 * 5 * 5);
    std::vector<float> conv2_bias(32);
    std::string conv2_weight_path = ".\\src" PATH_SEPARATOR "conv1.weight.bin";
    std::string conv2_bias_path = ".\\src" PATH_SEPARATOR "conv1.bias.bin";
    readBinaryFile(conv2_weight_path, conv2_weight);
    readBinaryFile(conv2_bias_path, conv2_bias);
    pretensor(conv2_input);
    
    // 进行250次测试，前50次预热，后200次计算中位数和P99
    const int total_iterations = 250;
    const int warmup_iterations = 50;
    double times[total_iterations];
    
    for (int i = 0; i < total_iterations; ++i)
    {
        // 重置输出矩阵
        std::fill(conv2_output.tensor.begin(), conv2_output.tensor.end(), 0);
        times[i] = conv2d(conv2_input, conv2_output, conv2_weight, conv2_bias, conv_kernel_size, conv_stride, padding);
    }
    
    // 提取后200次的时间数据并排序
    const int valid_count = total_iterations - warmup_iterations;
    std::vector<double> valid_times(valid_count);
    for (int i = 0; i < valid_count; ++i)
    {
        valid_times[i] = times[warmup_iterations + i];
    }
    std::sort(valid_times.begin(), valid_times.end());
    
    // 计算中位数（第50百分位）
    double median;
    if (valid_count % 2 == 0)
    {
        median = (valid_times[valid_count / 2 - 1] + valid_times[valid_count / 2]) / 2.0;
    }
    else
    {
        median = valid_times[valid_count / 2];
    }
    
    // 计算P99（第99百分位）
    int p99_index = (int)(valid_count * 0.99) - 1;
    if (p99_index < 0) p99_index = 0;
    if (p99_index >= valid_count) p99_index = valid_count - 1;
    double p99 = valid_times[p99_index];
    
    std::cout << "Median time (after warmup): " << median << " ms" << std::endl;
    std::cout << "P99 time (after warmup): " << p99 << " ms" << std::endl;
    //printMat(conv2_output);
    return 0;
}
