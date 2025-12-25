#include "gauss_seidel_2d.h"
#include "gauss_seidel_2d_optimized.h"
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

void print_result(const string& method, int iter_count, double residual, 
                  double error, double time_ms, int N, double baseline_ms = 0) {
    cout << "\n" << string(70, '=') << endl;
    cout << "方法: " << method << endl;
    cout << string(70, '-') << endl;
    cout << "网格规模:        " << N << " x " << N << endl;
    cout << "迭代次数:        " << iter_count << endl;
    cout << "最终残差:        " << scientific << setprecision(6) << residual << endl;
    cout << "相对误差:        " << scientific << setprecision(6) << error << endl;
    cout << "计算时间:        " << fixed << setprecision(3) << time_ms << " ms" << endl;
    cout << "每次迭代时间:    " << fixed << setprecision(3) 
         << time_ms / iter_count << " ms" << endl;
    
    if (baseline_ms > 0) {
        double speedup = baseline_ms / time_ms;
        cout << "加速比:          " << fixed << setprecision(2) << speedup << "x" << endl;
        
        // 计算性能提升百分比
        double improvement = ((baseline_ms - time_ms) / baseline_ms) * 100.0;
        cout << "性能提升:        " << fixed << setprecision(1) << improvement << "%" << endl;
    }
    
    cout << string(70, '=') << endl;
}

int main(int argc, char* argv[]) {
    #ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    #endif
    
    // 从命令行获取参数
    if (argc < 3) {
        cerr << "用法: " << argv[0] << " <网格尺寸> <线程数>" << endl;
        cerr << "示例: " << argv[0] << " 512 8" << endl;
        return 1;
    }
    
    int N = atoi(argv[1]);
    int num_threads = atoi(argv[2]);
    
    if (N <= 0 || N > 10000) {
        cerr << "错误: 网格尺寸必须在 1 到 10000 之间" << endl;
        return 1;
    }
    
    if (num_threads <= 0 || num_threads > 128) {
        cerr << "错误: 线程数必须在 1 到 128 之间" << endl;
        return 1;
    }
    
    double h = 1.0 / (N + 1);
    // 根据网格规模自适应设置最大迭代次数
    // Gauss-Seidel需要O(N^2)次迭代收敛
    int max_iter = std::max(10000, 4 * N * N);  // 至少10000次，大规模问题用4*N^2
    double tol = 1e-6;
    
    cout << "\n" << string(70, '=') << endl;
    cout << "       访存优化性能对比测试 - 2D泊松方程 Gauss-Seidel" << endl;
    cout << string(70, '=') << endl;
    cout << "网格尺寸:    " << N << " x " << N << " (" << N*N << " 个点)" << endl;
    cout << "线程数:      " << num_threads << endl;
    cout << "最大迭代:    " << max_iter << endl;
    cout << "收敛容差:    " << scientific << tol << endl;
    cout << string(70, '=') << endl;
    
    // 初始化问题
    cout << "\n初始化问题..." << flush;
    vector<double> u, f, u_exact;
    auto init_start = high_resolution_clock::now();
    GaussSeidel2D::init_test_problem(u, f, u_exact, N, h);
    auto init_end = high_resolution_clock::now();
    double init_ms = duration_cast<milliseconds>(init_end - init_start).count();
    cout << " 完成 (" << fixed << setprecision(1) << init_ms << " ms)" << endl;
    
    // 为了公平比较，每个方法使用相同的初始条件
    vector<double> u_baseline = u;
    vector<double> u_test1 = u;
    vector<double> u_test2 = u;
    vector<double> u_test3 = u;
    
    double baseline_time = 0;
    
    // ========== 测试1: 基准版本（当前实现）==========
    {
        cout << "\n[1/4] 运行基准版本..." << flush;
        int iter_count = 0;
        double residual = 0.0;
        
        auto start = high_resolution_clock::now();
        GaussSeidel2D::solve_parallel_redblack(u_baseline, f, N, h, max_iter, tol, 
                                               iter_count, residual, num_threads);
        auto end = high_resolution_clock::now();
        
        baseline_time = duration_cast<microseconds>(end - start).count() / 1000.0;
        double error = compute_error(u_baseline, u_exact, N);
        cout << " 完成" << endl;
        
        print_result("基准: 当前实现 (无分块)", iter_count, residual, 
                    error, baseline_time, N);
    }
    
    // ========== 测试2: 二级Tiling优化 ==========
    {
        cout << "\n[2/4] 运行Tiling优化版本..." << flush;
        int iter_count = 0;
        double residual = 0.0;
        
        auto start = high_resolution_clock::now();
        GaussSeidel2DOptimized::solve_parallel_redblack_tiled(
            u_test1, f, N, h, max_iter, tol, iter_count, residual, num_threads);
        auto end = high_resolution_clock::now();
        
        double time_ms = duration_cast<microseconds>(end - start).count() / 1000.0;
        double error = compute_error(u_test1, u_exact, N);
        cout << " 完成" << endl;
        
        print_result("优化1: 二级Tiling + 预取 + 数据重用", iter_count, residual, 
                    error, time_ms, N, baseline_time);
    }
    
    // ========== 测试3: SIMD优化（实验性）==========
    {
        cout << "\n[3/4] 运行SIMD优化版本..." << flush;
        int iter_count = 0;
        double residual = 0.0;
        
        auto start = high_resolution_clock::now();
        GaussSeidel2DOptimized::solve_parallel_redblack_simd(
            u_test2, f, N, h, max_iter, tol, iter_count, residual, num_threads);
        auto end = high_resolution_clock::now();
        
        double time_ms = duration_cast<microseconds>(end - start).count() / 1000.0;
        double error = compute_error(u_test2, u_exact, N);
        cout << " 完成" << endl;
        
        print_result("优化2: SIMD向量化（实验性）", iter_count, residual, 
                    error, time_ms, N, baseline_time);
    }
    
    // ========== 测试4: 数据重排优化 ==========
    {
        cout << "\n[4/4] 运行数据重排优化版本..." << flush;
        int iter_count = 0;
        double residual = 0.0;
        
        auto start = high_resolution_clock::now();
        GaussSeidel2DOptimized::solve_parallel_redblack_restructured(
            u_test3, f, N, h, max_iter, tol, iter_count, residual, num_threads);
        auto end = high_resolution_clock::now();
        
        double time_ms = duration_cast<microseconds>(end - start).count() / 1000.0;
        double error = compute_error(u_test3, u_exact, N);
        cout << " 完成" << endl;
        
        print_result("优化3: 数据重排 + 连续存储", iter_count, residual, 
                    error, time_ms, N, baseline_time);
    }
    
    // ========== 总结 ==========
    cout << "\n" << string(70, '=') << endl;
    cout << "                      优化技术总结" << endl;
    cout << string(70, '=') << endl;
    cout << "\n【优化1: 二级Tiling】" << endl;
    cout << "  技术: L1/L2缓存分层分块 + 软件预取 + 数据重用" << endl;
    cout << "  优点: 显著提升缓存利用率，减少cache miss" << endl;
    cout << "  适用: 中大规模问题 (N >= 256)" << endl;
    
    cout << "\n【优化2: SIMD向量化】" << endl;
    cout << "  技术: AVX2指令集 + 循环展开" << endl;
    cout << "  缺点: 红黑排序的跨步访问限制SIMD效果" << endl;
    cout << "  适用: 特定硬件和编译器优化" << endl;
    
    cout << "\n【优化3: 数据重排】" << endl;
    cout << "  技术: 红黑点分别连续存储，消除跨步访问" << endl;
    cout << "  优点: 最佳的空间局部性和SIMD潜力" << endl;
    cout << "  缺点: 额外内存开销和映射开销" << endl;
    
    cout << "\n【关键发现】" << endl;
    cout << "  1. Tiling是CPU上最有效的优化（通常1.5-2.5x加速）" << endl;
    cout << "  2. 红黑排序天然限制了连续访问，这是固有瓶颈" << endl;
    cout << "  3. 对于极大规模问题，GPU实现（CUDA）更合适" << endl;
    cout << "  4. 自适应tile大小很重要，需要根据N和缓存调整" << endl;
    
    cout << "\n【进一步优化方向】" << endl;
    cout << "  - NUMA感知的内存分配（多Socket系统）" << endl;
    cout << "  - 使用线性求解器前置条件（Multigrid）" << endl;
    cout << "  - 混合精度计算（float用于迭代，double验证）" << endl;
    cout << "  - 异步通信隐藏同步开销" << endl;
    
    cout << "\n" << string(70, '=') << endl;
    cout << endl;
    
    return 0;
}
