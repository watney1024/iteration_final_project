#include "gauss_seidel_2d.h"
#include <cmath>
#include <algorithm>
#include <omp.h>

#ifdef __x86_64__
#include <emmintrin.h>  // SSE2 for _mm_prefetch
#endif

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

// 并行红黑 Gauss-Seidel（修复小规模性能问题）
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
    const double inv4 = 0.25;
    omp_set_num_threads(num_threads);
    
    // 关键修复：根据规模和线程数自适应tile_size
    // 目标：保证块数量 >> 线程数，才能有效并行
    int tile_size;
    if (N <= 64) {
        // 64: 需要至少16个块 (64/16=4, 4*4=16块)
        tile_size = (num_threads >= 4) ? 16 : 32;
    } else if (N <= 128) {
        // 128: 需要至少16-64个块
        tile_size = (num_threads >= 8) ? 16 : 32;
    } else if (N <= 256) {
        tile_size = 32;  // 256/32=8, 8*8=64个块
    } else if (N <= 512) {
        tile_size = 64;  // 512/64=8, 8*8=64个块
    } else {
        tile_size = 128; // 1024/128=8, 8*8=64个块
    }
    
    // 小规模问题更容易收敛，应该更频繁检查
    // 大规模问题收敛慢，减少检查频率
    int check_interval = 50;
    if (N >= 512) {
        check_interval = 200;  // 大规模问题减少检查
    } else if (N >= 256) {
        check_interval = 100;
    }
    
    iter_count = 0;
    
    #pragma omp parallel num_threads(num_threads)
    {
        // 线程局部变量
        double local_h2 = h2;
        double local_inv4 = inv4;
        
        for (int iter = 0; iter < max_iter; ++iter) {
            
            // 红点更新 - 使用简单的2D分块
            #pragma omp for schedule(static) collapse(2) nowait
            for (int bi = 1; bi <= N; bi += tile_size) {
                for (int bj = 1; bj <= N; bj += tile_size) {
                    int i_end = std::min(bi + tile_size, N + 1);
                    int j_end = std::min(bj + tile_size, N + 1);
                    
                    // 块内更新红点
                    for (int i = bi; i < i_end; ++i) {
                        // 计算j起始位置，确保(i+j)%2==0
                        int j_start = bj + ((i + bj) % 2 == 0 ? 0 : 1);
                        for (int j = j_start; j < j_end; j += 2) {
                            // 寄存器缓存
                            double u_im = U(i-1, j);
                            double u_ip = U(i+1, j);
                            double u_jm = U(i, j-1);
                            double u_jp = U(i, j+1);
                            double f_val = local_h2 * F(i-1, j-1);
                            
                            U(i, j) = local_inv4 * (u_im + u_ip + u_jm + u_jp + f_val);
                        }
                    }
                }
            }
            
            #pragma omp barrier
            
            // 黑点更新
            #pragma omp for schedule(static) collapse(2) nowait
            for (int bi = 1; bi <= N; bi += tile_size) {
                for (int bj = 1; bj <= N; bj += tile_size) {
                    int i_end = std::min(bi + tile_size, N + 1);
                    int j_end = std::min(bj + tile_size, N + 1);
                    
                    // 块内更新黑点
                    for (int i = bi; i < i_end; ++i) {
                        // 计算j起始位置，确保(i+j)%2==1
                        int j_start = bj + ((i + bj) % 2 == 1 ? 0 : 1);
                        for (int j = j_start; j < j_end; j += 2) {
                            double u_im = U(i-1, j);
                            double u_ip = U(i+1, j);
                            double u_jm = U(i, j-1);
                            double u_jp = U(i, j+1);
                            double f_val = local_h2 * F(i-1, j-1);
                            
                            U(i, j) = local_inv4 * (u_im + u_ip + u_jm + u_jp + f_val);
                        }
                    }
                }
            }
            
            #pragma omp barrier
            
            // 收敛检查
            if (iter % check_interval == check_interval - 1) {
                #pragma omp single
                {
                    residual = compute_residual(u, f, N, h);
                    if (residual < tol) {
                        iter_count = iter + 1;
                        max_iter = iter;
                    }
                }
            }
        }
    }
    
    if (iter_count == 0) {
        residual = compute_residual(u, f, N, h);
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
