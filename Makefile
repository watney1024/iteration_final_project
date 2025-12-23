# 编译器和选项
CXX = g++
CXXFLAGS = -fopenmp -O2 -std=c++11

# 自动查找所有 .cpp 文件
SOURCES = $(wildcard *.cpp)
OBJECTS = $(SOURCES:.cpp=.o)

# 默认目标
all: test_gs_2d

# 主程序
test_gs_2d: test_gs_2d.o gauss_seidel_2d.o
	$(CXX) $(CXXFLAGS) $^ -o $@.exe

# 通用规则：编译 .cpp 到 .o
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 清理
clean:
	rm -f *.o *.exe

# 运行
run: test_gs_2d
	./test_gs_2d.exe

.PHONY: all clean run
