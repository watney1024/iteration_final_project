#include <iostream>
#include <omp.h>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;

int main()
{
    // 设置Windows控制台为UTF-8编码
    #ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    #endif
    
    // 显示当前OpenMP设置
    cout << "==============================================" << endl;
    cout << "         OpenMP 线程测试" << endl;
    cout << "==============================================" << endl;
    
    // 获取最大可用线程数
    int max_threads = omp_get_max_threads();
    cout << "\n系统最大可用线程数: " << max_threads << endl;
    
    // 获取处理器数量
    int num_procs = omp_get_num_procs();
    cout << "系统处理器数量: " << num_procs << endl;
    
    // 检查环境变量设置
    char* env_threads = getenv("OMP_NUM_THREADS");
    if (env_threads != NULL) {
        cout << "OMP_NUM_THREADS 环境变量: " << env_threads << endl;
    } else {
        cout << "OMP_NUM_THREADS 环境变量: 未设置" << endl;
    }
    
    cout << "\n----------------------------------------------" << endl;
    cout << "测试并行区域实际启动的线程数:" << endl;
    cout << "----------------------------------------------" << endl;
    
    // 测试实际并行区域
#pragma omp parallel
    {
#pragma omp single
        {
            int actual_threads = omp_get_num_threads();
            cout << "并行区域实际启动线程数: " << actual_threads << endl;
        }
        
        // 每个线程打印自己的ID
#pragma omp critical
        {
            int thread_id = omp_get_thread_num();
            cout << "  线程 " << thread_id << " 正在运行" << endl;
        }
    }
    
    cout << "\n----------------------------------------------" << endl;
    cout << "测试不同线程数设置:" << endl;
    cout << "----------------------------------------------" << endl;
    
    // 测试不同的线程数
    int test_threads[] = {1, 2, 4, 8};
    for (int i = 0; i < 4; i++) {
        int num = test_threads[i];
        omp_set_num_threads(num);
        
#pragma omp parallel
        {
#pragma omp single
            {
                int actual = omp_get_num_threads();
                cout << "设置 " << num << " 线程 -> 实际启动: " << actual << " 线程" << endl;
            }
        }
    }
    
    cout << "\n==============================================" << endl;
    cout << "         测试完成" << endl;
    cout << "==============================================" << endl;
    
    return 0;
}