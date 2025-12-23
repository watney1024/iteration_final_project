#include "gauss_seidel_3d.h"
#include <cmath>
#include <algorithm>
#include <omp.h>

// 辅助宏：访问三维数组 (N+2)x(N+2)x(N+2)
#define U(i, j, k) u[(i) * (N + 2) * (N + 2) + (j) * (N + 2) + (k)]
#define F(i, j, k) f[(i) * N * N + (j) * N + (k)]

// 串行普通 Gauss-Seidel
void GaussSeidel3D::solve_serial(
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
        // 按顺序扫描更新所有内部点
        for (int i = 1; i <= N; ++i) {
            for (int j = 1; j <= N; ++j) {
                for (int k = 1; k <= N; ++k) {
                    U(i, j, k) = (1.0 / 6.0) * (
                        U(i-1, j, k) + U(i+1, j, k) + 
                        U(i, j-1, k) + U(i, j+1, k) +
                        U(i, j, k-1) + U(i, j, k+1) +
                        h2 * F(i-1, j-1, k-1)
                    );
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

// 串行红黑 Gauss-Seidel
void GaussSeidel3D::solve_serial_redblack(
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
        // 红点更新：(i+j+k) % 2 == 0
        for (int i = 1; i <= N; ++i) {
            for (int j = 1; j <= N; ++j) {
                for (int k = 1; k <= N; ++k) {
                    if ((i + j + k) % 2 == 0) {
                        U(i, j, k) = (1.0 / 6.0) * (
                            U(i-1, j, k) + U(i+1, j, k) + 
                            U(i, j-1, k) + U(i, j+1, k) +
                            U(i, j, k-1) + U(i, j, k+1) +
                            h2 * F(i-1, j-1, k-1)
                        );
                    }
                }
            }
        }
        
        // 黑点更新：(i+j+k) % 2 == 1
        for (int i = 1; i <= N; ++i) {
            for (int j = 1; j <= N; ++j) {
                for (int k = 1; k <= N; ++k) {
                    if ((i + j + k) % 2 == 1) {
                        U(i, j, k) = (1.0 / 6.0) * (
                            U(i-1, j, k) + U(i+1, j, k) + 
                            U(i, j-1, k) + U(i, j+1, k) +
                            U(i, j, k-1) + U(i, j, k+1) +
                            h2 * F(i-1, j-1, k-1)
                        );
                    }
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
void GaussSeidel3D::solve_parallel_redblack(
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
    
    // 参考论文的分块策略
    // 对于512^3，使用较大的tile以获得更好的缓存局部性
    const int tile_size = 64;  // 与参考代码类似的tile大小
    
    for (int iter = 0; iter < max_iter; ++iter) {
        // 红点更新阶段
        #pragma omp parallel num_threads(num_threads)
        {
            #pragma omp for schedule(static) collapse(3) nowait
            for (int block_i = 1; block_i <= N; block_i += tile_size) {
                for (int block_j = 1; block_j <= N; block_j += tile_size) {
                    for (int block_k = 1; block_k <= N; block_k += tile_size) {
                        int i_end = std::min(block_i + tile_size, N + 1);
                        int j_end = std::min(block_j + tile_size, N + 1);
                        int k_end = std::min(block_k + tile_size, N + 1);
                        
                        // 在块内更新红点
                        for (int i = block_i; i < i_end; ++i) {
                            for (int j = block_j; j < j_end; ++j) {
                                for (int k = block_k; k < k_end; ++k) {
                                    if ((i + j + k) % 2 == 0) {
                                        U(i, j, k) = (1.0 / 6.0) * (
                                            U(i-1, j, k) + U(i+1, j, k) + 
                                            U(i, j-1, k) + U(i, j+1, k) +
                                            U(i, j, k-1) + U(i, j, k+1) +
                                            h2 * F(i-1, j-1, k-1)
                                        );
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        
        // 黑点更新阶段
        #pragma omp parallel num_threads(num_threads)
        {
            #pragma omp for schedule(static) collapse(3) nowait
            for (int block_i = 1; block_i <= N; block_i += tile_size) {
                for (int block_j = 1; block_j <= N; block_j += tile_size) {
                    for (int block_k = 1; block_k <= N; block_k += tile_size) {
                        int i_end = std::min(block_i + tile_size, N + 1);
                        int j_end = std::min(block_j + tile_size, N + 1);
                        int k_end = std::min(block_k + tile_size, N + 1);
                        
                        // 在块内更新黑点
                        for (int i = block_i; i < i_end; ++i) {
                            for (int j = block_j; j < j_end; ++j) {
                                for (int k = block_k; k < k_end; ++k) {
                                    if ((i + j + k) % 2 == 1) {
                                        U(i, j, k) = (1.0 / 6.0) * (
                                            U(i-1, j, k) + U(i+1, j, k) + 
                                            U(i, j-1, k) + U(i, j+1, k) +
                                            U(i, j, k-1) + U(i, j, k+1) +
                                            h2 * F(i-1, j-1, k-1)
                                        );
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        
        // 每隔几次迭代计算一次残差（减少开销）
        if (iter % 10 == 0) {
            residual = compute_residual(u, f, N, h);
            if (residual < tol) {
                iter_count = iter + 1;
                break;
            }
        }
        iter_count = iter + 1;
    }
    
    // 最终计算一次残差
    residual = compute_residual(u, f, N, h);
}

// 计算残差范数（L2范数）
double GaussSeidel3D::compute_residual(
    const std::vector<double>& u,
    const std::vector<double>& f,
    int N,
    double h
) {
    double h2 = h * h;
    double sum = 0.0;
    
    #pragma omp parallel for reduction(+:sum) collapse(3)
    for (int i = 1; i <= N; ++i) {
        for (int j = 1; j <= N; ++j) {
            for (int k = 1; k <= N; ++k) {
                // 计算残差：r = f - (-Δu)
                double laplacian = (U(i-1, j, k) + U(i+1, j, k) + 
                                   U(i, j-1, k) + U(i, j+1, k) +
                                   U(i, j, k-1) + U(i, j, k+1) - 
                                   6.0 * U(i, j, k)) / h2;
                double r = F(i-1, j-1, k-1) + laplacian;
                sum += r * r;
            }
        }
    }
    
    return std::sqrt(sum);
}

// 初始化测试问题：求解 -Δu = f
// 使用制造解：u_exact = sin(πx) * sin(πy) * sin(πz)
// 则 f = 3π² * sin(πx) * sin(πy) * sin(πz)
void GaussSeidel3D::init_test_problem(
    std::vector<double>& u,
    std::vector<double>& f,
    std::vector<double>& u_exact,
    int N,
    double h
) {
    const double PI = 3.14159265358979323846;
    
    // 初始化 u = 0（包括边界）
    u.assign((N + 2) * (N + 2) * (N + 2), 0.0);
    
    // 初始化 f 和 u_exact
    f.resize(N * N * N);
    u_exact.resize((N + 2) * (N + 2) * (N + 2), 0.0);
    
    #pragma omp parallel for collapse(3)
    for (int i = 1; i <= N; ++i) {
        for (int j = 1; j <= N; ++j) {
            for (int k = 1; k <= N; ++k) {
                double x = i * h;
                double y = j * h;
                double z = k * h;
                
                // 制造解
                double u_val = std::sin(PI * x) * std::sin(PI * y) * std::sin(PI * z);
                u_exact[i * (N + 2) * (N + 2) + j * (N + 2) + k] = u_val;
                
                // 右端项
                f[(i-1) * N * N + (j-1) * N + (k-1)] = 3.0 * PI * PI * u_val;
            }
        }
    }
}

#undef U
#undef F
