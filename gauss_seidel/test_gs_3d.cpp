#include "gauss_seidel_3d.h"
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

// 打印结果
void print_result(const string& method, int iter_count, double residual, 
                  double error, double time_ms, int N) {
    cout << "\n" << string(60, '=') << endl;
    cout << "方法: " << method << endl;
    cout << string(60, '-') << endl;
    cout << "网格规模:        " << N << " x " << N << " x " << N << endl;
    cout << "迭代次数:        " << iter_count << endl;
    cout << "最终残差:        " << scientific << setprecision(6) << residual << endl;
    cout << "相对误差:        " << scientific << setprecision(6) << error << endl;
    cout << "计算时间:        " << fixed << setprecision(3) << time_ms / 1000.0 << " s" << endl;
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
    
    // 设置问题参数 - 参考 gulang2019 的设置
    int N = 512;                   // 内部网格点数 512^3
    double h = 1.0 / (N + 1);      // 网格间距
    int max_iter = 100;            // 最大迭代次数（由于规模大，用较少迭代）
    double tol = 1e-6;             // 收敛容差
    
    cout << "\n";
    cout << "============================================================" << endl;
    cout << "       三维泊松方程 Gauss-Seidel 求解器性能测试            " << endl;
    cout << "       (参考 gulang2019/optimizing-gauss-seidel-iteration)" << endl;
    cout << "============================================================" << endl;
    
    cout << "\n问题设置:" << endl;
    cout << "  求解方程: -Δu = f" << endl;
    cout << "  边界条件: u = 0" << endl;
    cout << "  精确解:   u(x,y,z) = sin(πx) * sin(πy) * sin(πz)" << endl;
    cout << "  网格规模: " << N << " x " << N << " x " << N << endl;
    cout << "  总格点数: " << (long long)N * N * N << endl;
    cout << "  最大迭代: " << max_iter << endl;
    cout << "  收敛容差: " << scientific << tol << endl;
    
    cout << "\n初始化问题..." << endl;
    vector<double> u, f, u_exact;
    auto init_start = high_resolution_clock::now();
    GaussSeidel3D::init_test_problem(u, f, u_exact, N, h);
    auto init_end = high_resolution_clock::now();
    double init_time = duration_cast<milliseconds>(init_end - init_start).count() / 1000.0;
    cout << "初始化完成，用时: " << fixed << setprecision(2) << init_time << " s" << endl;
    
    // ========== 测试1: 串行红黑 Gauss-Seidel ==========
    cout << "\n开始测试串行红黑 Gauss-Seidel..." << endl;
    {
        vector<double> u_test = u;
        int iter_count = 0;
        double residual = 0.0;
        
        auto start = high_resolution_clock::now();
        GaussSeidel3D::solve_serial_redblack(u_test, f, N, h, max_iter, tol, 
                                             iter_count, residual);
        auto end = high_resolution_clock::now();
        
        double time_ms = duration_cast<milliseconds>(end - start).count();
        double error = compute_error(u_test, u_exact, N);
        
        print_result("串行红黑 Gauss-Seidel", iter_count, residual, 
                    error, time_ms, N);
    }
    
    // ========== 测试2: 并行红黑 Gauss-Seidel (不同线程数) ==========
    int thread_counts[] = {1, 2, 4, 8};
    vector<double> times(4);
    
    for (int idx = 0; idx < 4; ++idx) {
        int threads = thread_counts[idx];
        cout << "\n开始测试并行红黑 Gauss-Seidel (" << threads << " 线程)..." << endl;
        
        vector<double> u_test = u;
        int iter_count = 0;
        double residual = 0.0;
        
        auto start = high_resolution_clock::now();
        GaussSeidel3D::solve_parallel_redblack(u_test, f, N, h, max_iter, tol, 
                                               iter_count, residual, threads);
        auto end = high_resolution_clock::now();
        
        double time_ms = duration_cast<milliseconds>(end - start).count();
        double error = compute_error(u_test, u_exact, N);
        times[idx] = time_ms;
        
        string method = "并行红黑 Gauss-Seidel (" + to_string(threads) + " 线程)";
        print_result(method, iter_count, residual, error, time_ms, N);
        
        if (threads > 1 && times[0] > 0) {
            cout << "加速比 (相对于1线程): " << fixed << setprecision(2) 
                 << times[0] / time_ms << "x" << endl;
            
            // 计算并行效率
            double efficiency = (times[0] / time_ms) / threads * 100.0;
            cout << "并行效率:             " << fixed << setprecision(1) 
                 << efficiency << "%" << endl;
        }
    }
    
    // ========== 性能总结 ==========
    cout << "\n============================================================" << endl;
    cout << "                      性能分析总结                          " << endl;
    cout << "============================================================" << endl;
    cout << "\n关键观察:" << endl;
    cout << "1. 三维问题规模大(512^3)，并行化效果更明显" << endl;
    cout << "2. 红黑排序消除了数据依赖，实现真正的并行" << endl;
    cout << "3. 区域分解和分块策略提高了缓存局部性" << endl;
    cout << "4. collapse(3) 指令增加了并行粒度" << endl;
    cout << "\n实现细节:" << endl;
    cout << "  - 采用红黑排序消除数据依赖" << endl;
    cout << "  - 使用静态调度减少开销" << endl;
    cout << "  - 分块大小为64x64x64，优化缓存利用" << endl;
    cout << "  - 每10次迭代检查一次收敛性" << endl;
    cout << "  - 残差计算使用OpenMP reduction优化" << endl;
    cout << "\n" << endl;
    
    return 0;
}
