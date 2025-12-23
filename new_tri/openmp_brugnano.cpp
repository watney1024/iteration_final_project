/**
 * @file openmp_brugnano.cpp
 * @brief 基于 OpenMP 的 Brugnano 并行 Thomas 算法
 * @details 参考 tanim72/15418-final-project
 *          使用局部修改的 Thomas 算法 + 规约系统求解
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

/**
 * @brief 局部修改的 Thomas 算法
 * @details 对局部三对角系统进行特殊的前向消元和回代
 *          保留第一行和最后一行的信息用于构建规约系统
 */
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
    
    // 归一化前两行
    d[0] = d[0] / b[0];
    c[0] = c[0] / b[0];
    a[0] = a[0] / b[0];
    
    if (m > 1) {
        d[1] = d[1] / b[1];
        c[1] = c[1] / b[1];
        a[1] = a[1] / b[1];
    }
    
    // 前向消元
    for (int i = 2; i < m; i++) {
        double denom = b[i] - a[i] * c[i-1];
        if (abs(denom) < 1e-10) denom = 1e-10;
        double r = 1.0 / denom;
        d[i] = r * (d[i] - a[i] * d[i-1]);
        c[i] = r * c[i];
        a[i] = -r * (a[i] * a[i-1]);
    }
    
    // 回代（保留边界）
    for (int i = m - 3; i >= 1; i--) {
        d[i] = d[i] - c[i] * d[i+1];
        c[i] = -c[i] * c[i+1];
        a[i] = a[i] - c[i] * a[i+1];
    }
    
    // 最终处理第一行
    if (m >= 2) {
        double r = 1.0 / (1.0 - a[1] * c[0]);
        d[0] = r * (d[0] - a[0] * d[1]);
        c[0] = -r * c[0] * c[1];
        a[0] = r * a[0];
    }
}

/**
 * @brief 标准 Thomas 算法求解规约系统
 */
void standard_thomas_solver(int size,
                           vector<double>& a,
                           vector<double>& b,
                           vector<double>& c,
                           vector<double>& d) {
    vector<double> gamma(size, 0.0);
    
    // 前向消元
    gamma[0] = c[0] / b[0];
    d[0] = d[0] / b[0];
    
    for (int i = 1; i < size; i++) {
        double denom = b[i] - a[i] * gamma[i-1];
        if (i < size - 1) {
            gamma[i] = c[i] / denom;
        }
        d[i] = (d[i] - a[i] * d[i-1]) / denom;
    }
    
    // 回代
    for (int i = size - 2; i >= 0; i--) {
        d[i] = d[i] - gamma[i] * d[i+1];
    }
}

/**
 * @brief OpenMP Brugnano 并行 Thomas 算法
 */
void thomas_brugnano(int n,
                    const vector<double>& global_a,
                    const vector<double>& global_b,
                    const vector<double>& global_c,
                    const vector<double>& global_d,
                    vector<double>& global_x,
                    int num_threads) {
    
    // 计算每个线程的分块大小
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
    
    // 存储每个分块的边界系数
    vector<vector<double>> all_coefs(num_threads, vector<double>(6));
    
    #pragma omp parallel num_threads(num_threads)
    {
        int tid = omp_get_thread_num();
        int start_idx = start_indices[tid];
        int m = chunk_sizes[tid];
        
        // 复制局部数据
        vector<double> local_a(m), local_b(m), local_c(m), local_d(m);
        for (int i = 0; i < m; i++) {
            int gi = start_idx + i;
            local_a[i] = global_a[gi];
            local_b[i] = global_b[gi];
            local_c[i] = global_c[gi];
            local_d[i] = global_d[gi];
        }
        
        // 应用修改的 Thomas 算法
        if (m == 1) {
            local_d[0] /= local_b[0];
            local_a[0] = local_c[0] = 0.0;
        } else {
            modified_thomas_algorithm(m, local_a, local_b, local_c, local_d);
        }
        
        // 存储边界系数 (第一行和最后一行)
        all_coefs[tid][0] = local_a[0];
        all_coefs[tid][1] = local_c[0];
        all_coefs[tid][2] = local_d[0];
        all_coefs[tid][3] = local_a[m-1];
        all_coefs[tid][4] = local_c[m-1];
        all_coefs[tid][5] = local_d[m-1];
        
        #pragma omp barrier
        
        // 主线程构建并求解规约系统
        #pragma omp single
        {
            int R = 2 * num_threads;
            vector<double> ra(R, 0.0), rb(R, 1.0), rc(R, 0.0), rd(R);
            
            // 构建规约系统
            for (int i = 0; i < num_threads; i++) {
                int e1 = 2 * i;      // 分块起始
                int e2 = 2 * i + 1;  // 分块结束
                
                ra[e1] = all_coefs[i][0];
                rc[e1] = all_coefs[i][1];
                rd[e1] = all_coefs[i][2];
                ra[e2] = all_coefs[i][3];
                rc[e2] = all_coefs[i][4];
                rd[e2] = all_coefs[i][5];
            }
            
            // 连接相邻分块的边界
            // 每个分块结束节点的 c 系数应该连到下一个分块的开始
            for (int i = 0; i < num_threads - 1; i++) {
                int curr_end = 2 * i + 1;      // 当前分块的结束
                int next_start = 2 * (i + 1);   // 下一个分块的开始
                // 它们之间需要连接
                rc[curr_end] = -ra[next_start];
                ra[next_start] = -rc[curr_end];
            }
            
            // 求解规约系统
            standard_thomas_solver(R, ra, rb, rc, rd);
            
            // 将边界值放回全局解
            for (int i = 0; i < num_threads; i++) {
                int s = start_indices[i];
                int e = s + chunk_sizes[i] - 1;
                global_x[s] = rd[2*i];
                global_x[e] = rd[2*i+1];
            }
        }
        
        #pragma omp barrier
        
        // 更新内部节点
        int s = start_idx;
        double d0 = global_x[s];
        double dN = global_x[s + m - 1];
        
        for (int i = 1; i < m - 1; i++) {
            double local_d_i = local_d[i];
            double local_a_i = local_a[i];
            double local_c_i = local_c[i];
            global_x[s + i] = local_d_i - local_a_i * d0 - local_c_i * dN;
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
    
    cout << "=====================================================" << endl;
    cout << "OpenMP Brugnano 并行 Thomas 算法" << endl;
    cout << "=====================================================" << endl;
    cout << "问题规模: N = " << n << endl;
    cout << "-----------------------------------------------------" << endl;
    
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
        thomas_brugnano(n, a, b, c, d, x, nt);
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
    
    cout << "-----------------------------------------------------" << endl;
    
    return 0;
}
