// cpu_flops_test.cpp
// 编译：g++ -O3 -march=native -fopenmp -o cpu_test cpu_flops_test.cpp
// 运行：./cpu_test [线程数]

#include <omp.h>
#include <chrono>
#include <cstdio>
#include <string>

// CPU浮点运算能力测试函数
// 通过执行大量FMA(融合乘加)指令测量峰值GFLOPS
void cpu_flops_test(int n_threads) {
    const long long N = 2000000000LL; // 20亿次FMA操作
    float a = 1.5f, b = 2.3f;
    
    omp_set_num_threads(n_threads);
    auto start = std::chrono::high_resolution_clock::now();
    
    // 每个线程使用私有寄存器变量，避免false sharing
    #pragma omp parallel
    {
        float c0 = 0.0f, c1 = 0.0f, c2 = 0.0f, c3 = 0.0f;
        float a_reg = a, b_reg = b;
        
        #pragma omp for schedule(static)
        for (long long i = 0; i < N; ++i) {
            // 4个独立依赖链，让CPU流水线满载
            c0 += a_reg * b_reg;
            c1 += a_reg * b_reg;
            c2 += a_reg * b_reg;
            c3 += a_reg * b_reg;
        }
        
        // 防止编译器优化掉计算
        if (c0 + c1 + c2 + c3 == 0.0f) {
            printf("Never happens\n");
        }
    }
    
    double ms = std::chrono::duration<double, std::milli>(
        std::chrono::high_resolution_clock::now() - start).count();
    
    // 正确计算：N次操作 × 2 FLOPs/次 = 总FLOPs
    double gflops = (N * 2.0) / (ms * 1e6);
    printf("线程数: %2d | 耗时: %.2f ms | GFLOPS: %.2f\n", n_threads, ms, gflops);
}

int main(int argc, char* argv[]) {
    printf("=== CPU浮点运算能力测试 ===\n");
    printf("测试原理：执行20亿次FMA融合乘加指令\n");
    printf("峰值参考：10核3.5GHz AVX2+FMA ≈ 560 GFLOPS\n\n");
    
    if (argc > 1) {
        int threads = std::stoi(argv[1]);
        if (threads <= 0 || threads > 64) {
            printf("错误：线程数应在1-64之间\n");
            return 1;
        }
        cpu_flops_test(threads);
    } else {
        printf("未指定线程数，测试1,2,4,8,10,16,20线程\n\n");
        int test_threads[] = {1, 2, 4, 8, 10, 16, 20};
        int num_tests = sizeof(test_threads) / sizeof(test_threads[0]);
        
        for (int i = 0; i < num_tests; ++i) {
            cpu_flops_test(test_threads[i]);
        }
    }
    
    return 0;
}