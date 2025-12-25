/**
 * @file profile_brugnano.cpp
 * @brief 带详细性能分析的 Brugnano 算法
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <iomanip>
#include <chrono>
#include <omp.h>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;

// 复制原来的算法函数
void modified_thomas_algorithm(int m, 
                               vector<double>& a,
                               vector<double>& b,
                               vector<double>& c,
                               vector<double>& d) {
    if (m == 1) {
        d[0] = d[0] / b[0];
        a[0] = 0.0;
        c[0] = 0.0;
        return;
    }
    
    d[0] = d[0] / b[0];
    c[0] = c[0] / b[0];
    a[0] = a[0] / b[0];
    
    if (m > 1) {
        d[1] = d[1] / b[1];
        c[1] = c[1] / b[1];
        a[1] = a[1] / b[1];
    }
    
    for (int i = 2; i < m; i++) {
        double denom = b[i] - a[i] * c[i-1];
        if (abs(denom) < 1e-10) denom = 1e-10;
        double r = 1.0 / denom;
        d[i] = r * (d[i] - a[i] * d[i-1]);
        c[i] = r * c[i];
        a[i] = -r * (a[i] * a[i-1]);
    }
    
    for (int i = m - 3; i >= 1; i--) {
        d[i] = d[i] - c[i] * d[i+1];
        c[i] = -c[i] * c[i+1];
        a[i] = a[i] - c[i] * a[i+1];
    }
    
    if (m >= 2) {
        double r = 1.0 / (1.0 - a[1] * c[0]);
        d[0] = r * (d[0] - a[0] * d[1]);
        c[0] = -r * c[0] * c[1];
        a[0] = r * a[0];
    }
}

void standard_thomas_solver(int size,
                           vector<double>& a,
                           vector<double>& b,
                           vector<double>& c,
                           vector<double>& d) {
    vector<double> gamma(size, 0.0);
    
    gamma[0] = c[0] / b[0];
    d[0] = d[0] / b[0];
    
    for (int i = 1; i < size; i++) {
        double denom = b[i] - a[i] * gamma[i-1];
        if (i < size - 1) {
            gamma[i] = c[i] / denom;
        }
        d[i] = (d[i] - a[i] * d[i-1]) / denom;
    }
    
    for (int i = size - 2; i >= 0; i--) {
        d[i] = d[i] - gamma[i] * d[i+1];
    }
}

void thomas_brugnano(int n,
                    const vector<double>& global_a,
                    const vector<double>& global_b,
                    const vector<double>& global_c,
                    const vector<double>& global_d,
                    vector<double>& global_x,
                    int num_threads) {
    
    vector<int> chunk_sizes(num_threads);
    vector<int> start_indices(num_threads);
    
    int base = n / num_threads;
    int rem = n % num_threads;
    int cur = 0;
    for (int i = 0; i < num_threads; i++) {
        chunk_sizes[i] = base + (i < rem ? 1 : 0);
        start_indices[i] = cur;
        cur += chunk_sizes[i];
    }
    
    vector<vector<double>> all_coefs(num_threads, vector<double>(6));
    
    #pragma omp parallel num_threads(num_threads)
    {
        int tid = omp_get_thread_num();
        int start_idx = start_indices[tid];
        int m = chunk_sizes[tid];
        
        vector<double> local_a(m), local_b(m), local_c(m), local_d(m);
        for (int i = 0; i < m; i++) {
            int gi = start_idx + i;
            local_a[i] = global_a[gi];
            local_b[i] = global_b[gi];
            local_c[i] = global_c[gi];
            local_d[i] = global_d[gi];
        }
        
        if (m == 1) {
            local_d[0] /= local_b[0];
            local_a[0] = local_c[0] = 0.0;
        } else {
            modified_thomas_algorithm(m, local_a, local_b, local_c, local_d);
        }
        
        all_coefs[tid][0] = local_a[0];
        all_coefs[tid][1] = local_c[0];
        all_coefs[tid][2] = local_d[0];
        all_coefs[tid][3] = local_a[m-1];
        all_coefs[tid][4] = local_c[m-1];
        all_coefs[tid][5] = local_d[m-1];
        
        #pragma omp barrier
        
        #pragma omp single
        {
            int R = 2 * num_threads;
            vector<double> ra(R, 0.0), rb(R, 1.0), rc(R, 0.0), rd(R);
            
            for (int i = 0; i < num_threads; i++) {
                int e1 = 2 * i;
                int e2 = 2 * i + 1;
                
                ra[e1] = all_coefs[i][0];
                rc[e1] = all_coefs[i][1];
                rd[e1] = all_coefs[i][2];
                ra[e2] = all_coefs[i][3];
                rc[e2] = all_coefs[i][4];
                rd[e2] = all_coefs[i][5];
            }
            
            for (int i = 0; i < num_threads - 1; i++) {
                int curr_end = 2 * i + 1;
                int next_start = 2 * (i + 1);
                rc[curr_end] = -ra[next_start];
                ra[next_start] = -rc[curr_end];
            }
            
            standard_thomas_solver(R, ra, rb, rc, rd);
            
            for (int i = 0; i < num_threads; i++) {
                int s = start_indices[i];
                int e = s + chunk_sizes[i] - 1;
                global_x[s] = rd[2*i];
                global_x[e] = rd[2*i+1];
            }
        }
        
        #pragma omp barrier
        
        int s = start_idx;
        double d0 = global_x[s];
        double dN = global_x[s + m - 1];
        
        for (int i = 1; i < m - 1; i++) {
            global_x[s + i] = local_d[i] - local_a[i] * d0 - local_c[i] * dN;
        }
    }
}

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
    
    auto total_start = chrono::high_resolution_clock::now();
    
    string input_file = "inputs/test_input.txt";
    int num_threads = 1;
    
    if (argc > 1) {
        input_file = argv[1];
    }
    if (argc > 2) {
        num_threads = atoi(argv[2]);
        if (num_threads < 1) num_threads = 1;
    }
    
    cout << "=====================================================" << endl;
    cout << "性能分析版本 - OpenMP Brugnano" << endl;
    cout << "=====================================================" << endl;
    
    // 读取文件
    auto io_start = chrono::high_resolution_clock::now();
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
    auto io_end = chrono::high_resolution_clock::now();
    double io_time = chrono::duration<double>(io_end - io_start).count();
    
    cout << "问题规模: N = " << n << endl;
    cout << "线程数: " << num_threads << endl;
    cout << "-----------------------------------------------------" << endl;
    
    // 求解
    vector<double> x(n, 0.0);
    
    auto solve_start = chrono::high_resolution_clock::now();
    thomas_brugnano(n, a, b, c, d, x, num_threads);
    auto solve_end = chrono::high_resolution_clock::now();
    double solve_time = chrono::duration<double>(solve_end - solve_start).count();
    
    // 验证
    auto verify_start = chrono::high_resolution_clock::now();
    double error = verify_solution(n, a, b, c, d, x);
    auto verify_end = chrono::high_resolution_clock::now();
    double verify_time = chrono::duration<double>(verify_end - verify_start).count();
    
    auto total_end = chrono::high_resolution_clock::now();
    double total_time = chrono::duration<double>(total_end - total_start).count();
    
    // 输出性能分析
    cout << fixed << setprecision(2);
    cout << "\n【性能分析】" << endl;
    cout << "  读取文件:   " << (io_time * 1000) << " ms" << endl;
    cout << "  求解算法:   " << (solve_time * 1000) << " ms" << endl;
    cout << "  验证结果:   " << (verify_time * 1000) << " ms" << endl;
    cout << "  其他开销:   " << ((total_time - io_time - solve_time - verify_time) * 1000) << " ms" << endl;
    cout << "  -------------------------" << endl;
    cout << "  总运行时间: " << (total_time * 1000) << " ms" << endl;
    cout << "\n最大残差: " << scientific << error << endl;
    cout << "=====================================================" << endl;
    
    return 0;
}
