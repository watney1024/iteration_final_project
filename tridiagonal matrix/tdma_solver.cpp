#include "tdma_solver.h"
#include <cmath>
#include <algorithm>
#include <omp.h>

// 串行 Thomas 算法（追赶法）
// 经典的三对角矩阵求解算法，分为前向消元和回代两步
void TDMASolver::solve_thomas(
    const std::vector<double>& a,
    const std::vector<double>& b,
    const std::vector<double>& c,
    const std::vector<double>& d,
    std::vector<double>& x,
    int n
) {
    // 工作数组
    std::vector<double> c_prime(n);
    std::vector<double> d_prime(n);
    
    // 前向消元
    c_prime[0] = c[0] / b[0];
    d_prime[0] = d[0] / b[0];
    
    for (int i = 1; i < n; ++i) {
        double m = 1.0 / (b[i] - a[i] * c_prime[i-1]);
        c_prime[i] = c[i] * m;
        d_prime[i] = (d[i] - a[i] * d_prime[i-1]) * m;
    }
    
    // 回代
    x[n-1] = d_prime[n-1];
    for (int i = n - 2; i >= 0; --i) {
        x[i] = d_prime[i] - c_prime[i] * x[i+1];
    }
}

// 并行 PCR (Parallel Cyclic Reduction) 算法
// 通过递归消元，每次迭代消除一半的方程
// 适合并行计算，因为每个归约步骤中的所有更新都是独立的
void TDMASolver::solve_pcr(
    const std::vector<double>& a,
    const std::vector<double>& b,
    const std::vector<double>& c,
    const std::vector<double>& d,
    std::vector<double>& x,
    int n,
    int num_threads
) {
    omp_set_num_threads(num_threads);
    
    // 工作数组 - 需要两个副本用于乒乓缓冲
    std::vector<double> a_work[2];
    std::vector<double> b_work[2];
    std::vector<double> c_work[2];
    std::vector<double> d_work[2];
    
    a_work[0] = a;
    b_work[0] = b;
    c_work[0] = c;
    d_work[0] = d;
    
    a_work[1].resize(n);
    b_work[1].resize(n);
    c_work[1].resize(n);
    d_work[1].resize(n);
    
    // PCR 迭代次数 = log2(n)
    int num_levels = 0;
    int temp_n = n;
    while (temp_n > 1) {
        temp_n >>= 1;
        num_levels++;
    }
    
    int stride = 1;
    int current = 0;
    
    // PCR 前向归约阶段
    for (int level = 0; level < num_levels; ++level) {
        int next = 1 - current;
        
        #pragma omp parallel for schedule(static)
        for (int i = 0; i < n; ++i) {
            int i_left = i - stride;
            int i_right = i + stride;
            
            double alpha = 0.0, gamma = 0.0;
            
            // 计算归约系数，增加数值稳定性检查
            if (i_left >= 0 && std::abs(b_work[current][i_left]) > 1e-15) {
                alpha = -a_work[current][i] / b_work[current][i_left];
            }
            if (i_right < n && std::abs(b_work[current][i_right]) > 1e-15) {
                gamma = -c_work[current][i] / b_work[current][i_right];
            }
            
            // 更新系数
            if (i_left >= 0) {
                a_work[next][i] = alpha * a_work[current][i_left];
            } else {
                a_work[next][i] = 0.0;
            }
            
            if (i_right < n) {
                c_work[next][i] = gamma * c_work[current][i_right];
            } else {
                c_work[next][i] = 0.0;
            }
            
            b_work[next][i] = b_work[current][i];
            d_work[next][i] = d_work[current][i];
            
            if (i_left >= 0) {
                b_work[next][i] += alpha * c_work[current][i_left];
                d_work[next][i] += alpha * d_work[current][i_left];
            }
            if (i_right < n) {
                b_work[next][i] += gamma * a_work[current][i_right];
                d_work[next][i] += gamma * d_work[current][i_right];
            }
        }
        
        stride *= 2;
        current = next;
    }
    
    // 求解最终方程
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < n; ++i) {
        if (std::abs(b_work[current][i]) > 1e-10) {
            x[i] = d_work[current][i] / b_work[current][i];
        } else {
            x[i] = 0.0;
        }
    }
}

// 验证解的正确性
double TDMASolver::verify_solution(
    const std::vector<double>& a,
    const std::vector<double>& b,
    const std::vector<double>& c,
    const std::vector<double>& d,
    const std::vector<double>& x,
    int n
) {
    double residual = 0.0;
    
    #pragma omp parallel for reduction(+:residual)
    for (int i = 0; i < n; ++i) {
        double ax = 0.0;
        
        if (i > 0) {
            ax += a[i] * x[i-1];
        }
        ax += b[i] * x[i];
        if (i < n - 1) {
            ax += c[i] * x[i+1];
        }
        
        double r = d[i] - ax;
        residual += r * r;
    }
    
    return std::sqrt(residual);
}

// 生成测试用例 - 参考 jihoonakang/parallel_tdma_cpp 的设置
void TDMASolver::generate_test_system(
    std::vector<double>& a,
    std::vector<double>& b,
    std::vector<double>& c,
    std::vector<double>& d,
    int n
) {
    a.resize(n);
    b.resize(n);
    c.resize(n);
    d.resize(n);
    
    // 生成对角占优的三对角矩阵
    // 参考 jihoonakang repo 中的系数设置
    for (int i = 0; i < n; ++i) {
        a[i] = 1.0 + 0.01 * i;
        c[i] = 1.0 + 0.02 * i;
        b[i] = -(a[i] + c[i]) - 0.1 - 0.02 * i * i;
        d[i] = static_cast<double>(i);
    }
    
    // 边界处理
    a[0] = 0.0;      // 第一行没有下对角元素
    c[n-1] = 0.0;    // 最后一行没有上对角元素
}
