#ifndef GAUSS_SEIDEL_2D_MT_OPTIMIZED_H
#define GAUSS_SEIDEL_2D_MT_OPTIMIZED_H

#include <vector>

namespace GaussSeidel2DMultiThread {

// 优化1: 消除隐式栅障
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
);

// 优化2: 行分块 + 消除伪共享
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
);

// 优化3: 流水线式波前法
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
);

} // namespace GaussSeidel2DMultiThread

#endif
