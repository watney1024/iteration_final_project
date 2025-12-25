#include "gauss_seidel_3d.h"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <cmath>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;
using namespace std::chrono;

// 计算相对误差
double compute_error(const vector<double>& u, const vector<double>& u_exact, int N) {
    double error = 0.0;
    double norm = 0.0;
    
    #pragma omp parallel for reduction(+:error,norm) collapse(3)
    for (int i = 1; i <= N; ++i) {
        for (int j = 1; j <= N; ++j) {
            for (int k = 1; k <= N; ++k) {
                int idx = i * (N + 2) * (N + 2) + j * (N + 2) + k;
                double diff = u[idx] - u_exact[idx];
                error += diff * diff;
                norm += u_exact[idx] * u_exact[idx];
            }
        }
    }
    
    return sqrt(error / norm);
}

int main(int argc, char* argv[]) {
    #ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    #endif
    
    // 从命令行获取参数
    if (argc < 3) {
        cerr << "用法: " << argv[0] << " <网格尺寸> <线程数>" << endl;
        cerr << "示例: " << argv[0] << " 128 4" << endl;
        return 1;
    }
    
    int N = atoi(argv[1]);          // 内部网格点数
    int num_threads = atoi(argv[2]); // 线程数
    
    if (N <= 0 || N > 1024) {
        cerr << "错误: 网格尺寸必须在 1 到 1024 之间" << endl;
        return 1;
    }
    
    if (num_threads <= 0 || num_threads > 128) {
        cerr << "错误: 线程数必须在 1 到 128 之间" << endl;
        return 1;
    }
    
    double h = 1.0 / (N + 1);      // 网格间距
    int max_iter = 100;            // 最大迭代次数（3D问题规模大，迭代次数少）
    double tol = 1e-6;             // 收敛容差
    
    // 计算内存需求
    long long total_points = (long long)(N + 2) * (N + 2) * (N + 2);
    long long inner_points = (long long)N * N * N;
    double memory_gb = (total_points * 2 + inner_points) * sizeof(double) / (1024.0 * 1024.0 * 1024.0);
    
    cout << "\n============================================================" << endl;
    cout << "三维泊松方程 Gauss-Seidel 并行红黑求解器" << endl;
    cout << "============================================================" << endl;
    cout << "网格尺寸:    " << N << " x " << N << " x " << N << endl;
    cout << "总格点数:    " << inner_points << endl;
    cout << "线程数:      " << num_threads << endl;
    cout << "最大迭代:    " << max_iter << endl;
    cout << "收敛容差:    " << scientific << tol << endl;
    cout << "估计内存:    " << fixed << setprecision(2) << memory_gb << " GB" << endl;
    cout << "============================================================" << endl;
    
    // 初始化问题
    cout << "\n初始化问题..." << flush;
    vector<double> u, f, u_exact;
    auto init_start = high_resolution_clock::now();
    try {
        GaussSeidel3D::init_test_problem(u, f, u_exact, N, h);
    } catch (const bad_alloc& e) {
        cerr << "\n错误: 内存分配失败！需要约 " << memory_gb << " GB 内存" << endl;
        return 1;
    }
    auto init_end = high_resolution_clock::now();
    double init_s = duration_cast<milliseconds>(init_end - init_start).count() / 1000.0;
    cout << " 完成 (" << fixed << setprecision(2) << init_s << " s)" << endl;
    
    // 运行并行红黑 Gauss-Seidel
    cout << "\n开始求解..." << flush;
    int iter_count = 0;
    double residual = 0.0;
    
    auto start = high_resolution_clock::now();
    GaussSeidel3D::solve_parallel_redblack(u, f, N, h, max_iter, tol, 
                                           iter_count, residual, num_threads);
    auto end = high_resolution_clock::now();
    
    double time_ms = duration_cast<microseconds>(end - start).count() / 1000.0;
    cout << " 完成" << endl;
    
    // 计算误差
    cout << "\n计算误差..." << flush;
    auto err_start = high_resolution_clock::now();
    double error = compute_error(u, u_exact, N);
    auto err_end = high_resolution_clock::now();
    double err_ms = duration_cast<microseconds>(err_end - err_start).count() / 1000.0;
    cout << " 完成 (" << fixed << setprecision(2) << err_ms << " ms)" << endl;
    
    // Output results
    cout << "\n" << string(60, '=') << endl;
    cout << "Results" << endl;
    cout << string(60, '-') << endl;
    cout << "Iterations:      " << iter_count << endl;
    cout << "Final residual:  " << scientific << setprecision(6) << residual << endl;
    cout << "Relative error:  " << scientific << setprecision(6) << error << endl;
    cout << "Total time:      " << fixed << setprecision(3) << time_ms << " ms" << endl;
    cout << "Time per iter:   " << fixed << setprecision(3) 
         << time_ms / iter_count << " ms" << endl;
    
    // Calculate performance metrics
    double gflops_per_iter = (inner_points * 13.0) / 1e9;
    double gflops = gflops_per_iter * iter_count / (time_ms / 1000.0);
    cout << "Performance:     " << fixed << setprecision(2) << gflops << " GFLOPS" << endl;
    
    cout << string(60, '=') << endl;
    
    // 如果是多线程，计算加速比（需要先运行单线程基准）
    if (num_threads > 1) {
        cout << "\n注意: 加速比需要与单线程版本对比计算" << endl;
    }
    
    cout << endl;
    
    return 0;
}
