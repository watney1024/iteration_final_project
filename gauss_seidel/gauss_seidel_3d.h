#ifndef GAUSS_SEIDEL_3D_H
#define GAUSS_SEIDEL_3D_H

#include <vector>

// 三维泊松方程求解器
// 求解 -Δu = f，边界条件 u = 0

class GaussSeidel3D {
public:
    // 串行普通 Gauss-Seidel
    static void solve_serial(
        std::vector<double>& u,      // 解向量 (N+2)x(N+2)x(N+2)，包含边界
        const std::vector<double>& f, // 右端项 NxNxN
        int N,                        // 内部网格点数
        double h,                     // 网格间距
        int max_iter,                 // 最大迭代次数
        double tol,                   // 收敛容差
        int& iter_count,             // 实际迭代次数
        double& residual             // 最终残差
    );

    // 串行红黑 Gauss-Seidel
    static void solve_serial_redblack(
        std::vector<double>& u,
        const std::vector<double>& f,
        int N,
        double h,
        int max_iter,
        double tol,
        int& iter_count,
        double& residual
    );

    // 并行红黑 Gauss-Seidel（基于区域分解）
    static void solve_parallel_redblack(
        std::vector<double>& u,
        const std::vector<double>& f,
        int N,
        double h,
        int max_iter,
        double tol,
        int& iter_count,
        double& residual,
        int num_threads = 8          // OpenMP线程数
    );

    // 计算残差范数
    static double compute_residual(
        const std::vector<double>& u,
        const std::vector<double>& f,
        int N,
        double h
    );

    // 初始化测试问题
    static void init_test_problem(
        std::vector<double>& u,
        std::vector<double>& f,
        std::vector<double>& u_exact,
        int N,
        double h
    );
};

#endif // GAUSS_SEIDEL_3D_H
