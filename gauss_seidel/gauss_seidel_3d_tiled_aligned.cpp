#include "gauss_seidel_3d.h"
#include <cmath>
#include <algorithm>
#include <omp.h>
#include <cstdlib>

#ifdef _WIN32
#include <malloc.h>
#define aligned_alloc(alignment, size) _aligned_malloc(size, alignment)
#define aligned_free(ptr) _aligned_free(ptr)
#else
#define aligned_free(ptr) free(ptr)
#endif

#define U(i, j, k) u[(i) * (N + 2) * (N + 2) + (j) * (N + 2) + (k)]
#define F(i, j, k) f[(i) * N * N + (j) * N + (k)]

namespace GaussSeidel3DTiledAligned {

// 前置声明
double compute_residual_aligned(const double* u, const double* f, int N, double h);

// 64字节对齐的内存分配wrapper
class AlignedArray {
private:
    double* data_;
    size_t size_;
    
public:
    AlignedArray(size_t size) : size_(size) {
        data_ = static_cast<double*>(aligned_alloc(64, size * sizeof(double)));
        if (!data_) {
            throw std::bad_alloc();
        }
    }
    
    ~AlignedArray() {
        if (data_) {
            aligned_free(data_);
        }
    }
    
    double* data() { return data_; }
    const double* data() const { return data_; }
    size_t size() const { return size_; }
    
    double& operator[](size_t idx) { return data_[idx]; }
    const double& operator[](size_t idx) const { return data_[idx]; }
    
    // 禁用拷贝
    AlignedArray(const AlignedArray&) = delete;
    AlignedArray& operator=(const AlignedArray&) = delete;
};

// 2层分块并行红黑Gauss-Seidel（内存对齐优化）
void solve_4level_tiling_aligned(
    std::vector<double>& u_vec,
    const std::vector<double>& f_vec,
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
    
    // 分配64字节对齐的内存
    AlignedArray u_aligned((N + 2) * (N + 2) * (N + 2));
    AlignedArray f_aligned(N * N * N);
    
    double* u = u_aligned.data();
    double* f = f_aligned.data();
    
    // 拷贝输入数据到对齐内存
    std::copy(u_vec.begin(), u_vec.end(), u);
    std::copy(f_vec.begin(), f_vec.end(), f);
    
    // 2层tiling策略
    int L3_tile = 64;
    int L1_tile = 16;
    
    if (N <= 32) {
        L3_tile = 32;
        L1_tile = 8;
    } else if (N <= 64) {
        L3_tile = 32;
        L1_tile = 16;
    } else if (N >= 128) {
        L3_tile = 128;
        L1_tile = 32;
    }
    
    int check_interval = (N >= 64) ? 500 : 100;
    iter_count = 0;
    
    #pragma omp parallel num_threads(num_threads)
    {
        double local_h2 = h2;
        
        for (int iter = 0; iter < max_iter; ++iter) {
            
            // ============ 红点更新 ============
            #pragma omp for schedule(dynamic, 2) collapse(3) nowait
            for (int block_i = 1; block_i <= N; block_i += L3_tile) {
                for (int block_j = 1; block_j <= N; block_j += L3_tile) {
                    for (int block_k = 1; block_k <= N; block_k += L3_tile) {
                        int i_end = std::min(block_i + L3_tile, N + 1);
                        int j_end = std::min(block_j + L3_tile, N + 1);
                        int k_end = std::min(block_k + L3_tile, N + 1);
                        
                        for (int tile_i = block_i; tile_i < i_end; tile_i += L1_tile) {
                            for (int tile_j = block_j; tile_j < j_end; tile_j += L1_tile) {
                                for (int tile_k = block_k; tile_k < k_end; tile_k += L1_tile) {
                                    int ti_end = std::min(tile_i + L1_tile, i_end);
                                    int tj_end = std::min(tile_j + L1_tile, j_end);
                                    int tk_end = std::min(tile_k + L1_tile, k_end);
                                    
                                    for (int i = tile_i; i < ti_end; ++i) {
                                        for (int j = tile_j; j < tj_end; ++j) {
                                            for (int k = tile_k + ((i + j + tile_k) % 2); k < tk_end; k += 2) {
                                                double u_im = U(i-1, j, k);
                                                double u_ip = U(i+1, j, k);
                                                double u_jm = U(i, j-1, k);
                                                double u_jp = U(i, j+1, k);
                                                double u_km = U(i, j, k-1);
                                                double u_kp = U(i, j, k+1);
                                                double f_val = local_h2 * F(i-1, j-1, k-1);
                                                
                                                U(i, j, k) = (u_im + u_ip + u_jm + u_jp + u_km + u_kp + f_val) / 6.0;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            
            #pragma omp barrier
            
            // ============ 黑点更新 ============
            #pragma omp for schedule(dynamic, 2) collapse(3) nowait
            for (int block_i = 1; block_i <= N; block_i += L3_tile) {
                for (int block_j = 1; block_j <= N; block_j += L3_tile) {
                    for (int block_k = 1; block_k <= N; block_k += L3_tile) {
                        int i_end = std::min(block_i + L3_tile, N + 1);
                        int j_end = std::min(block_j + L3_tile, N + 1);
                        int k_end = std::min(block_k + L3_tile, N + 1);
                        
                        for (int tile_i = block_i; tile_i < i_end; tile_i += L1_tile) {
                            for (int tile_j = block_j; tile_j < j_end; tile_j += L1_tile) {
                                for (int tile_k = block_k; tile_k < k_end; tile_k += L1_tile) {
                                    int ti_end = std::min(tile_i + L1_tile, i_end);
                                    int tj_end = std::min(tile_j + L1_tile, j_end);
                                    int tk_end = std::min(tile_k + L1_tile, k_end);
                                    
                                    for (int i = tile_i; i < ti_end; ++i) {
                                        for (int j = tile_j; j < tj_end; ++j) {
                                            for (int k = tile_k + ((i + j + tile_k + 1) % 2); k < tk_end; k += 2) {
                                                double u_im = U(i-1, j, k);
                                                double u_ip = U(i+1, j, k);
                                                double u_jm = U(i, j-1, k);
                                                double u_jp = U(i, j+1, k);
                                                double u_km = U(i, j, k-1);
                                                double u_kp = U(i, j, k+1);
                                                double f_val = local_h2 * F(i-1, j-1, k-1);
                                                
                                                U(i, j, k) = (u_im + u_ip + u_jm + u_jp + u_km + u_kp + f_val) / 6.0;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            
            #pragma omp barrier
            
            if (iter % check_interval == check_interval - 1) {
                #pragma omp single
                {
                    residual = compute_residual_aligned(u, f, N, h);
                    iter_count = iter + 1;
                }
                
                if (residual < tol) {
                    break;
                }
            }
        }
        
        #pragma omp single
        {
            if (iter_count == 0 || iter_count % check_interval != 0) {
                residual = compute_residual_aligned(u, f, N, h);
                iter_count = max_iter;
            }
        }
    }
    
    // 拷贝结果回vector
    std::copy(u, u + u_vec.size(), u_vec.begin());
}

// 计算残差（对齐内存版本）
double compute_residual_aligned(
    const double* u,
    const double* f,
    int N,
    double h
) {
    double h2 = h * h;
    double res = 0.0;
    
    #pragma omp parallel for reduction(+:res) collapse(3)
    for (int i = 1; i <= N; ++i) {
        for (int j = 1; j <= N; ++j) {
            for (int k = 1; k <= N; ++k) {
                double laplacian = (U(i-1,j,k) + U(i+1,j,k) + U(i,j-1,k) + U(i,j+1,k) + 
                                   U(i,j,k-1) + U(i,j,k+1) - 6.0*U(i,j,k)) / h2;
                double diff = laplacian - F(i-1, j-1, k-1);
                res += diff * diff;
            }
        }
    }
    
    return std::sqrt(res);
}

// 初始化测试问题（对齐内存版本）
void init_test_problem_aligned(
    std::vector<double>& u,
    std::vector<double>& f,
    std::vector<double>& u_exact,
    int N,
    double h
) {
    // 使用标准版本的初始化
    GaussSeidel3D::init_test_problem(u, f, u_exact, N, h);
}

} // namespace GaussSeidel3DTiledAligned

#undef U
#undef F
