#include "gauss_seidel_2d.h"
#include "gauss_seidel_2d_simd.h"
#include <iostream>

// 前置声明tiled namespace
namespace GaussSeidel2DTiled {
    void solve_4level_tiling(
        std::vector<double>& u,
        const std::vector<double>& f,
        int N,
        double h,
        int max_iter,
        double tol,
        int& iter_count,
        double& residual,
        int num_threads
    );
}
#include <fstream>
#include <iomanip>
#include <chrono>
#include <cmath>
#include <string>

using namespace std;
using namespace std::chrono;

void run_test(int N, int max_iter, double tol, int threads, const string& method, ofstream& csv_out) {
    double h = 1.0 / (N + 1);
    
    vector<double> u, f, u_exact;
    
    if (method == "Original") {
        GaussSeidel2D::init_test_problem(u, f, u_exact, N, h);
    } else if (method == "Tiled") {
        GaussSeidel2D::init_test_problem(u, f, u_exact, N, h);
    } else if (method == "SIMD") {
        GaussSeidel2DSIMD::init_test_problem(u, f, u_exact, N, h);
    }
    
    int iter_count = 0;
    double residual = 0.0;
    
    auto start = high_resolution_clock::now();
    
    if (method == "Original") {
        GaussSeidel2D::solve_parallel_redblack(u, f, N, h, max_iter, tol, iter_count, residual, threads);
    } else if (method == "Tiled") {
        GaussSeidel2DTiled::solve_4level_tiling(u, f, N, h, max_iter, tol, iter_count, residual, threads);
    } else if (method == "SIMD") {
        GaussSeidel2DSIMD::solve_parallel_redblack_simd(u, f, N, h, max_iter, tol, iter_count, residual, threads);
    }
    
    auto end = high_resolution_clock::now();
    double elapsed = duration_cast<microseconds>(end - start).count() / 1e3;
    
    // 计算误差
    double max_error = 0.0;
    for (int i = 1; i <= N; ++i) {
        for (int j = 1; j <= N; ++j) {
            int idx = i * (N + 2) + j;
            max_error = max(max_error, abs(u[idx] - u_exact[idx]));
        }
    }
    
    // 输出到控制台
    cout << method << " | N=" << N << " | Threads=" << threads 
         << " | Time=" << fixed << setprecision(2) << elapsed << "ms"
         << " | Iters=" << iter_count 
         << " | Residual=" << scientific << setprecision(2) << residual
         << " | Error=" << max_error << endl;
    
    // 输出到CSV
    csv_out << method << "," << N << "," << threads << "," 
            << fixed << setprecision(2) << elapsed << ","
            << iter_count << ","
            << scientific << setprecision(4) << residual << ","
            << max_error << endl;
}

int main() {
    const double tol = 1e-6;
    const int max_iter = 1000;
    
    // 测试配置
    vector<int> N_values = {64, 128, 256, 512};
    vector<int> thread_counts = {1, 2, 4, 8, 12, 16, 20};
    vector<string> methods = {"Original", "Tiled", "SIMD"};
    
    // 创建CSV文件
    ofstream csv_out("simd_results_2d.csv");
    csv_out << "Method,N,Threads,Time(ms),Iterations,Residual,MaxError" << endl;
    
    cout << "========== 2D Gauss-Seidel SIMD Performance Test ==========" << endl;
    cout << "tol=" << tol << ", max_iter=" << max_iter << endl << endl;
    
    for (const auto& method : methods) {
        cout << "\n======== " << method << " Method ========" << endl;
        for (int N : N_values) {
            for (int threads : thread_counts) {
                run_test(N, max_iter, tol, threads, method, csv_out);
            }
        }
    }
    
    csv_out.close();
    cout << "\n结果已保存到 simd_results_2d.csv" << endl;
    
    return 0;
}
