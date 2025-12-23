/**
 * @file openmp_recursive_doubling.cpp
 * @brief 基于 OpenMP 的递归倍增(Recursive Doubling)并行 Thomas 算法
 * @details 参考 tanim72/15418-final-project
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <array>
#include <cmath>
#include <algorithm>
#include <iomanip>
#include <chrono>
#include <omp.h>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;

const double EPSILON = 1e-15;

/**
 * @brief OpenMP 递归倍增并行 Thomas 算法
 * @param n 方程组维数
 * @param a 下对角线系数 (a[0] = 0)
 * @param b 主对角线系数
 * @param c 上对角线系数 (c[n-1] = 0)
 * @param q 右端项
 * @param x 解向量 (输出)
 * @param num_threads OpenMP 线程数
 */
void thomas_recursive_doubling(int n,
                               const vector<double>& a,
                               const vector<double>& b,
                               const vector<double>& c,
                               const vector<double>& q,
                               vector<double>& x,
                               int num_threads) {
    vector<array<double, 4>> R_store(num_threads);
    vector<double> d(n), l(n);
    
    #pragma omp parallel num_threads(num_threads)
    {
        int tid = omp_get_thread_num();
        
        // 计算每个线程的工作范围
        int base = n / num_threads;
        int rem = n % num_threads;
        int local_rows = (tid < rem ? base + 1 : base);
        int offset = (tid < rem ? tid * (base + 1) : rem * (base + 1) + (tid - rem) * base);
        
        // 阶段1: 计算局部系数矩阵 R
        double R00 = 1.0, R01 = 0.0;
        double R10 = 0.0, R11 = 1.0;
        
        for (int i = 0; i < local_rows; i++) {
            int idx = offset + i;
            double a_val = a[idx];
            double mult = (idx > 0 ? b[idx-1] * c[idx-1] : 0.0);
            
            double tmp0 = a_val * R00 - mult * R10;
            double tmp1 = a_val * R01 - mult * R11;
            R10 = R00;
            R11 = R01;
            R00 = tmp0;
            R01 = tmp1;
            
            // 归一化防止溢出
            double scale = max({abs(R00), abs(R01), abs(R10), abs(R11)});
            if (scale > 0) {
                R00 /= scale; R01 /= scale;
                R10 /= scale; R11 /= scale;
            }
        }
        
        R_store[tid] = {R00, R01, R10, R11};
        #pragma omp barrier
        
        // 阶段2: 递归倍增通信
        int stages = (int)log2(num_threads);
        for (int s = 0; s < stages; s++) {
            int dist = 1 << s;
            auto my_R = R_store[tid];
            int partner = (tid >= dist ? tid - dist : tid + dist);
            auto p_R = R_store[partner];
            
            double n00 = my_R[0] * p_R[0] + my_R[1] * p_R[2];
            double n01 = my_R[0] * p_R[1] + my_R[1] * p_R[3];
            double n10 = my_R[2] * p_R[0] + my_R[3] * p_R[2];
            double n11 = my_R[2] * p_R[1] + my_R[3] * p_R[3];
            
            // 归一化
            double scale = max({abs(n00), abs(n01), abs(n10), abs(n11)});
            if (scale > 0) {
                n00 /= scale; n01 /= scale;
                n10 /= scale; n11 /= scale;
            }
            
            #pragma omp barrier
            R_store[tid] = {n00, n01, n10, n11};
            #pragma omp barrier
        }
        
        // 阶段3: 计算边界 d 值
        {
            auto& R = R_store[tid];
            int boundary = offset + local_rows - 1;
            double denom = R[2] + R[3];
            if (abs(denom) < EPSILON) {
                denom = (denom >= 0 ? EPSILON : -EPSILON);
            }
            d[boundary] = (R[0] + R[1]) / denom;
        }
        #pragma omp barrier
        
        // 阶段4: 构建完整的 d 和 l 数组
        if (tid == 0) {
            l[0] = 0.0;
            d[0] = a[0];
            for (int i = 1; i < local_rows; i++) {
                double dp = d[i-1];
                double dm = abs(dp) < EPSILON ? (dp >= 0 ? EPSILON : -EPSILON) : dp;
                l[i] = b[i-1] / dm;
                d[i] = a[i] - l[i] * c[i-1];
            }
        } else {
            for (int i = 0; i < local_rows; i++) {
                int idx = offset + i;
                double dp = d[idx-1];
                double dm = abs(dp) < EPSILON ? (dp >= 0 ? EPSILON : -EPSILON) : dp;
                l[idx] = b[idx-1] / dm;
                d[idx] = a[idx] - l[idx] * c[idx-1];
            }
        }
        #pragma omp barrier
        
        // 阶段5: 前向和回代（单线程）
        #pragma omp single
        {
            vector<double> y(n);
            y[0] = q[0];
            for (int i = 1; i < n; i++) {
                y[i] = q[i] - l[i] * y[i-1];
            }
            
            double dm = abs(d[n-1]) < EPSILON ? (d[n-1] >= 0 ? EPSILON : -EPSILON) : d[n-1];
            x[n-1] = y[n-1] / dm;
            for (int i = n - 2; i >= 0; i--) {
                dm = abs(d[i]) < EPSILON ? (d[i] >= 0 ? EPSILON : -EPSILON) : d[i];
                x[i] = (y[i] - c[i] * x[i+1]) / dm;
            }
        }
    }
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

/**
 * @brief 串行 Thomas 算法（用于对比）
 */
vector<double> thomas_serial(int n,
                            const vector<double>& a,
                            const vector<double>& b,
                            const vector<double>& c,
                            const vector<double>& d) {
    vector<double> gamma(n, 0.0);
    vector<double> rho(n, 0.0);
    
    gamma[0] = c[0] / b[0];
    rho[0] = d[0] / b[0];
    
    for (int i = 1; i < n; i++) {
        double denom = b[i] - a[i] * gamma[i-1];
        if (i < n - 1) {
            gamma[i] = c[i] / denom;
        }
        rho[i] = (d[i] - a[i] * rho[i-1]) / denom;
    }
    
    vector<double> x(n, 0.0);
    x[n-1] = rho[n-1];
    for (int i = n - 2; i >= 0; i--) {
        x[i] = rho[i] - gamma[i] * x[i+1];
    }
    
    return x;
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
    
    for (int i = 0; i < n; i++) {
        fin >> b[i];
    }
    
    a[0] = 0.0;
    for (int i = 1; i < n; i++) {
        fin >> a[i];
    }
    
    for (int i = 0; i < n - 1; i++) {
        fin >> c[i];
    }
    c[n-1] = 0.0;
    
    for (int i = 0; i < n; i++) {
        fin >> d[i];
    }
    
    fin.close();
    
    cout << "========================================================" << endl;
    cout << "OpenMP 递归倍增 (Recursive Doubling) 并行 Thomas 算法" << endl;
    cout << "========================================================" << endl;
    cout << "问题规模: N = " << n << endl;
    cout << "--------------------------------------------------------" << endl;
    
    // 串行版本
    cout << "\n[串行版本]" << endl;
    auto t_start = chrono::high_resolution_clock::now();
    vector<double> x_serial = thomas_serial(n, a, b, c, d);
    auto t_end = chrono::high_resolution_clock::now();
    double time_serial = chrono::duration<double>(t_end - t_start).count();
    double error_serial = verify_solution(n, a, b, c, d, x_serial);
    
    cout << fixed << setprecision(6);
    cout << "求解时间: " << time_serial << " 秒" << endl;
    cout << "最大残差: " << scientific << error_serial << endl;
    
    // 并行版本 (1, 2, 4, 8 线程)
    vector<int> thread_counts = {1, 2, 4, 8};
    
    for (int nt : thread_counts) {
        cout << "\n[并行版本 - " << nt << " 线程]" << endl;
        vector<double> x(n, 0.0);
        
        t_start = chrono::high_resolution_clock::now();
        thomas_recursive_doubling(n, a, b, c, d, x, nt);
        t_end = chrono::high_resolution_clock::now();
        
        double time_parallel = chrono::duration<double>(t_end - t_start).count();
        double error_parallel = verify_solution(n, a, b, c, d, x);
        double speedup = time_serial / time_parallel;
        
        cout << fixed << setprecision(6);
        cout << "求解时间: " << time_parallel << " 秒" << endl;
        cout << "最大残差: " << scientific << error_parallel << endl;
        cout << fixed << setprecision(2);
        cout << "加速比: " << speedup << "x" << endl;
    }
    
    cout << "--------------------------------------------------------" << endl;
    
    return 0;
}
