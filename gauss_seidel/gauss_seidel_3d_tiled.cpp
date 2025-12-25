#include "gauss_seidel_3d.h"
#include <cmath>
#include <algorithm>
#include <omp.h>
#include <immintrin.h>

// 2层分块优化的3D Gauss-Seidel求解器
// 优化并行策略：collapse(3)增加并行粒度

#define U(i, j, k) u[(i) * (N + 2) * (N + 2) + (j) * (N + 2) + (k)]
#define F(i, j, k) f[(i) * N * N + (j) * N + (k)]

namespace GaussSeidel3DTiled {

// 2层分块并行红黑Gauss-Seidel (3D版本) - 性能优化版
void solve_4level_tiling(
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
    const double inv6 = 1.0 / 6.0;  // 预计算常量
    omp_set_num_threads(num_threads);
    
    // 关键平衡：tile_size需要在缓存局部性和并行度之间权衡
    // - 太大：并行度不足（块数太少）
    // - 太小：缓存局部性差（频繁cache miss）
    // 策略：根据线程数动态调整
    int TILE_SIZE;
    if (N <= 32) {
        TILE_SIZE = 16;   // 32^3: (32/16)^3 = 8个块
    } else if (N <= 64) {
        TILE_SIZE = 32;   // 64^3: (64/32)^3 = 8个块
    } else if (N <= 128) {
        // 128^3: 根据线程数调整
        if (num_threads >= 8) {
            TILE_SIZE = 32;   // (128/32)^3 = 64个块，充足的并行度
        } else if (num_threads >= 4) {
            TILE_SIZE = 43;   // (128/43)^3 ≈ 27个块，平衡点
        } else {
            TILE_SIZE = 64;   // (128/64)^3 = 8个块，更好的缓存局部性
        }
    } else if (N <= 256) {
        TILE_SIZE = 64;   // 256^3: (256/64)^3 = 64个块
    } else {
        TILE_SIZE = 128;  // 512^3: (512/128)^3 = 64个块
    }
    
    // 大幅增加check_interval，减少残差计算的同步开销
    int check_interval = 100;
    if (N >= 128) {
        check_interval = 500;  // 大规模问题大幅减少检查频率
    }
    
    iter_count = 0;
    
    #pragma omp parallel num_threads(num_threads)
    {
        // 线程局部变量：避免重复计算和false sharing
        double local_h2 = h2;
        double local_inv6 = inv6;
        
        for (int iter = 0; iter < max_iter; ++iter) {
            
            // ========== 红点更新 (i+j+k)%2==0 ==========
            // 使用静态调度减少调度开销，块数足够多时static更高效
            #pragma omp for schedule(static) collapse(3) nowait
            for (int block_i = 1; block_i <= N; block_i += TILE_SIZE) {
                for (int block_j = 1; block_j <= N; block_j += TILE_SIZE) {
                    for (int block_k = 1; block_k <= N; block_k += TILE_SIZE) {
                        int i_end = std::min(block_i + TILE_SIZE, N + 1);
                        int j_end = std::min(block_j + TILE_SIZE, N + 1);
                        int k_end = std::min(block_k + TILE_SIZE, N + 1);
                        
                        // ========== 关键优化：消除条件分支 ==========
                        // 在块内更新红点 - k循环直接跳2步
                        for (int i = block_i; i < i_end; ++i) {
                            for (int j = block_j; j < j_end; ++j) {
                                // 计算k的起始值，确保(i+j+k)%2==0
                                int k_start = block_k + ((i + j + block_k) % 2 == 0 ? 0 : 1);
                                // k每次跳2，只访问红点，无需条件判断
                                for (int k = k_start; k < k_end; k += 2) {
                                    // 寄存器缓存：减少内存访问
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
            
            #pragma omp barrier
            
            // ========== 黑点更新 (i+j+k)%2==1 ==========
            #pragma omp for schedule(static) collapse(3) nowait
            for (int block_i = 1; block_i <= N; block_i += TILE_SIZE) {
                for (int block_j = 1; block_j <= N; block_j += TILE_SIZE) {
                    for (int block_k = 1; block_k <= N; block_k += TILE_SIZE) {
                        int i_end = std::min(block_i + TILE_SIZE, N + 1);
                        int j_end = std::min(block_j + TILE_SIZE, N + 1);
                        int k_end = std::min(block_k + TILE_SIZE, N + 1);
                        
                        // 在块内更新黑点 - 同样消除条件分支
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
            
            #pragma omp barrier
            
            // 大幅减少残差检查频率
            if (iter % check_interval == check_interval - 1) {
                #pragma omp single
                {
                    residual = GaussSeidel3D::compute_residual(u, f, N, h);
                    if (residual < tol) {
                        iter_count = iter + 1;
                        max_iter = iter;
                    }
                }
            }
        }
    }
    
    if (iter_count == 0) {
        residual = GaussSeidel3D::compute_residual(u, f, N, h);
        iter_count = max_iter;
    }
}

} // namespace GaussSeidel3DTiled

#undef U
#undef F
