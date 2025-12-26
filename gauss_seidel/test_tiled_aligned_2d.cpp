#include "gauss_seidel_2d.h"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <cmath>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;
using namespace std::chrono;

// 声明tiled版本
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

// 声明tiled+aligned版本
namespace GaussSeidel2DTiledAligned {
    void solve_4level_tiling_aligned(
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

// 计算相对误差
double compute_error(const vector<double>& u, const vector<double>& u_exact, int N) {
    double error = 0.0;
    double norm = 0.0;
    
    #pragma omp parallel for reduction(+:error,norm) collapse(2)
    for (int i = 1; i <= N; ++i) {
        for (int j = 1; j <= N; ++j) {
            int idx = i * (N + 2) + j;
            double diff = u[idx] - u_exact[idx];
            error += diff * diff;
            norm += u_exact[idx] * u_exact[idx];
        }
    }
    
    return sqrt(error / norm);
}

int main(int argc, char* argv[]) {
    // 设置Windows控制台为UTF-8编码
    #ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    #endif
    
    // 解析命令行参数
    if (argc < 3) {
        cout << "用法: " << argv[0] << " <N> <线程数>" << endl;
        return 1;
    }
    
    int N = atoi(argv[1]);
    int num_threads = atoi(argv[2]);
    double h = 1.0 / (N + 1);
    int max_iter = 1000;
    double tol = 1e-6;
    
    cout << "\n======================================================================" << endl;
    cout << "2D Poisson - Memory Alignment Optimization Test" << endl;
    cout << "======================================================================" << endl;
    cout << "Grid Size: " << N << " x " << N << endl;
    cout << "Threads:   " << num_threads << endl;
    cout << endl;
    
    // Initialize problem
    vector<double> u, f, u_exact;
    GaussSeidel2D::init_test_problem(u, f, u_exact, N, h);
    
    cout << "Method            | Iters    | Time(ms)  | Error      | Speedup" << endl;
    cout << "----------------------------------------------------------------------" << endl;
    
    // Test original implementation
    double time_original = 0.0;
    {
        vector<double> u_test = u;
        int iter_count = 0;
        double residual = 0.0;
        
        auto start = high_resolution_clock::now();
        GaussSeidel2D::solve_parallel_redblack(u_test, f, N, h, max_iter, tol, 
                                               iter_count, residual, num_threads);
        auto end = high_resolution_clock::now();
        
        time_original = duration_cast<microseconds>(end - start).count() / 1e3;
        double error = compute_error(u_test, u_exact, N);
        
        cout << left << setw(18) << "Original"
             << "| " << setw(9) << iter_count
             << "| " << setw(10) << fixed << setprecision(2) << time_original
             << "| " << scientific << setprecision(2) << error
             << " | " << fixed << setprecision(2) << "1.00x" << endl;
    }
    
    // Test 2-layer Tiling optimization
    double time_tiled = 0.0;
    {
        vector<double> u_test = u;
        int iter_count = 0;
        double residual = 0.0;
        
        auto start = high_resolution_clock::now();
        GaussSeidel2DTiled::solve_4level_tiling(u_test, f, N, h, max_iter, tol, 
                                                iter_count, residual, num_threads);
        auto end = high_resolution_clock::now();
        
        time_tiled = duration_cast<microseconds>(end - start).count() / 1e3;
        double error = compute_error(u_test, u_exact, N);
        double speedup = time_original / time_tiled;
        
        cout << left << setw(18) << "Tiled"
             << "| " << setw(9) << iter_count
             << "| " << setw(10) << fixed << setprecision(2) << time_tiled
             << "| " << scientific << setprecision(2) << error
             << " | " << fixed << setprecision(2) << speedup << "x" << endl;
    }
    
    // Test Tiled + Memory Alignment optimization
    {
        vector<double> u_test = u;
        int iter_count = 0;
        double residual = 0.0;
        
        auto start = high_resolution_clock::now();
        GaussSeidel2DTiledAligned::solve_4level_tiling_aligned(u_test, f, N, h, max_iter, tol, 
                                                               iter_count, residual, num_threads);
        auto end = high_resolution_clock::now();
        
        double time_aligned = duration_cast<microseconds>(end - start).count() / 1e3;
        double error = compute_error(u_test, u_exact, N);
        double speedup = time_original / time_aligned;
        
        cout << left << setw(18) << "Tiled+Aligned"
             << "| " << setw(9) << iter_count
             << "| " << setw(10) << fixed << setprecision(2) << time_aligned
             << "| " << scientific << setprecision(2) << error
             << " | " << fixed << setprecision(2) << speedup << "x" << endl;
    }
    
    cout << "======================================================================" << endl;
    
    return 0;
}
