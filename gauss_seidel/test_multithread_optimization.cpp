#include <iostream>
#include <vector>
#include <cmath>
#include <chrono>
#include <iomanip>
#include "gauss_seidel_2d.h"
#include "gauss_seidel_2d_mt_optimized.h"

using namespace std;

// 初始化问题
void initialize_problem(vector<double>& u, vector<double>& f, 
                        vector<double>& exact, int N, double h) {
    u.assign((N + 2) * (N + 2), 0.0);
    f.resize(N * N);
    exact.resize((N + 2) * (N + 2), 0.0);
    
    const double PI = 3.14159265358979323846;
    
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            double x = (i + 1) * h;
            double y = (j + 1) * h;
            f[i * N + j] = 2.0 * PI * PI * sin(PI * x) * sin(PI * y);
            exact[(i + 1) * (N + 2) + (j + 1)] = sin(PI * x) * sin(PI * y);
        }
    }
}

// 计算相对误差
double compute_error(const vector<double>& u, const vector<double>& exact, int N) {
    double error_norm = 0.0;
    double exact_norm = 0.0;
    
    for (int i = 1; i <= N; ++i) {
        for (int j = 1; j <= N; ++j) {
            int idx = i * (N + 2) + j;
            double diff = u[idx] - exact[idx];
            error_norm += diff * diff;
            exact_norm += exact[idx] * exact[idx];
        }
    }
    
    return sqrt(error_norm / exact_norm);
}

// 测试函数类型
typedef void (*SolverFunc)(vector<double>&, const vector<double>&, int, double,
                          int, double, int&, double&, int);

// 运行测试
void run_test(const string& name, SolverFunc solver, 
              int N, double h, int max_iter, double tol,
              const vector<double>& f, const vector<double>& exact,
              int num_threads) {
    
    vector<double> u;
    initialize_problem(u, const_cast<vector<double>&>(f), 
                      const_cast<vector<double>&>(exact), N, h);
    
    int iter_count = 0;
    double residual = 0.0;
    
    auto start = chrono::high_resolution_clock::now();
    solver(u, f, N, h, max_iter, tol, iter_count, residual, num_threads);
    auto end = chrono::high_resolution_clock::now();
    
    double time_ms = chrono::duration<double, milli>(end - start).count();
    double rel_error = compute_error(u, exact, N);
    
    cout << setw(25) << left << name << " | "
         << setw(8) << iter_count << " | "
         << setw(10) << fixed << setprecision(2) << time_ms << " ms | "
         << setw(10) << scientific << setprecision(3) << residual << " | "
         << setw(10) << scientific << setprecision(3) << rel_error << endl;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "用法: " << argv[0] << " <网格规模N> <线程数>" << endl;
        return 1;
    }
    
    int N = atoi(argv[1]);
    int num_threads = atoi(argv[2]);
    
    double h = 1.0 / (N + 1);
    int max_iter = std::max(10000, N * N / 2);  // 自适应迭代次数
    double tol = 1e-6;
    
    vector<double> f, exact, u_dummy;
    initialize_problem(u_dummy, f, exact, N, h);
    
    cout << "\n" << string(80, '=') << endl;
    cout << "多线程性能优化对比测试" << endl;
    cout << string(80, '=') << endl;
    cout << "网格规模: " << N << " x " << N << endl;
    cout << "线程数:   " << num_threads << endl;
    cout << "最大迭代: " << max_iter << endl;
    cout << "收敛容差: " << scientific << tol << endl;
    cout << string(80, '=') << endl;
    
    cout << "\n" << setw(25) << left << "方法" << " | "
         << setw(8) << "迭代次数" << " | "
         << setw(13) << "时间" << " | "
         << setw(10) << "残差" << " | "
         << setw(10) << "相对误差" << endl;
    cout << string(80, '-') << endl;
    
    // 基准：原始实现
    run_test("基准 (原实现)", 
             GaussSeidel2D::solve_parallel_redblack,
             N, h, max_iter, tol, f, exact, num_threads);
    
    // 优化1：消除隐式栅障
    run_test("优化1: 无隐式栅障",
             GaussSeidel2DMultiThread::solve_no_implicit_barrier,
             N, h, max_iter, tol, f, exact, num_threads);
    
    // 优化2：行分块
    run_test("优化2: 行分块",
             GaussSeidel2DMultiThread::solve_row_blocking,
             N, h, max_iter, tol, f, exact, num_threads);
    
    // 优化3：波前法（警告：对大规模问题慢）
    if (N <= 128) {
        run_test("优化3: 波前法",
                 GaussSeidel2DMultiThread::solve_wavefront_pipeline,
                 N, h, max_iter, tol, f, exact, num_threads);
    } else {
        cout << setw(25) << left << "优化3: 波前法" << " | "
             << "跳过 (N>128时过慢)" << endl;
    }
    
    cout << string(80, '=') << endl;
    cout << "\n关键发现:" << endl;
    cout << "1. 消除隐式栅障可显著减少线程同步开销" << endl;
    cout << "2. 行分块策略降低False Sharing影响" << endl;
    cout << "3. 波前法理论最优但实现开销大" << endl;
    cout << "4. 红黑排序本质上限制了并行度" << endl;
    cout << "\n建议: 对于N>=256，考虑使用多重网格或GPU实现\n" << endl;
    
    return 0;
}
