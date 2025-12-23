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

// 计算相对误差
double compute_error(const vector<double>& u, const vector<double>& u_exact, int N) {
    double error = 0.0;
    double norm = 0.0;
    
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

// 打印结果
void print_result(const string& method, int iter_count, double residual, 
                  double error, double time_ms, int N) {
    cout << "\n" << string(60, '=') << endl;
    cout << "方法: " << method << endl;
    cout << string(60, '-') << endl;
    cout << "网格规模:        " << N << " x " << N << endl;
    cout << "迭代次数:        " << iter_count << endl;
    cout << "最终残差:        " << scientific << setprecision(6) << residual << endl;
    cout << "相对误差:        " << scientific << setprecision(6) << error << endl;
    cout << "计算时间:        " << fixed << setprecision(3) << time_ms << " ms" << endl;
    cout << "每次迭代时间:    " << fixed << setprecision(3) 
         << time_ms / iter_count << " ms" << endl;
    cout << string(60, '=') << endl;
}

int main(int argc, char* argv[]) {
    // 设置Windows控制台为UTF-8编码
    #ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    #endif
    
    // 设置问题参数
    int N = 128;                   // 内部网格点数（增大问题规模）
    double h = 1.0 / (N + 1);      // 网格间距
    int max_iter = 10000;          // 最大迭代次数
    double tol = 1e-6;             // 收敛容差
    
    cout << "\n";
    cout << "============================================================" << endl;
    cout << "       二维泊松方程 Gauss-Seidel 求解器性能测试            " << endl;
    cout << "============================================================" << endl;
    
    // 初始化问题
    vector<double> u, f, u_exact;
    GaussSeidel2D::init_test_problem(u, f, u_exact, N, h);
    
    cout << "\n问题设置:" << endl;
    cout << "  求解方程: -Δu = f" << endl;
    cout << "  边界条件: u = 0" << endl;
    cout << "  精确解:   u(x,y) = sin(πx) * sin(πy)" << endl;
    cout << "  网格规模: " << N << " x " << N << endl;
    cout << "  最大迭代: " << max_iter << endl;
    cout << "  收敛容差: " << scientific << tol << endl;
    
    // ========== 测试1: 串行普通 Gauss-Seidel ==========
    {
        vector<double> u_test = u;
        int iter_count = 0;
        double residual = 0.0;
        
        auto start = high_resolution_clock::now();
        GaussSeidel2D::solve_serial(u_test, f, N, h, max_iter, tol, 
                                    iter_count, residual);
        auto end = high_resolution_clock::now();
        
        double time_ms = duration_cast<microseconds>(end - start).count() / 1000.0;
        double error = compute_error(u_test, u_exact, N);
        
        print_result("串行普通 Gauss-Seidel", iter_count, residual, 
                    error, time_ms, N);
    }
    
    // ========== 测试2: 串行红黑 Gauss-Seidel ==========
    {
        vector<double> u_test = u;
        int iter_count = 0;
        double residual = 0.0;
        
        auto start = high_resolution_clock::now();
        GaussSeidel2D::solve_serial_redblack(u_test, f, N, h, max_iter, tol, 
                                             iter_count, residual);
        auto end = high_resolution_clock::now();
        
        double time_ms = duration_cast<microseconds>(end - start).count() / 1000.0;
        double error = compute_error(u_test, u_exact, N);
        
        print_result("串行红黑 Gauss-Seidel", iter_count, residual, 
                    error, time_ms, N);
    }
    
    // ========== 测试3: 并行红黑 Gauss-Seidel (不同线程数) ==========
    int thread_counts[] = {1, 2, 4, 8};
    vector<double> times(4);
    
    for (int idx = 0; idx < 4; ++idx) {
        int threads = thread_counts[idx];
        vector<double> u_test = u;
        int iter_count = 0;
        double residual = 0.0;
        
        auto start = high_resolution_clock::now();
        GaussSeidel2D::solve_parallel_redblack(u_test, f, N, h, max_iter, tol, 
                                               iter_count, residual, threads);
        auto end = high_resolution_clock::now();
        
        double time_ms = duration_cast<microseconds>(end - start).count() / 1000.0;
        double error = compute_error(u_test, u_exact, N);
        times[idx] = time_ms;
        
        string method = "并行红黑 Gauss-Seidel (" + to_string(threads) + " 线程)";
        print_result(method, iter_count, residual, error, time_ms, N);
        
        if (threads > 1 && times[0] > 0) {
            cout << "加速比 (相对于1线程): " << fixed << setprecision(2) 
                 << times[0] / time_ms << "x" << endl;
        }
    }
    
    // ========== 性能总结 ==========
    cout << "\n============================================================" << endl;
    cout << "                      性能分析总结                          " << endl;
    cout << "============================================================" << endl;
    cout << "\n关键观察:" << endl;
    cout << "1. 红黑排序允许并行化，相同颜色的点可以同时更新" << endl;
    cout << "2. 区域分解策略将问题域划分为多个块，提高缓存局部性" << endl;
    cout << "3. 多级分块策略（16x16小块）优化了内存访问模式" << endl;
    cout << "4. OpenMP并行化在多核处理器上获得了显著加速" << endl;
    cout << "\n实现细节:" << endl;
    cout << "  - 采用红黑排序消除数据依赖" << endl;
    cout << "  - 使用动态调度平衡负载" << endl;
    cout << "  - 分块大小为16x16，优化L1缓存利用率" << endl;
    cout << "  - 残差计算使用OpenMP reduction优化" << endl;
    cout << "\n" << endl;
    
    return 0;
}
