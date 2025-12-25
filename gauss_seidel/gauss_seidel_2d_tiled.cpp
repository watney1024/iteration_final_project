#include "gauss_seidel_2d.h"
#include <cmath>
#include <algorithm>
#include <omp.h>
#include <immintrin.h>

// 2层分块优化的Gauss-Seidel求解器
// 外层：L3 Cache块，内层：L1 Cache块

#define U(i, j) u[(i) * (N + 2) + (j)]
#define F(i, j) f[(i) * N + (j)]

namespace GaussSeidel2DTiled {

// 2层分块并行红黑Gauss-Seidel
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
    omp_set_num_threads(num_threads);
    
    // 2层tiling策略
    // 外层：L3 Cache块（线程间分配）
    // 内层：L1 Cache块（线程内优化）
    const int L3_TILE = (N >= 512) ? 128 : 64;
    const int L1_TILE = 16;
    
    const int check_interval = (N < 256) ? 200 : 100;
    iter_count = 0;
    
    #pragma omp parallel num_threads(num_threads) firstprivate(h2)
    {
        // 寄存器缓存
        double reg[4];
        
        for (int iter = 0; iter < max_iter; ++iter) {
            
            // ========== 红点更新 ==========
            #pragma omp for schedule(static) nowait
            for (int bi = 1; bi <= N; bi += L3_TILE) {
                int bi_end = std::min(bi + L3_TILE, N + 1);
                
                // L1 Cache级别优化
                for (int ti = bi; ti < bi_end; ti += L1_TILE) {
                    int ti_end = std::min(ti + L1_TILE, bi_end);
                    
                    // 预取下一个L1块
                    #ifdef __x86_64__
                    if (ti + L1_TILE < bi_end) {
                        _mm_prefetch((const char*)&U(ti + L1_TILE, 1), _MM_HINT_T0);
                    }
                    #endif
                    
                    for (int i = ti; i < ti_end; ++i) {
                        int j_start = (i % 2 == 1) ? 1 : 2;
                        
                        // 内核循环 - 红点
                        for (int j = j_start; j <= N; j += 2) {
                            // 显式加载到寄存器减少内存访问
                            reg[0] = U(i-1, j);
                            reg[1] = U(i+1, j);
                            reg[2] = U(i, j-1);
                            reg[3] = U(i, j+1);
                            
                            U(i, j) = 0.25 * (reg[0] + reg[1] + reg[2] + reg[3] + h2 * F(i-1, j-1));
                        }
                    }
                }
            }
            
            #pragma omp barrier
            
            // ========== 黑点更新 ==========
            #pragma omp for schedule(static) nowait
            for (int bi = 1; bi <= N; bi += L3_TILE) {
                int bi_end = std::min(bi + L3_TILE, N + 1);
                
                for (int ti = bi; ti < bi_end; ti += L1_TILE) {
                    int ti_end = std::min(ti + L1_TILE, bi_end);
                    
                    if (ti + L1_TILE < bi_end) {
                        _mm_prefetch((const char*)&U(ti + L1_TILE, 1), _MM_HINT_T0);
                    }
                    
                    for (int i = ti; i < ti_end; ++i) {
                        int j_start = (i % 2 == 1) ? 2 : 1;
                        
                        for (int j = j_start; j <= N; j += 2) {
                            reg[0] = U(i-1, j);
                            reg[1] = U(i+1, j);
                            reg[2] = U(i, j-1);
                            reg[3] = U(i, j+1);
                            
                            U(i, j) = 0.25 * (reg[0] + reg[1] + reg[2] + reg[3] + h2 * F(i-1, j-1));
                        }
                    }
                }
            }
            
            #pragma omp barrier
            
            // 收敛检查
            if (iter % check_interval == check_interval - 1) {
                #pragma omp single
                {
                    residual = GaussSeidel2D::compute_residual(u, f, N, h);
                    if (residual < tol) {
                        iter_count = iter + 1;
                        max_iter = iter;
                    }
                }
            }
        }
    }
    
    if (iter_count == 0) {
        residual = GaussSeidel2D::compute_residual(u, f, N, h);
        iter_count = max_iter;
    }
}

} // namespace GaussSeidel2DTiled

#undef U
#undef F
