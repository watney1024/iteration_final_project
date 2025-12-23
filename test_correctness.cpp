#include <iostream>
#include <vector>
#include <iomanip>
#include "red_black_gauss_seidel.h"

#ifdef _WIN32
#include <windows.h>
#endif

void test_small_system() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "Small System Test (3x3)" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    // 测试系统: 
    // 10x - y + 2z = 6
    // -x + 11y - z + 3 = 25
    // 2x - y + 10z = -11
    
    std::vector<std::vector<double>> A = {
        {10, -1, 2},
        {-1, 11, -1},
        {2, -1, 10}
    };
    
    std::vector<double> b = {6, 25, -11};
    std::vector<double> x_serial(3, 0.0);
    std::vector<double> x_parallel(3, 0.0);
    
    std::cout << "System of equations:" << std::endl;
    std::cout << "10x - y + 2z = 6" << std::endl;
    std::cout << "-x + 11y - z = 25" << std::endl;
    std::cout << "2x - y + 10z = -11" << std::endl;
    std::cout << "\nExpected solution: x ≈ 1, y ≈ 2, z ≈ -1\n" << std::endl;
    
    // 串行求解
    RedBlackGaussSeidel::solve_serial(A, b, x_serial, 100, 1e-10);
    std::cout << "\nSerial solution:" << std::endl;
    std::cout << "x = " << std::fixed << std::setprecision(6) << x_serial[0] << std::endl;
    std::cout << "y = " << x_serial[1] << std::endl;
    std::cout << "z = " << x_serial[2] << std::endl;
    
    // 并行求解
    RedBlackGaussSeidel::solve_parallel(A, b, x_parallel, 100, 1e-10, 2);
    std::cout << "\nParallel solution (2 threads):" << std::endl;
    std::cout << "x = " << std::fixed << std::setprecision(6) << x_parallel[0] << std::endl;
    std::cout << "y = " << x_parallel[1] << std::endl;
    std::cout << "z = " << x_parallel[2] << std::endl;
    
    std::cout << std::endl;
}

int main() {
    // 设置控制台为UTF-8编码（Windows）
    #ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    #endif
    
    test_small_system();
    return 0;
}
