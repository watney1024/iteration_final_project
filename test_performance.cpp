#include <iostream>
#include <vector>
#include <chrono>
#include <iomanip>
#include <random>
#include <omp.h>
#include "red_black_gauss_seidel.h"

#ifdef _WIN32
#include <windows.h>
#endif

// 生成对角占优矩阵以确保收敛
void generate_diagonally_dominant_matrix(
    std::vector<std::vector<double>>& A,
    std::vector<double>& b,
    int n
) {
    std::random_device rd;
    std::mt19937 gen(42); // 固定种子以保证可重复性
    std::uniform_real_distribution<> dis(0.0, 1.0);
    
    // 生成随机矩阵
    for (int i = 0; i < n; i++) {
        double row_sum = 0.0;
        for (int j = 0; j < n; j++) {
            if (i != j) {
                A[i][j] = dis(gen);
                row_sum += std::abs(A[i][j]);
            }
        }
        // 使对角元素占优
        A[i][i] = row_sum + 1.0 + dis(gen);
        b[i] = dis(gen) * 10.0;
    }
}

// 生成简单的测试矩阵
void generate_simple_test_matrix(
    std::vector<std::vector<double>>& A,
    std::vector<double>& b,
    int n
) {
    // 生成一个简单的对角占优矩阵
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            if (i == j) {
                A[i][j] = 10.0;
            } else if (std::abs(i - j) == 1) {
                A[i][j] = -1.0;
            } else {
                A[i][j] = 0.0;
            }
        }
        b[i] = 1.0;
    }
}

// 性能测试函数
void performance_test(int n, int max_iterations, double tolerance, 
                     const std::vector<int>& thread_counts) {
    std::cout << "\n========================================" << std::endl;
    std::cout << "Performance Test" << std::endl;
    std::cout << "Matrix size: " << n << "x" << n << std::endl;
    std::cout << "Max iterations: " << max_iterations << std::endl;
    std::cout << "Tolerance: " << tolerance << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    // 生成测试矩阵
    std::vector<std::vector<double>> A(n, std::vector<double>(n));
    std::vector<double> b(n);
    generate_diagonally_dominant_matrix(A, b, n);
    
    // 存储串行执行时间作为基准
    double serial_time = 0.0;
    
    // 串行版本测试
    {
        std::vector<double> x(n, 0.0);
        
        auto start = std::chrono::high_resolution_clock::now();
        RedBlackGaussSeidel::solve_serial(A, b, x, max_iterations, tolerance);
        auto end = std::chrono::high_resolution_clock::now();
        
        serial_time = std::chrono::duration<double>(end - start).count();
        double residual = RedBlackGaussSeidel::compute_residual(A, b, x);
        
        std::cout << "Serial Version:" << std::endl;
        std::cout << "  Time: " << std::fixed << std::setprecision(6) 
                 << serial_time << " seconds" << std::endl;
        std::cout << "  Final residual: " << residual << std::endl;
        std::cout << std::endl;
    }
    
    // 并行版本测试（不同线程数）
    std::cout << "Parallel Version Results:" << std::endl;
    std::cout << std::setw(10) << "Threads" 
             << std::setw(15) << "Time (s)" 
             << std::setw(15) << "Speedup"
             << std::setw(20) << "Residual" << std::endl;
    std::cout << std::string(60, '-') << std::endl;
    
    for (int num_threads : thread_counts) {
        std::vector<double> x(n, 0.0);
        
        auto start = std::chrono::high_resolution_clock::now();
        RedBlackGaussSeidel::solve_parallel(A, b, x, max_iterations, tolerance, num_threads);
        auto end = std::chrono::high_resolution_clock::now();
        
        double parallel_time = std::chrono::duration<double>(end - start).count();
        double speedup = serial_time / parallel_time;
        double residual = RedBlackGaussSeidel::compute_residual(A, b, x);
        
        std::cout << std::setw(10) << num_threads 
                 << std::setw(15) << std::fixed << std::setprecision(6) << parallel_time
                 << std::setw(15) << std::fixed << std::setprecision(3) << speedup
                 << std::setw(20) << std::scientific << std::setprecision(4) << residual
                 << std::endl;
    }
    std::cout << std::endl;
}

int main(int argc, char* argv[]) {
    // 设置控制台为UTF-8编码（Windows）
    #ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    #endif
    
    // 获取系统最大线程数
    int max_threads = omp_get_max_threads();
    std::cout << "System maximum threads: " << max_threads << std::endl;
    
    // 定义要测试的线程数
    std::vector<int> thread_counts = {1, 2, 4, 8};
    
    // 过滤掉超过系统最大线程数的配置
    std::vector<int> valid_thread_counts;
    for (int t : thread_counts) {
        if (t <= max_threads) {
            valid_thread_counts.push_back(t);
        }
    }
    
    // 测试不同规模的矩阵
    std::vector<int> matrix_sizes = {100, 500, 1000};
    int max_iterations = 1000;
    double tolerance = 1e-6;
    
    for (int n : matrix_sizes) {
        performance_test(n, max_iterations, tolerance, valid_thread_counts);
    }
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "算法说明:" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "红黑排序 Gauss-Seidel 算法：" << std::endl;
    std::cout << "1. 将所有网格点分为红点和黑点（棋盘模式）" << std::endl;
    std::cout << "2. 先更新所有红点（红点之间相互独立，可并行）" << std::endl;
    std::cout << "3. 再更新所有黑点（黑点之间相互独立，可并行）" << std::endl;
    std::cout << "4. 重复步骤2-3直到收敛" << std::endl;
    std::cout << "\n优点：" << std::endl;
    std::cout << "- 红点和黑点内部可以完全并行计算" << std::endl;
    std::cout << "- 保持了 Gauss-Seidel 方法的快速收敛特性" << std::endl;
    std::cout << "- 适合 OpenMP 共享内存并行" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    return 0;
}
