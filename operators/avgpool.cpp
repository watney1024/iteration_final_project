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

std::vector<int> padding;
std::vector<int> kernel_size;
std::vector<int> stride;
std::vector<int> dilation;

double get_current_time()
{
    auto now = std::chrono::high_resolution_clock::now();
    auto usec = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch());
    return usec.count() / 1000.0;
}

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
                    int index = d * mat.channel * mat.height * mat.width + c * mat.height * mat.width + h * mat.width + w;
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

double avgp(const Mat &input, Mat &output, std::vector<int> avgp_kernel_size, std::vector<int> avgp_stride)
{
    double start = get_current_time();
    int input_h = input.height;
    int input_w = input.width;
    int out_h = output.height;
    int out_w = output.width;

    for (int d = 0; d < input.dim; ++d)
    {
        for (int c = 0; c < input.channel; ++c)
        {
            for (int oh = 0; oh < out_h; ++oh)
            {
                for (int ow = 0; ow < out_w; ++ow)
                {
                    float sum = 0.0;
                    int count = 0;
                    for (int kh = 0; kh < avgp_kernel_size[0]; ++kh)
                    {
                        for (int kw = 0; kw < avgp_kernel_size[1]; ++kw)
                        {
                            int h = oh * avgp_stride[1] + kh;
                            int w = ow * avgp_stride[0] + kw;
                            if (h < input_h && w < input_w)
                            {
                                int index = (d * input.channel * input_h * input_w) + (c * input_h * input_w) + (h * input_w) + w;
                                sum += input[index];
                                count++;
                            }
                        }
                    }
                    output[(d * output.channel * out_h * out_w) + (c * out_h * out_w) + (oh * out_w) + ow] = sum / count;
                }
            }
        }
    }
    double end = get_current_time();
    return (end - start);
}


int main()
{
    padding.assign(0, 0);     
    kernel_size.assign(2, 2); 
    stride.assign(2, 2);      

    Mat mp1_input(1, 320, 300, 300);
    Mat mp1_output(1, 320, 150, 150);

    pretensor(mp1_input);
    
    // 进行250次测试，前50次预热，后200次计算中位数和P99
    const int total_iterations = 250;
    const int warmup_iterations = 50;
    double times[total_iterations];
    
    for (int i = 0; i < total_iterations; ++i)
    {
        // 重置输出矩阵
        std::fill(mp1_output.tensor.begin(), mp1_output.tensor.end(), 0);
        times[i] = avgp(mp1_input, mp1_output, kernel_size, stride);
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
    //printMat(mp1_input);
    //printMat(mp1_output);

    return 0;
}
