#include "gauss_seidel_2d.h"
#include <cmath>
#include <algorithm>
#include <omp.h>

#define U(i, j) u[(i) * (N + 2) + (j)]
#define F(i, j) f[(i) * N + (j)]

namespace GaussSeidel2DMultiThread {

// ========== 优化1: 消除隐式栅障 ==========
void solve_no_implicit_barrier(
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
    const int check_interval = 10;
    iter_count = 0;
    
    // 关键：parallel region在整个迭代外部，避免重复创建线程
    #pragma omp parallel firstprivate(h2) num_threads(num_threads)
    {
        for (int iter = 0; iter < max_iter; ++iter) {
            // 红点更新 - nowait避免隐式栅障
            #pragma omp for schedule(static) nowait
            for (int i = 1; i <= N; ++i) {
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
                    residual = GaussSeidel2D::compute_residual(u, f, N, h);
                    if (residual < tol) {
                        iter_count = iter + 1;
                        max_iter = iter;  // 提前终止
                    }
                }
            }
            
            // 确保所有线程同步max_iter的更新
            #pragma omp barrier
        }
    }
    
    if (iter_count == 0) {
        residual = GaussSeidel2D::compute_residual(u, f, N, h);
        iter_count = max_iter;
    }
}


// ========== 优化2: 行分块 + 消除伪共享 ==========
void solve_row_blocking(
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
    const int check_interval = 10;
    iter_count = 0;
    
    // 行分块大小：至少8行/块，避免false sharing
    int block_size = std::max(8, N / (num_threads * 4));
    
    #pragma omp parallel firstprivate(h2) num_threads(num_threads)
    {
        for (int iter = 0; iter < max_iter; ++iter) {
            // 红点更新 - 按块处理
            #pragma omp for schedule(static, block_size) nowait
            for (int i = 1; i <= N; ++i) {
                int j_start = (i % 2 == 1) ? 1 : 2;
                for (int j = j_start; j <= N; j += 2) {
                    U(i, j) = 0.25 * (U(i-1, j) + U(i+1, j) + 
                                      U(i, j-1) + U(i, j+1) + 
                                      h2 * F(i-1, j-1));
                }
            }
            
            #pragma omp barrier
            
            // 黑点更新 - 按块处理
            #pragma omp for schedule(static, block_size) nowait
            for (int i = 1; i <= N; ++i) {
                int j_start = (i % 2 == 1) ? 2 : 1;
                for (int j = j_start; j <= N; j += 2) {
                    U(i, j) = 0.25 * (U(i-1, j) + U(i+1, j) + 
                                      U(i, j-1) + U(i, j+1) + 
                                      h2 * F(i-1, j-1));
                }
            }
            
            #pragma omp barrier
            
            #pragma omp master
            {
                if (iter % check_interval == check_interval - 1) {
                    residual = GaussSeidel2D::compute_residual(u, f, N, h);
                    if (residual < tol) {
                        iter_count = iter + 1;
                        max_iter = iter;
                    }
                }
            }
            #pragma omp barrier
        }
    }
    
    if (iter_count == 0) {
        residual = GaussSeidel2D::compute_residual(u, f, N, h);
        iter_count = max_iter;
    }
}


// ========== 优化3: 流水线式波前法 ==========
void solve_wavefront_pipeline(
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
    const int check_interval = 10;
    iter_count = 0;
    
    // 波前法：沿对角线方向并行
    #pragma omp parallel firstprivate(h2) num_threads(num_threads)
    {
        for (int iter = 0; iter < max_iter; ++iter) {
            // 红点更新 - 波前法
            for (int diag = 0; diag < 2*N; ++diag) {
                #pragma omp for schedule(dynamic, 8) nowait
                for (int i = 1; i <= N; ++i) {
                    int j_start = (i % 2 == 1) ? 1 : 2;
                    for (int j = j_start; j <= N; j += 2) {
                        if (i + j == diag + 2) {  // 在当前对角线上
                            U(i, j) = 0.25 * (U(i-1, j) + U(i+1, j) + 
                                              U(i, j-1) + U(i, j+1) + 
                                              h2 * F(i-1, j-1));
                        }
                    }
                }
                #pragma omp barrier
            }
            
            // 黑点更新 - 波前法
            for (int diag = 0; diag < 2*N; ++diag) {
                #pragma omp for schedule(dynamic, 8) nowait
                for (int i = 1; i <= N; ++i) {
                    int j_start = (i % 2 == 1) ? 2 : 1;
                    for (int j = j_start; j <= N; j += 2) {
                        if (i + j == diag + 2) {
                            U(i, j) = 0.25 * (U(i-1, j) + U(i+1, j) + 
                                              U(i, j-1) + U(i, j+1) + 
                                              h2 * F(i-1, j-1));
                        }
                    }
                }
                #pragma omp barrier
            }
            
            #pragma omp master
            {
                if (iter % check_interval == check_interval - 1) {
                    residual = GaussSeidel2D::compute_residual(u, f, N, h);
                    if (residual < tol) {
                        iter_count = iter + 1;
                        max_iter = iter;
                    }
                }
            }
            #pragma omp barrier
        }
    }
    
    if (iter_count == 0) {
        residual = GaussSeidel2D::compute_residual(u, f, N, h);
        iter_count = max_iter;
    }
}

} // namespace GaussSeidel2DMultiThread
