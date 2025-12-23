#include "red_black_gauss_seidel.h"
#include <cmath>
#include <omp.h>
#include <iostream>

// 计算残差范数
double RedBlackGaussSeidel::compute_residual(
    const std::vector<std::vector<double>>& A,
    const std::vector<double>& b,
    const std::vector<double>& x
) {
    int n = b.size();
    double residual = 0.0;
    
    for (int i = 0; i < n; i++) {
        double sum = 0.0;
        for (int j = 0; j < n; j++) {
            sum += A[i][j] * x[j];
        }
        double r = b[i] - sum;
        residual += r * r;
    }
    
    return std::sqrt(residual);
}

// 串行红黑排序 Gauss-Seidel 求解
void RedBlackGaussSeidel::solve_serial(
    const std::vector<std::vector<double>>& A,
    const std::vector<double>& b,
    std::vector<double>& x,
    int max_iterations,
    double tolerance
) {
    int n = b.size();
    
    for (int iter = 0; iter < max_iterations; iter++) {
        // 更新红点（Red points: (i+j) % 2 == 0）
        for (int i = 0; i < n; i++) {
            if (!is_red(i, 0)) continue;
            
            double sigma = 0.0;
            for (int j = 0; j < n; j++) {
                if (j != i) {
                    sigma += A[i][j] * x[j];
                }
            }
            x[i] = (b[i] - sigma) / A[i][i];
        }
        
        // 更新黑点（Black points: (i+j) % 2 == 1）
        for (int i = 0; i < n; i++) {
            if (is_red(i, 0)) continue;
            
            double sigma = 0.0;
            for (int j = 0; j < n; j++) {
                if (j != i) {
                    sigma += A[i][j] * x[j];
                }
            }
            x[i] = (b[i] - sigma) / A[i][i];
        }
        
        // 检查收敛性
        double residual = compute_residual(A, b, x);
        if (residual < tolerance) {
            std::cout << "Serial converged at iteration " << iter + 1 
                     << " with residual " << residual << std::endl;
            break;
        }
    }
}

// 并行红黑排序 Gauss-Seidel 求解（使用OpenMP）
void RedBlackGaussSeidel::solve_parallel(
    const std::vector<std::vector<double>>& A,
    const std::vector<double>& b,
    std::vector<double>& x,
    int max_iterations,
    double tolerance,
    int num_threads
) {
    int n = b.size();
    omp_set_num_threads(num_threads);
    
    for (int iter = 0; iter < max_iterations; iter++) {
        // 更新红点（Red points: i % 2 == 0）
        // 红点之间相互独立，可以并行更新
        #pragma omp parallel for schedule(static)
        for (int i = 0; i < n; i++) {
            if (!is_red(i, 0)) continue;
            
            double sigma = 0.0;
            for (int j = 0; j < n; j++) {
                if (j != i) {
                    sigma += A[i][j] * x[j];
                }
            }
            x[i] = (b[i] - sigma) / A[i][i];
        }
        
        // 同步，确保所有红点更新完成
        #pragma omp barrier
        
        // 更新黑点（Black points: i % 2 == 1）
        // 黑点之间相互独立，可以并行更新
        #pragma omp parallel for schedule(static)
        for (int i = 0; i < n; i++) {
            if (is_red(i, 0)) continue;
            
            double sigma = 0.0;
            for (int j = 0; j < n; j++) {
                if (j != i) {
                    sigma += A[i][j] * x[j];
                }
            }
            x[i] = (b[i] - sigma) / A[i][i];
        }
        
        // 检查收敛性（每10次迭代检查一次以减少开销）
        if (iter % 10 == 0 || iter == max_iterations - 1) {
            double residual = compute_residual(A, b, x);
            if (residual < tolerance) {
                std::cout << "Parallel (" << num_threads << " threads) converged at iteration " 
                         << iter + 1 << " with residual " << residual << std::endl;
                break;
            }
        }
    }
}
