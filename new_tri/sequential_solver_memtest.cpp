/**
 * @file sequential_solver_memtest.cpp
 * @brief 串行 Thomas 算法 - 内存数据生成版本
 */

#include <iostream>
#include <vector>
#include <iomanip>
#include <chrono>
#include <random>
#include <cmath>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;

/**
 * @brief 在内存中生成对角占优的三对角矩阵测试数据
 */
void generate_test_data(int n, 
                       vector<double>& a,
                       vector<double>& b,
                       vector<double>& c,
                       vector<double>& d) {
    mt19937 rng(12345);
    uniform_real_distribution<double> dist(1.0, 10.0);
    
    a.resize(n);
    b.resize(n);
    c.resize(n);
    d.resize(n);
    
    for (int i = 0; i < n; i++) {
        if (i > 0) {
            a[i] = dist(rng);
        } else {
            a[i] = 0.0;
        }
        
        if (i < n - 1) {
            c[i] = dist(rng);
        } else {
            c[i] = 0.0;
        }
        
        double sum = (i > 0 ? a[i] : 0.0) + (i < n - 1 ? c[i] : 0.0);
        b[i] = sum + dist(rng) + 5.0;
        
        d[i] = dist(rng);
    }
}

/**
 * @brief 串行 Thomas 算法求解三对角线性方程组
 */
vector<double> thomas_solver(int n, 
                            const vector<double>& a,
                            const vector<double>& b,
                            const vector<double>& c,
                            const vector<double>& d) {
    vector<double> gamma(n, 0.0);
    vector<double> rho(n, 0.0);
    
    // 前向消元
    gamma[0] = c[0] / b[0];
    rho[0] = d[0] / b[0];
    
    for (int i = 1; i < n; i++) {
        double denom = b[i] - a[i] * gamma[i-1];
        if (i < n - 1) {
            gamma[i] = c[i] / denom;
        }
        rho[i] = (d[i] - a[i] * rho[i-1]) / denom;
    }
    
    // 回代
    vector<double> x(n, 0.0);
    x[n-1] = rho[n-1];
    for (int i = n - 2; i >= 0; i--) {
        x[i] = rho[i] - gamma[i] * x[i+1];
    }
    
    return x;
}

/**
 * @brief 验证解的正确性
 */
double verify_solution(int n,
                       const vector<double>& a,
                       const vector<double>& b,
                       const vector<double>& c,
                       const vector<double>& d,
                       const vector<double>& x) {
    double max_error = 0.0;
    
    double residual = b[0] * x[0] + c[0] * x[1] - d[0];
    max_error = max(max_error, abs(residual));
    
    for (int i = 1; i < n - 1; i++) {
        residual = a[i] * x[i-1] + b[i] * x[i] + c[i] * x[i+1] - d[i];
        max_error = max(max_error, abs(residual));
    }
    
    residual = a[n-1] * x[n-2] + b[n-1] * x[n-1] - d[n-1];
    max_error = max(max_error, abs(residual));
    
    return max_error;
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(65001);
#endif
    
    int n = 1000000;
    
    if (argc > 1) {
        n = atoi(argv[1]);
        if (n < 1) n = 1000000;
    }
    
    cout << "==================================" << endl;
    cout << "Sequential Thomas Solver" << endl;
    cout << "==================================" << endl;
    cout << "Problem size: N = " << n << endl;
    cout << "----------------------------------" << endl;
    
    // 生成测试数据
    vector<double> a, b, c, d;
    generate_test_data(n, a, b, c, d);
    
    // 求解
    auto t_start = chrono::high_resolution_clock::now();
    vector<double> x = thomas_solver(n, a, b, c, d);
    auto t_end = chrono::high_resolution_clock::now();
    
    double elapsed = chrono::duration<double>(t_end - t_start).count();
    
    // 验证
    double error = verify_solution(n, a, b, c, d, x);
    
    cout << fixed << setprecision(6);
    cout << "Solve time: " << elapsed << " seconds" << endl;
    cout << "Max residual: " << scientific << error << endl;
    cout << "----------------------------------" << endl;
    
    return 0;
}
