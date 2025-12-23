#ifndef TDMA_SOLVER_H
#define TDMA_SOLVER_H

#include <vector>

/**
 * @brief 三对角矩阵求解器
 * @details 求解三对角线性方程组 Ax = d
 *          A 为三对角矩阵，由下对角线 a，主对角线 b，上对角线 c 组成
 *          参考：jihoonakang/parallel_tdma_cpp
 */
class TDMASolver {
public:
    /**
     * @brief 串行 Thomas 算法（追赶法）
     * @param a 下对角线系数 (a[0] 不使用)
     * @param b 主对角线系数
     * @param c 上对角线系数 (c[n-1] 不使用)
     * @param d 右端项
     * @param x 解向量 (输出)
     * @param n 方程组维数
     */
    static void solve_thomas(
        const std::vector<double>& a,
        const std::vector<double>& b,
        const std::vector<double>& c,
        const std::vector<double>& d,
        std::vector<double>& x,
        int n
    );

    /**
     * @brief 并行 PCR (Parallel Cyclic Reduction) 算法
     * @param a 下对角线系数 (a[0] 不使用)
     * @param b 主对角线系数
     * @param c 上对角线系数 (c[n-1] 不使用)
     * @param d 右端项
     * @param x 解向量 (输出)
     * @param n 方程组维数
     * @param num_threads OpenMP 线程数
     */
    static void solve_pcr(
        const std::vector<double>& a,
        const std::vector<double>& b,
        const std::vector<double>& c,
        const std::vector<double>& d,
        std::vector<double>& x,
        int n,
        int num_threads = 8
    );

    /**
     * @brief 验证解的正确性
     * @param a 下对角线系数
     * @param b 主对角线系数
     * @param c 上对角线系数
     * @param d 右端项
     * @param x 解向量
     * @param n 方程组维数
     * @return 残差的 L2 范数
     */
    static double verify_solution(
        const std::vector<double>& a,
        const std::vector<double>& b,
        const std::vector<double>& c,
        const std::vector<double>& d,
        const std::vector<double>& x,
        int n
    );

    /**
     * @brief 生成对角占优的三对角矩阵测试用例
     * @param a 下对角线系数 (输出)
     * @param b 主对角线系数 (输出)
     * @param c 上对角线系数 (输出)
     * @param d 右端项 (输出)
     * @param n 方程组维数
     */
    static void generate_test_system(
        std::vector<double>& a,
        std::vector<double>& b,
        std::vector<double>& c,
        std::vector<double>& d,
        int n
    );
};

#endif // TDMA_SOLVER_H
