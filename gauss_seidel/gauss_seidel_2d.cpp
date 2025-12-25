#include "gauss_seidel_2d.h"
#include <cmath>
#include <algorithm>
#include <omp.h>

// 辅助宏：访问二维数组 (N+2)x(N+2)
#define U(i, j) u[(i) * (N + 2) + (j)]
#define F(i, j) f[(i) * N + (j)]

// 串行普通 Gauss-Seidel
void GaussSeidel2D::solve_serial(
    std::vector<double>& u,
    const std::vector<double>& f,
    int N,
    double h,
    int max_iter,
    double tol,
    int& iter_count,
    double& residual
) {
    double h2 = h * h;
    
    for (int iter = 0; iter < max_iter; ++iter) {
        // 按行扫描更新所有内部点
        for (int i = 1; i <= N; ++i) {
            for (int j = 1; j <= N; ++j) {
                U(i, j) = 0.25 * (U(i-1, j) + U(i+1, j) + 
                                  U(i, j-1) + U(i, j+1) + 
                                  h2 * F(i-1, j-1));
            }
        }
        
        // 计算残差
        residual = compute_residual(u, f, N, h);
        iter_count = iter + 1;
        
        if (residual < tol) {
            break;
        }
    }
}

// 串行红黑 Gauss-Seidel
void GaussSeidel2D::solve_serial_redblack(
    std::vector<double>& u,
    const std::vector<double>& f,
    int N,
    double h,
    int max_iter,
    double tol,
    int& iter_count,
    double& residual
) {
    double h2 = h * h;
    
    for (int iter = 0; iter < max_iter; ++iter) {
        // 红点更新：(i+j) % 2 == 0
        for (int i = 1; i <= N; ++i) {
            for (int j = 1; j <= N; ++j) {
                if ((i + j) % 2 == 0) {
                    U(i, j) = 0.25 * (U(i-1, j) + U(i+1, j) + 
                                      U(i, j-1) + U(i, j+1) + 
                                      h2 * F(i-1, j-1));
                }
            }
        }
        
        // 黑点更新：(i+j) % 2 == 1
        for (int i = 1; i <= N; ++i) {
            for (int j = 1; j <= N; ++j) {
                if ((i + j) % 2 == 1) {
                    U(i, j) = 0.25 * (U(i-1, j) + U(i+1, j) + 
                                      U(i, j-1) + U(i, j+1) + 
                                      h2 * F(i-1, j-1));
                }
            }
        }
        
        // 计算残差
        residual = compute_residual(u, f, N, h);
        iter_count = iter + 1;
        
        if (residual < tol) {
            break;
        }
    }
}

// 并行红黑 Gauss-Seidel（优化版本）
void GaussSeidel2D::solve_parallel_redblack(
    std::vector<double>& u,
    const std::vector<double>& f,
    int N,
    double h,
    int max_iter,
    double tol,
    int& iter_count,
    double& residual,
    int num_threads
) {
    double h2 = h * h;
    omp_set_num_threads(num_threads);
    
    // 关键优化点：
    // 1. 红黑点分开循环避免分支预测失败
    // 2. 静态调度减少线程管理开销  
    // 3. 定期检查残差避免过度同步
    // 4. 使用firstprivate减少共享变量访问
    
    const int check_interval = 10;  // 每10次迭代检查一次（提高检查频率）
    
    // 关键优化：将parallel region移到循环外，避免重复创建线程
    #pragma omp parallel num_threads(num_threads) firstprivate(h2)
    {
        for (int iter = 0; iter < max_iter; ++iter) {
            // 红点更新 - nowait避免隐式栅障
            #pragma omp for schedule(static) nowait
            for (int i = 1; i <= N; ++i) {
                // 根据行号决定起始列，避免if (i+j)%2判断
                int j_start = (i % 2 == 1) ? 1 : 2;
                for (int j = j_start; j <= N; j += 2) {
                    U(i, j) = 0.25 * (U(i-1, j) + U(i+1, j) + 
                                      U(i, j-1) + U(i, j+1) + 
                                      h2 * F(i-1, j-1));
                }
            }
            
            // 显式栅障：确保红点全部更新完
            #pragma omp barrier
            
            // 黑点更新 - nowait避免隐式栅障
            #pragma omp for schedule(static) nowait
            for (int i = 1; i <= N; ++i) {
                int j_start = (i % 2 == 1) ? 2 : 1;
                for (int j = j_start; j <= N; j += 2) {
                    U(i, j) = 0.25 * (U(i-1, j) + U(i+1, j) + 
                                      U(i, j-1) + U(i, j+1) + 
                                      h2 * F(i-1, j-1));
                }
            }
            
            // 显式栅障：确保黑点全部更新完
            #pragma omp barrier
            
            // 只在master线程检查收敛
            #pragma omp master
            {
                if (iter % check_interval == check_interval - 1) {
                    residual = compute_residual(u, f, N, h);
                    if (residual < tol) {
                        iter_count = iter + 1;
                        max_iter = iter;  // 提前终止所有线程
                    }
                }
            }
            // 确保所有线程同步max_iter的更新
            #pragma omp barrier
        }
    }
    
    // 最终计算残差（注意：只在未break时设置iter_count）
    residual = compute_residual(u, f, N, h);
    if (iter_count == 0) {  // 未break，说明跑完所有迭代
        iter_count = max_iter;
    }
}

// 计算残差范数（L2范数）
double GaussSeidel2D::compute_residual(
    const std::vector<double>& u,
    const std::vector<double>& f,
    int N,
    double h
) {
    double h2 = h * h;
    double sum = 0.0;
    
    #pragma omp parallel for reduction(+:sum)
    for (int i = 1; i <= N; ++i) {
        for (int j = 1; j <= N; ++j) {
            // 计算残差：r = f - (-Δu)
            double laplacian = (U(i-1, j) + U(i+1, j) + 
                               U(i, j-1) + U(i, j+1) - 
                               4.0 * U(i, j)) / h2;
            double r = F(i-1, j-1) + laplacian;
            sum += r * r;
        }
    }
    
    return std::sqrt(sum);
}

// 初始化测试问题：求解 -Δu = f
// 使用制造解：u_exact = sin(πx) * sin(πy)
// 则 f = 2π² * sin(πx) * sin(πy)
void GaussSeidel2D::init_test_problem(
    std::vector<double>& u,
    std::vector<double>& f,
    std::vector<double>& u_exact,
    int N,
    double h
) {
    const double PI = 3.14159265358979323846;
    
    // 初始化 u = 0（包括边界）
    u.assign((N + 2) * (N + 2), 0.0);
    
    // 初始化 f 和 u_exact
    f.resize(N * N);
    u_exact.resize((N + 2) * (N + 2), 0.0);
    
    for (int i = 1; i <= N; ++i) {
        for (int j = 1; j <= N; ++j) {
            double x = i * h;
            double y = j * h;
            
            // 制造解
            double u_val = std::sin(PI * x) * std::sin(PI * y);
            u_exact[i * (N + 2) + j] = u_val;
            
            // 右端项
            f[(i-1) * N + (j-1)] = 2.0 * PI * PI * u_val;
        }
    }
}

#undef U
#undef F
