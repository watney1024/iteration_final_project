# Makefile for parallel tridiagonal solvers

CXX = g++
CXXFLAGS = -fopenmp -O2 -std=c++11 -Wall
PYTHON = python

# 目标程序
TARGETS = sequential_solver.exe openmp_recursive_doubling.exe openmp_brugnano.exe

# 默认规模
TEST_SIZE = 8192

.PHONY: all clean test generate_input run_all

all: $(TARGETS)

# 编译规则
sequential_solver.exe: sequential_solver.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

openmp_recursive_doubling.exe: openmp_recursive_doubling.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

openmp_brugnano.exe: openmp_brugnano.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

# 生成测试数据
generate_input:
	$(PYTHON) testGenerator.py -n $(TEST_SIZE) -o inputs/test_input.txt

# 运行所有测试
run_all: all generate_input
	@echo "======================================"
	@echo "运行串行求解器"
	@echo "======================================"
	./sequential_solver.exe
	@echo ""
	@echo "======================================"
	@echo "运行递归倍增并行求解器"
	@echo "======================================"
	./openmp_recursive_doubling.exe
	@echo ""
	@echo "======================================"
	@echo "运行 Brugnano 并行求解器"
	@echo "======================================"
	./openmp_brugnano.exe

# 测试不同规模
test: all
	@echo "测试不同问题规模..."
	@for size in 1024 4096 8192 16384; do \
		echo ""; \
		echo "========================================"; \
		echo "测试规模: N = $$size"; \
		echo "========================================"; \
		$(PYTHON) testGenerator.py -n $$size -o inputs/test_$$size.txt; \
		echo ""; \
		echo "[递归倍增]"; \
		./openmp_recursive_doubling.exe inputs/test_$$size.txt; \
		echo ""; \
		echo "[Brugnano]"; \
		./openmp_brugnano.exe inputs/test_$$size.txt; \
	done

clean:
	rm -f $(TARGETS)
	rm -rf inputs/

help:
	@echo "可用的 make 目标:"
	@echo "  make all              - 编译所有程序"
	@echo "  make generate_input   - 生成测试数据 (默认 N=$(TEST_SIZE))"
	@echo "  make run_all          - 编译并运行所有求解器"
	@echo "  make test             - 测试不同问题规模"
	@echo "  make clean            - 清理生成的文件"
	@echo ""
	@echo "自定义规模:"
	@echo "  make generate_input TEST_SIZE=16384"
	@echo "  make run_all TEST_SIZE=16384"
