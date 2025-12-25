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
#include <malloc.h>
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

Mat padd(const Mat input,int this_padding)
{
    if(this_padding == 0)
        return input;
    int new_height = input.height + 2*this_padding;
    int new_width = input.width + 2*this_padding;
    Mat new_mat(input.dim,input.channel,new_height,new_width);
    std::fill(new_mat.tensor.begin(), new_mat.tensor.end(), 0);
    for (int c = 0; c < input.channel; ++c)
    {
        for (int h = 0; h < input.height; ++h)
        {
            for (int w = 0; w < input.width; ++w)
            {
                new_mat[c * new_height * new_width + (h + this_padding) * new_width + w + this_padding] = input[c * input.height * input.width + h * input.width + w];
            }
        }
    }
    return new_mat;
}

double conv2d(const Mat &input, Mat &output, const std::vector<float> &weight, const std::vector<float> &bias,
              const std::vector<int> &conv_kernel_size, const std::vector<int> &conv_stride, int conv_padding)
{
    double start = get_current_time();
    int conv_kernel_max = conv_kernel_size[0] * conv_kernel_size[1];
    Mat padded_mat = padd(input, conv_padding);
    
    // 预计算偏移量数组
    int dx[25];
    memset(dx, 0, sizeof dx);
    if (conv_kernel_max == 9)
    {
        dx[0] = 0;
        dx[1] = 1;
        dx[2] = 2;
        dx[3] = padded_mat.width;
        dx[4] = padded_mat.width + 1;
        dx[5] = padded_mat.width + 2;
        dx[6] = 2 * padded_mat.width;
        dx[7] = 2 * padded_mat.width + 1;
        dx[8] = 2 * padded_mat.width + 2;
    }
    if (conv_kernel_max == 25)
    {
        dx[0] = 0;
        dx[1] = 1;
        dx[2] = 2;
        dx[3] = 3;
        dx[4] = 4;
        dx[5] = padded_mat.width;
        dx[6] = padded_mat.width + 1;
        dx[7] = padded_mat.width + 2;
        dx[8] = padded_mat.width + 3;
        dx[9] = padded_mat.width + 4;
        dx[10] = 2 * padded_mat.width;
        dx[11] = 2 * padded_mat.width + 1;
        dx[12] = 2 * padded_mat.width + 2;
        dx[13] = 2 * padded_mat.width + 3;
        dx[14] = 2 * padded_mat.width + 4;
        dx[15] = 3 * padded_mat.width;
        dx[16] = 3 * padded_mat.width + 1;
        dx[17] = 3 * padded_mat.width + 2;
        dx[18] = 3 * padded_mat.width + 3;
        dx[19] = 3 * padded_mat.width + 4;
        dx[20] = 4 * padded_mat.width;
        dx[21] = 4 * padded_mat.width + 1;
        dx[22] = 4 * padded_mat.width + 2;
        dx[23] = 4 * padded_mat.width + 3;
        dx[24] = 4 * padded_mat.width + 4;
    }
    
    int output_h = output.height;
    int output_w = output.width;
    int padded_h = padded_mat.height;
    int padded_w = padded_mat.width;
    int padded_hw = padded_h * padded_w;
    int output_hw = output_h * output_w;
    
    // 并行化移到最外层：每个线程处理一个输出通道
    #pragma omp parallel for
    for (int oc = 0; oc < output.channel; ++oc)
    {
        // 每个输出通道的起始位置
        float* output_channel_ptr = &output.tensor[oc * output_hw];
        
        // 遍历输入通道
        //#pragma omp parallel  for
        for (int ic = 0; ic < padded_mat.channel; ++ic)
        {
            // 当前输入通道和权重的起始位置
            const float* input_channel_ptr = &padded_mat.tensor[ic * padded_hw];
            const float* weight_ptr = &weight[(oc * padded_mat.channel + ic) * conv_kernel_max];
            
            // 遍历输出的每个位置（优化访存模式：先h后w，保证连续访问）
            //#pragma omp parallel  for
            for (int h = 0; h < output_h; h += conv_stride[0])
            {
                //#pragma omp parallel  for
                for (int w = 0; w < output_w; w += conv_stride[1])
                {
                    // 计算输入起始位置
                    int input_base_idx = h * padded_w + w;
                    
                    // 使用局部变量累加，减少对output的写入
                    float sum = 0.0f;
                    
                    // 卷积核计算（展开小循环提高效率）
                    for (int k = 0; k < conv_kernel_max; ++k)
                    {
                        sum += input_channel_ptr[input_base_idx + dx[k]] * weight_ptr[k];
                    }
                    
                    // 累加到输出（而非直接赋值）
                    output_channel_ptr[h * output_w + w] += sum;
                }
            }
        }
        
        // 添加bias（在通道循环结束后统一添加）
        float bias_value = bias[oc];
        //#pragma omp parallel for
        for (int i = 0; i < output_hw; ++i)
        {
            output_channel_ptr[i] += bias_value;
        }
    }
    
    double end = get_current_time();
    return (end - start);
}




int main(int argc, char* argv[])
{
    // Memory optimization: reduce page faults and memory fragmentation
    // Note: mallopt may not be available on all Windows/MinGW platforms
    #if !defined(_WIN32) && defined(M_MMAP_MAX) && defined(M_TRIM_THRESHOLD)
    mallopt(M_MMAP_MAX, 0);           // Disable mmap for malloc
    mallopt(M_TRIM_THRESHOLD, -1);    // Disable memory trimming
    //std::cout << "Memory optimization: Enabled (mallopt)" << std::endl;
    //#else
    //std::cout << "Memory optimization: Skipped (mallopt not available on this platform)" << std::endl;
    #endif
    
    // Get thread count from command line argument, default is 20
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
    std::cout << "Using " << num_threads << " threads (Memory Optimized)" << std::endl;
    
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
