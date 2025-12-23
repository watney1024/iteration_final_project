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

// 并行红黑 Gauss-Seidel（基于区域分解 + 多级分块）
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
    
    // 分块策略：将区域划分为多个块
    // 参考论文：每个线程负责一个或多个块
    // 这里采用二级分块：
    //   - 第一级：将整个区域分成 num_threads 个大块（按行分）
    //   - 第二级：每个大块内部再分成小块以提高缓存效率
    
    const int tile_size = 32;  // 增大块大小，减少调度次数
    
    for (int iter = 0; iter < max_iter; ++iter) {
        // 红点更新阶段
        #pragma omp parallel num_threads(num_threads)
        {
            // 按块迭代，每个线程处理多个块
            #pragma omp for schedule(static) nowait
            for (int block_i = 1; block_i <= N; block_i += tile_size) {
                int i_end = std::min(block_i + tile_size, N + 1);
                for (int block_j = 1; block_j <= N; block_j += tile_size) {
                    int j_end = std::min(block_j + tile_size, N + 1);
                    
                    // 在块内更新红点
                    for (int i = block_i; i < i_end; ++i) {
                        for (int j = block_j; j < j_end; ++j) {
                            if ((i + j) % 2 == 0) {
                                U(i, j) = 0.25 * (U(i-1, j) + U(i+1, j) + 
                                                  U(i, j-1) + U(i, j+1) + 
                                                  h2 * F(i-1, j-1));
                            }
                        }
                    }
                }
            }
        }
        
        // 黑点更新阶段
        #pragma omp parallel num_threads(num_threads)
        {
            #pragma omp for schedule(static) nowait
            for (int block_i = 1; block_i <= N; block_i += tile_size) {
                int i_end = std::min(block_i + tile_size, N + 1);
                for (int block_j = 1; block_j <= N; block_j += tile_size) {
                    int j_end = std::min(block_j + tile_size, N + 1);
                    
                    // 在块内更新黑点
                    for (int i = block_i; i < i_end; ++i) {
                        for (int j = block_j; j < j_end; ++j) {
                            if ((i + j) % 2 == 1) {
                                U(i, j) = 0.25 * (U(i-1, j) + U(i+1, j) + 
                                                  U(i, j-1) + U(i, j+1) + 
                                                  h2 * F(i-1, j-1));
                            }
                        }
                    }
                }
            }
        }
        
        // 计算残差（并行化）
        residual = compute_residual(u, f, N, h);
        iter_count = iter + 1;
        
        if (residual < tol) {
            break;
        }
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
