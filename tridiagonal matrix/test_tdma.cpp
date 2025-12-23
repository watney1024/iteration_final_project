#include "tdma_solver.h"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <cmath>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;
using namespace std::chrono;

// 打印结果
void print_result(const string& method, int n, double time_ms, double residual) {
    cout << "\n" << string(70, '=') << endl;
    cout << "方法: " << method << endl;
    cout << string(70, '-') << endl;
    cout << "问题规模:        " << n << endl;
    cout << "计算时间:        " << fixed << setprecision(6) << time_ms << " ms" << endl;
    cout << "残差 (L2范数):   " << scientific << setprecision(6) << residual << endl;
    cout << string(70, '=') << endl;
}

int main(int argc, char* argv[]) {
    // 设置 Windows 控制台为 UTF-8 编码
    #ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    #endif
    
    cout << "\n";
    cout << "======================================================================" << endl;
    cout << "       三对角矩阵求解器性能测试 (TDMA - Thomas & PCR)               " << endl;
    cout << "       参考: jihoonakang/parallel_tdma_cpp                           " << endl;
    cout << "======================================================================" << endl;
    
    // 测试不同规模
    // 参考 tanim72/15418-final-project，使用更大规模测试
    vector<int> test_sizes = {16384, 65536, 131072};
    
    cout << "\n算法说明:" << endl;
    cout << "  串行 Thomas 算法: 经典追赶法，时间复杂度 O(n)" << endl;
    cout << "  并行 PCR 算法:    并行循环归约，适合并行计算" << endl;
    cout << "  - 每次迭代消除一半方程" << endl;
    cout << "  - 迭代次数: log2(n)" << endl;
    cout << "  - 所有更新操作可并行" << endl;
    
    for (int n : test_sizes) {
        cout << "\n\n";
        cout << "=====================================================================" << endl;
        cout << "                    测试规模: N = " << n << endl;
        cout << "=====================================================================" << endl;
        
        // 生成测试系统
        vector<double> a, b, c, d;
        cout << "\n正在生成对角占优三对角矩阵..." << endl;
        TDMASolver::generate_test_system(a, b, c, d, n);
        cout << "矩阵生成完成" << endl;
        
        // 保存原始数据用于验证
        vector<double> a_orig = a;
        vector<double> b_orig = b;
        vector<double> c_orig = c;
        vector<double> d_orig = d;
        
        // ========== 测试1: 串行 Thomas 算法 ==========
        cout << "\n开始测试串行 Thomas 算法..." << endl;
        {
            vector<double> x(n);
            
            auto start = high_resolution_clock::now();
            TDMASolver::solve_thomas(a, b, c, d, x, n);
            auto end = high_resolution_clock::now();
            
            double time_ms = duration_cast<microseconds>(end - start).count() / 1000.0;
            double residual = TDMASolver::verify_solution(a_orig, b_orig, c_orig, d_orig, x, n);
            
            print_result("串行 Thomas 算法", n, time_ms, residual);
        }
        
        // ========== 测试2: 并行 PCR 算法 (不同线程数) ==========
        int thread_counts[] = {1, 2, 4, 8};
        vector<double> times(4);
        
        for (int idx = 0; idx < 4; ++idx) {
            int threads = thread_counts[idx];
            cout << "\n开始测试并行 PCR 算法 (" << threads << " 线程)..." << endl;
            
            vector<double> x(n);
            
            auto start = high_resolution_clock::now();
            TDMASolver::solve_pcr(a_orig, b_orig, c_orig, d_orig, x, n, threads);
            auto end = high_resolution_clock::now();
            
            double time_ms = duration_cast<microseconds>(end - start).count() / 1000.0;
            double residual = TDMASolver::verify_solution(a_orig, b_orig, c_orig, d_orig, x, n);
            times[idx] = time_ms;
            
            string method = "并行 PCR 算法 (" + to_string(threads) + " 线程)";
            print_result(method, n, time_ms, residual);
            
            if (threads > 1 && times[0] > 0) {
                double speedup = times[0] / time_ms;
                double efficiency = speedup / threads * 100.0;
                
                cout << "加速比 (相对于1线程): " << fixed << setprecision(2) 
                     << speedup << "x" << endl;
                cout << "并行效率:             " << fixed << setprecision(1) 
                     << efficiency << "%" << endl;
            }
        }
    }
    
    // ========== 性能分析总结 ==========
    cout << "\n\n";
    cout << "======================================================================" << endl;
    cout << "                         性能分析总结                                 " << endl;
    cout << "======================================================================" << endl;
    
    cout << "\n算法特点:" << endl;
    cout << "1. Thomas 算法:" << endl;
    cout << "   - 串行算法，无法并行化（前向消元和回代都有数据依赖）" << endl;
    cout << "   - 时间复杂度 O(n)，非常高效" << endl;
    cout << "   - 适合中小规模问题或串行环境" << endl;
    
    cout << "\n2. PCR 算法:" << endl;
    cout << "   - 天然并行算法，每个归约步骤的更新都独立" << endl;
    cout << "   - 时间复杂度 O(n log n)，但可并行" << endl;
    cout << "   - 并行后复杂度降为 O(log n)（理想情况）" << endl;
    cout << "   - 适合大规模问题和并行环境" << endl;
    
    cout << "\n性能观察:" << endl;
    cout << "- 对于小规模问题，串行 Thomas 算法最快" << endl;
    cout << "- 随着规模增大，并行 PCR 的优势逐渐显现" << endl;
    cout << "- 并行效率受问题规模、线程数、cache等因素影响" << endl;
    cout << "- PCR 需要更多内存访问，可能受内存带宽限制" << endl;
    
    cout << "\n实现细节:" << endl;
    cout << "  - 使用 OpenMP 进行并行化" << endl;
    cout << "  - 静态调度减少线程调度开销" << endl;
    cout << "  - 乒乓缓冲策略避免数据竞争" << endl;
    cout << "  - log2(n) 次归约迭代" << endl;
    
    cout << "\n参考文献:" << endl;
    cout << "  - Ji-Hoon Kang, parallel_tdma_cpp" << endl;
    cout << "  - Karniadakis & Kirby, Parallel Scientific Computing in C++ and MPI" << endl;
    cout << "  - Laszlo et al., ACM TOMS 42, 31 (2016)" << endl;
    
    cout << "\n" << endl;
    
    return 0;
}
