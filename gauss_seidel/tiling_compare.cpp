#include "gauss_seidel_2d.h"
#include <cmath>
#include <algorithm>
#include <omp.h>

#define U(i, j) u[(i) * (N + 2) + (j)]
#define F(i, j) f[(i) * N + (j)]

// 0层tiling：最简单版本
void solve_no_tiling(
    std::vector<double>& u,
    const std::vector<double>& f,
    int N, double h, int max_iter, double tol,
    int& iter_count, double& residual, int num_threads
) {
    double h2 = h * h;
    omp_set_num_threads(num_threads);
    const int check_interval = (N < 256) ? 200 : 100;
    iter_count = 0;
    
    #pragma omp parallel num_threads(num_threads) firstprivate(h2)
    {
        for (int iter = 0; iter < max_iter; ++iter) {
            #pragma omp for schedule(static) nowait
            for (int i = 1; i <= N; ++i) {
                int j_start = (i % 2 == 1) ? 1 : 2;
                for (int j = j_start; j <= N; j += 2) {
                    U(i, j) = 0.25 * (U(i-1, j) + U(i+1, j) + 
                                      U(i, j-1) + U(i, j+1) + h2 * F(i-1, j-1));
                }
            }
            #pragma omp barrier
            
            #pragma omp for schedule(static) nowait
            for (int i = 1; i <= N; ++i) {
                int j_start = (i % 2 == 1) ? 2 : 1;
                for (int j = j_start; j <= N; j += 2) {
                    U(i, j) = 0.25 * (U(i-1, j) + U(i+1, j) + 
                                      U(i, j-1) + U(i, j+1) + h2 * F(i-1, j-1));
                }
            }
            #pragma omp barrier
            
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

// 1层tiling：只分块不嵌套
void solve_1level_tiling(
    std::vector<double>& u,
    const std::vector<double>& f,
    int N, double h, int max_iter, double tol,
    int& iter_count, double& residual, int num_threads
) {
    double h2 = h * h;
    omp_set_num_threads(num_threads);
    const int TILE = (N >= 512) ? 64 : 32;
    const int check_interval = (N < 256) ? 200 : 100;
    iter_count = 0;
    
    #pragma omp parallel num_threads(num_threads) firstprivate(h2)
    {
        for (int iter = 0; iter < max_iter; ++iter) {
            #pragma omp for schedule(static) nowait
            for (int bi = 1; bi <= N; bi += TILE) {
                int bi_end = std::min(bi + TILE, N + 1);
                for (int i = bi; i < bi_end; ++i) {
                    int j_start = (i % 2 == 1) ? 1 : 2;
                    for (int j = j_start; j <= N; j += 2) {
                        U(i, j) = 0.25 * (U(i-1, j) + U(i+1, j) + 
                                          U(i, j-1) + U(i, j+1) + h2 * F(i-1, j-1));
                    }
                }
            }
            #pragma omp barrier
            
            #pragma omp for schedule(static) nowait
            for (int bi = 1; bi <= N; bi += TILE) {
                int bi_end = std::min(bi + TILE, N + 1);
                for (int i = bi; i < bi_end; ++i) {
                    int j_start = (i % 2 == 1) ? 2 : 1;
                    for (int j = j_start; j <= N; j += 2) {
                        U(i, j) = 0.25 * (U(i-1, j) + U(i+1, j) + 
                                          U(i, j-1) + U(i, j+1) + h2 * F(i-1, j-1));
                    }
                }
            }
            #pragma omp barrier
            
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

// 2层tiling（当前实现）在主文件中

#undef U
#undef F
