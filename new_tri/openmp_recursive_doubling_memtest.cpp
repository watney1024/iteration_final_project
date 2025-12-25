/**
 * @file openmp_recursive_doubling_memtest.cpp
 * @brief OpenMP 递归倍增算法 - 内存数据生成版本
 */

#include <iostream>
#include <vector>
#include <array>
#include <cmath>
#include <algorithm>
#include <iomanip>
#include <chrono>
#include <random>
#include <omp.h>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;

const double EPSILON = 1e-15;

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
 * @brief OpenMP 递归倍增并行 Thomas 算法
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
        
        int base = n / num_threads;
        int rem = n % num_threads;
        int local_rows = (tid < rem ? base + 1 : base);
        int offset = (tid < rem ? tid * (base + 1) : rem * (base + 1) + (tid - rem) * base);
        
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
            
            double scale = max({abs(R00), abs(R01), abs(R10), abs(R11)});
            if (scale > 0) {
                R00 /= scale; R01 /= scale;
                R10 /= scale; R11 /= scale;
            }
        }
        
        R_store[tid] = {R00, R01, R10, R11};
        #pragma omp barrier
        
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
            
            double scale = max({abs(n00), abs(n01), abs(n10), abs(n11)});
            if (scale > 0) {
                n00 /= scale; n01 /= scale;
                n10 /= scale; n11 /= scale;
            }
            
            R_store[tid] = {n00, n01, n10, n11};
            #pragma omp barrier
        }
        
        double p_sum = 0.0, q_sum = 0.0;
        for (int i = 0; i < local_rows; i++) {
            int idx = offset + i;
            p_sum += b[idx];
            q_sum += b[idx] * q[idx];
        }
        d[tid] = p_sum;
        l[tid] = q_sum;
        #pragma omp barrier
        
        for (int s = 0; s < stages; s++) {
            int dist = 1 << s;
            if (tid >= dist) {
                int partner = tid - dist;
                d[tid] += d[partner];
                l[tid] += l[partner];
            }
            #pragma omp barrier
        }
        
        for (int i = 0; i < local_rows; i++) {
            int idx = offset + i;
            x[idx] = (q[idx] - (idx > 0 ? c[idx-1] * x[idx-1] : 0.0)) / b[idx];
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
    int num_threads = 1;
    
    if (argc > 1) {
        n = atoi(argv[1]);
        if (n < 1) n = 1000000;
    }
    if (argc > 2) {
        num_threads = atoi(argv[2]);
        if (num_threads < 1) num_threads = 1;
    }
    
    cout << "========================================================" << endl;
    cout << "OpenMP Recursive Doubling Thomas Algorithm" << endl;
    cout << "========================================================" << endl;
    cout << "Problem size: N = " << n << endl;
    cout << "Threads: " << num_threads << endl;
    cout << "--------------------------------------------------------" << endl;
    
    // 生成测试数据
    vector<double> a, b, c, d;
    generate_test_data(n, a, b, c, d);
    
    // 求解
    vector<double> x(n, 0.0);
    auto t_start = chrono::high_resolution_clock::now();
    thomas_recursive_doubling(n, a, b, c, d, x, num_threads);
    auto t_end = chrono::high_resolution_clock::now();
    
    double time_parallel = chrono::duration<double>(t_end - t_start).count();
    double error = verify_solution(n, a, b, c, d, x);
    
    cout << fixed << setprecision(6);
    cout << "Solve time: " << time_parallel << " seconds" << endl;
    cout << "Max residual: " << scientific << error << endl;
    cout << "--------------------------------------------------------" << endl;
    
    return 0;
}
