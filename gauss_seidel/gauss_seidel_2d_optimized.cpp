#include "gauss_seidel_2d.h"
#include <cmath>
#include <algorithm>
#include <omp.h>
#include <immintrin.h>  // AVX2
#include <cstring>

// 辅助宏：访问二维数组 (N+2)x(N+2)
#define U(i, j) u[(i) * (N + 2) + (j)]
#define F(i, j) f[(i) * N + (j)]

namespace GaussSeidel2DOptimized {

// ========== 访存优化版本1: 二级Tiling + 数据重用 ==========
void solve_parallel_redblack_tiled(
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
    
    // 自适应Tile大小
    // L1缓存优化：32KB L1可存 4096个double，考虑5点模板需要5行数据
    // L2缓存优化：256KB L2可存更大的块
    int tile_l2 = 128;  // L2缓存块
    int tile_l1 = 16;   // L1缓存块
    
    // 根据问题规模自适应调整
    if (N <= 256) {
        tile_l2 = 64;
        tile_l1 = 16;
    } else if (N >= 2048) {
        tile_l2 = 256;
        tile_l1 = 32;
    }
    
    const int check_interval = 100;
    
    for (int iter = 0; iter < max_iter; ++iter) {
        // ========== 红点更新：二级分块 ==========
        #pragma omp parallel num_threads(num_threads) firstprivate(h2)
        {
            // L2级分块 - 跨线程的大块
            #pragma omp for schedule(static) nowait
            for (int bi = 1; bi <= N; bi += tile_l2) {
                for (int bj = 1; bj <= N; bj += tile_l2) {
                    int bi_end = std::min(bi + tile_l2, N + 1);
                    int bj_end = std::min(bj + tile_l2, N + 1);
                    
                    // L1级分块 - 单线程内的小块
                    for (int ti = bi; ti < bi_end; ti += tile_l1) {
                        int ti_end = std::min(ti + tile_l1, bi_end);
                        
                        for (int tj = bj; tj < bj_end; tj += tile_l1) {
                            int tj_end = std::min(tj + tile_l1, bj_end);
                            
                            // 预取下一块数据到L1缓存
                            if (tj + tile_l1 < bj_end) {
                                _mm_prefetch((const char*)&U(ti, tj + tile_l1), _MM_HINT_T0);
                            }
                            
                            // 内核计算 - 红点
                            for (int i = ti; i < ti_end; ++i) {
                                // 预取下一行
                                if (i + 1 < ti_end) {
                                    _mm_prefetch((const char*)&U(i + 1, tj), _MM_HINT_T0);
                                }
                                
                                // 计算红点的起始j（确保(i+j)%2==0）
                                int j_start = ((i % 2 == 1) ? 1 : 2);
                                // 调整到tile范围内，保持奇偶性
                                while (j_start < tj) j_start += 2;
                                
                                for (int j = j_start; j < tj_end && j <= N; j += 2) {
                                    // 显式加载数据以提高编译器优化
                                    double u_im1_j = U(i-1, j);
                                    double u_ip1_j = U(i+1, j);
                                    double u_i_jm1 = U(i, j-1);
                                    double u_i_jp1 = U(i, j+1);
                                    double f_val = h2 * F(i-1, j-1);
                                    
                                    U(i, j) = 0.25 * (u_im1_j + u_ip1_j + 
                                                      u_i_jm1 + u_i_jp1 + f_val);
                                }
                            }
                        }
                    }
                }
            }
        }
        
        // ========== 黑点更新：相同的分块策略 ==========
        #pragma omp parallel num_threads(num_threads) firstprivate(h2)
        {
            #pragma omp for schedule(static) nowait
            for (int bi = 1; bi <= N; bi += tile_l2) {
                for (int bj = 1; bj <= N; bj += tile_l2) {
                    int bi_end = std::min(bi + tile_l2, N + 1);
                    int bj_end = std::min(bj + tile_l2, N + 1);
                    
                    for (int ti = bi; ti < bi_end; ti += tile_l1) {
                        int ti_end = std::min(ti + tile_l1, bi_end);
                        
                        for (int tj = bj; tj < bj_end; tj += tile_l1) {
                            int tj_end = std::min(tj + tile_l1, bj_end);
                            
                            if (tj + tile_l1 < bj_end) {
                                _mm_prefetch((const char*)&U(ti, tj + tile_l1), _MM_HINT_T0);
                            }
                            
                            for (int i = ti; i < ti_end; ++i) {
                                if (i + 1 < ti_end) {
                                    _mm_prefetch((const char*)&U(i + 1, tj), _MM_HINT_T0);
                                }
                                
                                // 计算黑点的起始j（确保(i+j)%2==1）
                                int j_start = ((i % 2 == 1) ? 2 : 1);
                                // 调整到tile范围内，保持奇偶性
                                while (j_start < tj) j_start += 2;
                                
                                for (int j = j_start; j < tj_end && j <= N; j += 2) {
                                    double u_im1_j = U(i-1, j);
                                    double u_ip1_j = U(i+1, j);
                                    double u_i_jm1 = U(i, j-1);
                                    double u_i_jp1 = U(i, j+1);
                                    double f_val = h2 * F(i-1, j-1);
                                    
                                    U(i, j) = 0.25 * (u_im1_j + u_ip1_j + 
                                                      u_i_jm1 + u_i_jp1 + f_val);
                                }
                            }
                        }
                    }
                }
            }
        }
        
        if (iter % check_interval == check_interval - 1) {
            residual = GaussSeidel2D::compute_residual(u, f, N, h);
            if (residual < tol) {
                iter_count = iter + 1;
                break;
            }
        }
    }
    
    residual = GaussSeidel2D::compute_residual(u, f, N, h);
    if (iter_count == 0) {  // 未break，说明跑完所有迭代
        iter_count = max_iter;
    }
}


// ========== 访存优化版本2: SIMD向量化（实验性）==========
// 注意：由于红黑排序的跨步访问，SIMD效果有限
void solve_parallel_redblack_simd(
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
    const int check_interval = 100;
    
    __m256d vec_025 = _mm256_set1_pd(0.25);
    __m256d vec_h2 = _mm256_set1_pd(h2);
    
    for (int iter = 0; iter < max_iter; ++iter) {
        // 红点更新 - 尝试向量化
        #pragma omp parallel for schedule(static) num_threads(num_threads)
        for (int i = 1; i <= N; ++i) {
            int j_start = (i % 2 == 1) ? 1 : 2;
            
            // SIMD处理：每次处理4个红点（间隔为2）
            int j;
            for (j = j_start; j + 6 <= N; j += 8) {
                // 加载数据：由于跨步访问，需要gather
                // 这里简化为标量处理，完整SIMD需要_mm256_i64gather_pd
                for (int jj = j; jj < j + 8 && jj <= N; jj += 2) {
                    U(i, jj) = 0.25 * (U(i-1, jj) + U(i+1, jj) + 
                                       U(i, jj-1) + U(i, jj+1) + 
                                       h2 * F(i-1, jj-1));
                }
            }
            
            // 处理剩余元素
            for (; j <= N; j += 2) {
                U(i, j) = 0.25 * (U(i-1, j) + U(i+1, j) + 
                                  U(i, j-1) + U(i, j+1) + 
                                  h2 * F(i-1, j-1));
            }
        }
        
        // 黑点更新
        #pragma omp parallel for schedule(static) num_threads(num_threads)
        for (int i = 1; i <= N; ++i) {
            int j_start = (i % 2 == 1) ? 2 : 1;
            for (int j = j_start; j <= N; j += 2) {
                U(i, j) = 0.25 * (U(i-1, j) + U(i+1, j) + 
                                  U(i, j-1) + U(i, j+1) + 
                                  h2 * F(i-1, j-1));
            }
        }
        
        if (iter % check_interval == check_interval - 1) {
            residual = GaussSeidel2D::compute_residual(u, f, N, h);
            if (residual < tol) {
                iter_count = iter + 1;
                break;
            }
        }
    }
    
    residual = GaussSeidel2D::compute_residual(u, f, N, h);
    if (iter_count == 0) {  // 未break，说明跑完所有迭代
        iter_count = max_iter;
    }
}


// ========== 访存优化版本3: 数据重排 + 连续存储红黑点 ==========
// 思路：将红黑点分别连续存储，完全消除跨步访问
void solve_parallel_redblack_restructured(
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
    
    // 分配红黑点的连续存储
    int n_red = (N * N + 1) / 2;
    int n_black = N * N - n_red;
    std::vector<double> u_red(n_red);
    std::vector<double> u_black(n_black);
    std::vector<int> red_map(n_red * 2);   // 存储(i,j)映射
    std::vector<int> black_map(n_black * 2);
    
    // 构建映射表（只需一次）
    int r_idx = 0, b_idx = 0;
    for (int i = 1; i <= N; ++i) {
        for (int j = 1; j <= N; ++j) {
            if ((i + j) % 2 == 0) {
                red_map[r_idx * 2] = i;
                red_map[r_idx * 2 + 1] = j;
                u_red[r_idx++] = U(i, j);
            } else {
                black_map[b_idx * 2] = i;
                black_map[b_idx * 2 + 1] = j;
                u_black[b_idx++] = U(i, j);
            }
        }
    }
    
    const int check_interval = 100;
    
    for (int iter = 0; iter < max_iter; ++iter) {
        // 红点更新 - 现在是完全连续访问
        #pragma omp parallel for schedule(static) num_threads(num_threads)
        for (int idx = 0; idx < n_red; ++idx) {
            int i = red_map[idx * 2];
            int j = red_map[idx * 2 + 1];
            u_red[idx] = 0.25 * (U(i-1, j) + U(i+1, j) + 
                                 U(i, j-1) + U(i, j+1) + 
                                 h2 * F(i-1, j-1));
        }
        
        // 写回红点到原数组
        #pragma omp parallel for schedule(static) num_threads(num_threads)
        for (int idx = 0; idx < n_red; ++idx) {
            int i = red_map[idx * 2];
            int j = red_map[idx * 2 + 1];
            U(i, j) = u_red[idx];
        }
        
        // 黑点更新
        #pragma omp parallel for schedule(static) num_threads(num_threads)
        for (int idx = 0; idx < n_black; ++idx) {
            int i = black_map[idx * 2];
            int j = black_map[idx * 2 + 1];
            u_black[idx] = 0.25 * (U(i-1, j) + U(i+1, j) + 
                                   U(i, j-1) + U(i, j+1) + 
                                   h2 * F(i-1, j-1));
        }
        
        // 写回黑点
        #pragma omp parallel for schedule(static) num_threads(num_threads)
        for (int idx = 0; idx < n_black; ++idx) {
            int i = black_map[idx * 2];
            int j = black_map[idx * 2 + 1];
            U(i, j) = u_black[idx];
        }
        
        if (iter % check_interval == check_interval - 1) {
            residual = GaussSeidel2D::compute_residual(u, f, N, h);
            if (residual < tol) {
                iter_count = iter + 1;
                break;
            }
        }
    }
    
    residual = GaussSeidel2D::compute_residual(u, f, N, h);
    if (iter_count == 0) {  // 未break，说明跑完所有迭代
        iter_count = max_iter;
    }
}

} // namespace GaussSeidel2DOptimized

#undef U
#undef F
