#include <iostream>
#include <iomanip>
#include <chrono>
#include <cmath>
#include "gauss_seidel_2d.h"

using namespace std;
using namespace std::chrono;

// 声明4层tiling函数
namespace GaussSeidel2DTiled {
    void solve_4level_tiling(
        std::vector<double>& u,
        const std::vector<double>& f,
        int N, double h, int max_iter, double tol,
        int& iter_count, double& residual, int num_threads);
}

namespace GaussSeidel3DTiled {
    void solve_4level_tiling(
        std::vector<double>& u,
        const std::vector<double>& f,
        int N, double h, int max_iter, double tol,
        int& iter_count, double& residual, int num_threads);
}

double compute_error_2d(const vector<double>& u, const vector<double>& u_exact, int N) {
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

void test_2d(int N, int num_threads) {
    cout << "\n" << string(70, '=') << endl;
    cout << "2D Poisson - 2-Layer Tiling Optimization Test" << endl;
    cout << string(70, '=') << endl;
    cout << "Grid Size: " << N << " x " << N << endl;
    cout << "Threads:   " << num_threads << endl;
    
    double h = 1.0 / (N + 1);
    int max_iter = 10000;
    double tol = 1e-6;
    
    vector<double> u, f, u_exact;
    GaussSeidel2D::init_test_problem(u, f, u_exact, N, h);
    
    // Test original implementation
    vector<double> u1 = u;
    int iter1 = 0;
    double res1 = 0.0;
    auto start1 = high_resolution_clock::now();
    GaussSeidel2D::solve_parallel_redblack(u1, f, N, h, max_iter, tol, iter1, res1, num_threads);
    auto end1 = high_resolution_clock::now();
    double time1 = duration_cast<microseconds>(end1 - start1).count() / 1000.0;
    double error1 = compute_error_2d(u1, u_exact, N);
    
    // Test 2-layer tiling
    vector<double> u2 = u;
    int iter2 = 0;
    double res2 = 0.0;
    auto start2 = high_resolution_clock::now();
    GaussSeidel2DTiled::solve_4level_tiling(u2, f, N, h, max_iter, tol, iter2, res2, num_threads);
    auto end2 = high_resolution_clock::now();
    double time2 = duration_cast<microseconds>(end2 - start2).count() / 1000.0;
    double error2 = compute_error_2d(u2, u_exact, N);
    
    cout << "\nMethod            | Iters    | Time(ms)  | Error      | Speedup" << endl;
    cout << string(70, '-') << endl;
    cout << "Original          | " << setw(8) << iter1 << " | " 
         << fixed << setprecision(2) << setw(9) << time1 << " | "
         << scientific << setprecision(2) << error1 << " | 1.00x" << endl;
    cout << "2-Layer Tiling    | " << setw(8) << iter2 << " | " 
         << fixed << setprecision(2) << setw(9) << time2 << " | "
         << scientific << setprecision(2) << error2 << " | "
         << fixed << setprecision(2) << (time1 / time2) << "x" << endl;
    cout << string(70, '=') << endl;
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        cerr << "用法: " << argv[0] << " <2d|3d> <网格规模N> <线程数>" << endl;
        return 1;
    }
    
    string dim = argv[1];
    int N = atoi(argv[2]);
    int num_threads = atoi(argv[3]);
    
    if (dim == "2d") {
        test_2d(N, num_threads);
    } else {
        cerr << "暂时只实现了2D测试" << endl;
    }
    
    return 0;
}
