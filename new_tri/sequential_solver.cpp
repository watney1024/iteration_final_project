/**
 * @file sequential_solver.cpp
 * @brief 串行 Thomas 算法（追赶法）求解三对角方程组
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>
#include <chrono>
#include <cmath>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;

/**
 * @brief 串行 Thomas 算法求解三对角线性方程组
 * @param n 方程组维数
 * @param a 下对角线系数 (a[0] 不使用)
 * @param b 主对角线系数
 * @param c 上对角线系数 (c[n-1] 不使用)
 * @param d 右端项
 * @return 解向量
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
    
    // 验证第一行
    double residual = b[0] * x[0] + c[0] * x[1] - d[0];
    max_error = max(max_error, abs(residual));
    
    // 验证中间行
    for (int i = 1; i < n - 1; i++) {
        residual = a[i] * x[i-1] + b[i] * x[i] + c[i] * x[i+1] - d[i];
        max_error = max(max_error, abs(residual));
    }
    
    // 验证最后一行
    residual = a[n-1] * x[n-2] + b[n-1] * x[n-1] - d[n-1];
    max_error = max(max_error, abs(residual));
    
    return max_error;
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(65001);
#endif
    
    string input_file = "inputs/test_input.txt";
    
    if (argc > 1) {
        input_file = argv[1];
    }
    
    // 读取输入文件
    ifstream fin(input_file);
    if (!fin) {
        cerr << "Error: Cannot open file " << input_file << endl;
        return 1;
    }
    
    int n;
    fin >> n;
    
    vector<double> a(n), b(n), c(n), d(n);
    
    // 读取主对角线
    for (int i = 0; i < n; i++) {
        fin >> b[i];
    }
    
    // 读取下对角线 (a[0] = 0)
    a[0] = 0.0;
    for (int i = 1; i < n; i++) {
        fin >> a[i];
    }
    
    // 读取上对角线 (c[n-1] = 0)
    for (int i = 0; i < n - 1; i++) {
        fin >> c[i];
    }
    c[n-1] = 0.0;
    
    // 读取右端项
    for (int i = 0; i < n; i++) {
        fin >> d[i];
    }
    
    fin.close();
    
    cout << "==================================" << endl;
    cout << "串行 Thomas 算法求解器" << endl;
    cout << "==================================" << endl;
    cout << "问题规模: N = " << n << endl;
    cout << "----------------------------------" << endl;
    
    // 求解
    auto t_start = chrono::high_resolution_clock::now();
    vector<double> x = thomas_solver(n, a, b, c, d);
    auto t_end = chrono::high_resolution_clock::now();
    
    double elapsed = chrono::duration<double>(t_end - t_start).count();
    
    // 验证
    double error = verify_solution(n, a, b, c, d, x);
    
    cout << fixed << setprecision(6);
    cout << "求解时间: " << elapsed << " 秒" << endl;
    cout << "最大残差: " << scientific << error << endl;
    cout << "----------------------------------" << endl;
    
    // 输出部分解（仅当问题规模较小时）
    if (n <= 10) {
        cout << "解向量 x: ";
        for (int i = 0; i < n; i++) {
            cout << fixed << setprecision(6) << x[i] << " ";
        }
        cout << endl;
    }
    
    return 0;
}
