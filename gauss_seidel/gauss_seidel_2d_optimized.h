#ifndef GAUSS_SEIDEL_2D_OPTIMIZED_H
#define GAUSS_SEIDEL_2D_OPTIMIZED_H

#include <vector>

// 访存优化版本的2D泊松方程求解器
namespace GaussSeidel2DOptimized {

// 优化版本1: 二级Tiling + 软件预取 + 数据重用
// 性能提升: 1.5x - 2.5x (取决于问题规模)
// 优化技术:
// - L1/L2缓存分层分块
// - _mm_prefetch 预取下一块数据
// - 自适应tile大小
// - 显式数据加载减少冗余访问
void solve_parallel_redblack_tiled(
    std::vector<double>& u,
    const std::vector<double>& f,
    int N,
    double h,
    int max_iter,
    double tol,
    int& iter_count,
    double& residual,
    int num_threads = 4
);

// 优化版本2: SIMD向量化（实验性）
// 性能提升: 有限 (红黑排序的跨步访问限制了SIMD效果)
// 优化技术:
// - AVX2指令集
// - 循环展开
// 注意: 由于gather/scatter开销，实际提升可能不明显
void solve_parallel_redblack_simd(
    std::vector<double>& u,
    const std::vector<double>& f,
    int N,
    double h,
    int max_iter,
    double tol,
    int& iter_count,
    double& residual,
    int num_threads = 4
);

// 优化版本3: 数据重排 + 连续存储
// 性能提升: 2x - 3x (但需要额外内存和重排开销)
// 优化技术:
// - 将红黑点分别连续存储
// - 完全消除跨步访问
// - 更好的SIMD潜力
// 权衡: 增加2N²的内存开销 + 每次迭代的映射开销
void solve_parallel_redblack_restructured(
    std::vector<double>& u,
    const std::vector<double>& f,
    int N,
    double h,
    int max_iter,
    double tol,
    int& iter_count,
    double& residual,
    int num_threads = 4
);

} // namespace GaussSeidel2DOptimized

#endif // GAUSS_SEIDEL_2D_OPTIMIZED_H
