# 编译器设置
CXX = g++
CXXFLAGS = -fopenmp -O3 -std=c++11 -Wall

# 目标文件
TARGETS = test_performance test_correctness

# 源文件
SOURCES = red_black_gauss_seidel.cpp
HEADERS = red_black_gauss_seidel.h

# Windows 可执行文件扩展名
ifeq ($(OS),Windows_NT)
    EXT = .exe
else
    EXT =
endif

# 默认目标
all: $(TARGETS)

# 编译性能测试程序
test_performance: test_performance.cpp $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) test_performance.cpp $(SOURCES) -o test_performance$(EXT)

# 编译正确性测试程序
test_correctness: test_correctness.cpp $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) test_correctness.cpp $(SOURCES) -o test_correctness$(EXT)

# 清理编译产物
clean:
	rm -f $(TARGETS) *.exe *.o

# 运行正确性测试
test: test_correctness
	./test_correctness$(EXT)

# 运行性能测试
bench: test_performance
	./test_performance$(EXT)

# 帮助信息
help:
	@echo "可用的 make 目标："
	@echo "  make          - 编译所有程序"
	@echo "  make clean    - 清理编译产物"
	@echo "  make test     - 编译并运行正确性测试"
	@echo "  make bench    - 编译并运行性能测试"
	@echo "  make help     - 显示此帮助信息"

.PHONY: all clean test bench help
