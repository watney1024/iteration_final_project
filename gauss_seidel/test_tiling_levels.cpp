#include <iostream>
#include <iomanip>
#include <chrono>
#include <cmath>
#include "gauss_seidel_2d.h"

using namespace std;
using namespace std::chrono;

// 声明不同tiling版本
extern void solve_no_tiling(
    std::vector<double>&, const std::vector<double>&,
    int, double, int, double, int&, double&, int);

extern void solve_1level_tiling(
    std::vector<double>&, const std::vector<double>&,
    int, double, int, double, int&, double&, int);

double compute_error(const vector<double>& u, const vector<double>& u_exact, int N) {
    double error = 0.0, norm = 0.0;
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

void run_test(const string& name, 
              void (*solver)(vector<double>&, const vector<double>&, int, double, int, double, int&, double&, int),
              const vector<double>& u_init, const vector<double>& f, const vector<double>& u_exact,
              int N, double h, int max_iter, double tol, int num_threads) {
    
    vector<double> u = u_init;
    int iter_count = 0;
    double residual = 0.0;
    
    auto start = high_resolution_clock::now();
    solver(u, f, N, h, max_iter, tol, iter_count, residual, num_threads);
    auto end = high_resolution_clock::now();
    
    double time_ms = duration_cast<microseconds>(end - start).count() / 1000.0;
    double error = compute_error(u, u_exact, N);
    
    cout << setw(20) << left << name << " | "
         << setw(8) << iter_count << " | "
         << fixed << setprecision(2) << setw(10) << time_ms << " | "
         << scientific << setprecision(2) << error << endl;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "用法: " << argv[0] << " <N> <threads>" << endl;
        return 1;
    }
    
    int N = atoi(argv[1]);
    int num_threads = atoi(argv[2]);
    
    cout << "\n" << string(75, '=') << endl;
    cout << "Tiling层数对比测试 - N=" << N << ", 线程=" << num_threads << endl;
    cout << string(75, '=') << endl;
    
    double h = 1.0 / (N + 1);
    int max_iter = 10000;
    double tol = 1e-6;
    
    vector<double> u, f, u_exact;
    GaussSeidel2D::init_test_problem(u, f, u_exact, N, h);
    
    cout << "\n方法                  | 迭代次数 | 时间(ms)   | 误差" << endl;
    cout << string(75, '-') << endl;
    
    run_test("0层 (无tiling)", solve_no_tiling, u, f, u_exact, N, h, max_iter, tol, num_threads);
    run_test("1层 tiling", solve_1level_tiling, u, f, u_exact, N, h, max_iter, tol, num_threads);
    run_test("2层 tiling", GaussSeidel2D::solve_parallel_redblack, u, f, u_exact, N, h, max_iter, tol, num_threads);
    
    cout << string(75, '=') << endl;
    
    return 0;
}
