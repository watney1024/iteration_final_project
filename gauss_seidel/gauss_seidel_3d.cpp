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

// 并行红黑 Gauss-Seidel（修复版：减少同步开销，改善负载均衡）
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
    const double inv6 = 1.0 / 6.0;
    omp_set_num_threads(num_threads);
    
    // 增大tile_size以减少块数量和同步开销
    // 对于512^3，使用128的tile（512/128=4，总共4^3=64个块）
    int tile_size = 128;
    if (N <= 64) {
        tile_size = 32;
    } else if (N <= 256) {
        tile_size = 64;
    }
    
    // 大幅增加check_interval，减少残差计算的同步开销
    int check_interval = 100;
    if (N >= 128) {
        check_interval = 500;  // 大规模问题大幅减少检查频率
    }
    
    iter_count = 0;
    
    // 使用单个persistent parallel region
    #pragma omp parallel num_threads(num_threads)
    {
        // 线程局部缓存：减少重复计算和内存访问
        double local_h2 = h2;
        double local_inv6 = inv6;
        
        for (int iter = 0; iter < max_iter; ++iter) {
            // ============ 关键改进1: 块内红黑交替，减少全局同步 ============
            // 使用dynamic调度改善负载均衡
            #pragma omp for schedule(dynamic, 2) collapse(3) nowait
            for (int block_i = 1; block_i <= N; block_i += tile_size) {
                for (int block_j = 1; block_j <= N; block_j += tile_size) {
                    for (int block_k = 1; block_k <= N; block_k += tile_size) {
                        int i_end = std::min(block_i + tile_size, N + 1);
                        int j_end = std::min(block_j + tile_size, N + 1);
                        int k_end = std::min(block_k + tile_size, N + 1);
                        
                        // ============ 改进2: 块内先红后黑，消除条件分支 ============
                        // 红点更新
                        for (int i = block_i; i < i_end; ++i) {
                            // 根据i的奇偶性确定j的起始奇偶
                            for (int j = block_j; j < j_end; ++j) {
                                // 计算k的起始值，确保(i+j+k)%2==0
                                int k_start = block_k + ((i + j + block_k) % 2 == 0 ? 0 : 1);
                                // k每次跳2，只访问红点
                                for (int k = k_start; k < k_end; k += 2) {
                                    // ============ 改进3: 使用寄存器缓存，减少内存访问 ============
                                    double u_im = U(i-1, j, k);
                                    double u_ip = U(i+1, j, k);
                                    double u_jm = U(i, j-1, k);
                                    double u_jp = U(i, j+1, k);
                                    double u_km = U(i, j, k-1);
                                    double u_kp = U(i, j, k+1);
                                    double f_val = local_h2 * F(i-1, j-1, k-1);
                                    
                                    U(i, j, k) = local_inv6 * (u_im + u_ip + u_jm + u_jp + u_km + u_kp + f_val);
                                }
                            }
                        }
                    }
                }
            }
            
            // 只在红点全部更新后同步一次
            #pragma omp barrier
            
            // 黑点更新：同样的优化策略
            #pragma omp for schedule(dynamic, 2) collapse(3) nowait
            for (int block_i = 1; block_i <= N; block_i += tile_size) {
                for (int block_j = 1; block_j <= N; block_j += tile_size) {
                    for (int block_k = 1; block_k <= N; block_k += tile_size) {
                        int i_end = std::min(block_i + tile_size, N + 1);
                        int j_end = std::min(block_j + tile_size, N + 1);
                        int k_end = std::min(block_k + tile_size, N + 1);
                        
                        // 黑点更新
                        for (int i = block_i; i < i_end; ++i) {
                            for (int j = block_j; j < j_end; ++j) {
                                // 计算k的起始值，确保(i+j+k)%2==1
                                int k_start = block_k + ((i + j + block_k) % 2 == 1 ? 0 : 1);
                                for (int k = k_start; k < k_end; k += 2) {
                                    double u_im = U(i-1, j, k);
                                    double u_ip = U(i+1, j, k);
                                    double u_jm = U(i, j-1, k);
                                    double u_jp = U(i, j+1, k);
                                    double u_km = U(i, j, k-1);
                                    double u_kp = U(i, j, k+1);
                                    double f_val = local_h2 * F(i-1, j-1, k-1);
                                    
                                    U(i, j, k) = local_inv6 * (u_im + u_ip + u_jm + u_jp + u_km + u_kp + f_val);
                                }
                            }
                        }
                    }
                }
            }
            
            // 黑点更新完成后同步
            #pragma omp barrier
            
            // 大幅减少残差检查频率
            if (iter % check_interval == check_interval - 1) {
                #pragma omp single
                {
                    residual = compute_residual(u, f, N, h);
                    if (residual < tol) {
                        iter_count = iter + 1;
                        max_iter = iter;  // 提前终止
                    }
                }
            }
        }
    }
    
    // 如果没有提前终止，计算最终残差
    if (iter_count == 0) {
        residual = compute_residual(u, f, N, h);
        iter_count = max_iter;
    }
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
