#ifndef RED_BLACK_GAUSS_SEIDEL_H
#define RED_BLACK_GAUSS_SEIDEL_H

#include <vector>

// 红黑排序 Gauss-Seidel 求解器类
class RedBlackGaussSeidel {
public:
    // 串行版本
    static void solve_serial(
        const std::vector<std::vector<double>>& A,
        const std::vector<double>& b,
        std::vector<double>& x,
        int max_iterations,
        double tolerance
    );

    // 并行版本（使用OpenMP）
    static void solve_parallel(
        const std::vector<std::vector<double>>& A,
        const std::vector<double>& b,
        std::vector<double>& x,
        int max_iterations,
        double tolerance,
        int num_threads
    );

    // 计算残差范数
    static double compute_residual(
        const std::vector<std::vector<double>>& A,
        const std::vector<double>& b,
        const std::vector<double>& x
    );

private:
    // 判断点的颜色（红或黑）
    static inline bool is_red(int i, int j, int k = 0) {
        return (i + j + k) % 2 == 0;
    }
};

#endif // RED_BLACK_GAUSS_SEIDEL_H
