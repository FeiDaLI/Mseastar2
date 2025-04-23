# # 编译器
# CXX = g++

# # 编译选项
# CXXFLAGS = -Wall -std=c++20 -g -Iinclude -I/home/lifd/googletest/googletest/include -pthread
# LDFLAGS = -lrt -pthread -lhwloc -lboost_program_options -lunwind

# # Google Test 库路径
# GTEST_DIR = /home/lifd/googletest/build
# GTEST_LIB = $(GTEST_DIR)/lib/libgtest_main.a $(GTEST_DIR)/lib/libgtest.a

# #---------------------------------------------------------------------------------------------------------------

# # 源文件
# SRCS = main.cpp
# # RESOURCE_SRCS = include/resource/resource.cc
# # MEMORY_SRCS = include/mem/memory.cc
# # FD_SRCS = include/fd/posix.cc
# # UTIL_SRCS = include/util/backtrace.cc
# # TEST_SRCS = test/future_test.cc test/task_test.cc test/future_promise_test.cc test/timer_test.cc test/shared_ptr_test.cc test/resource_test.cc test/mem_test.cc

# #---------------------------------------------------------------------------------------------------------------

# # 目标文件
# OBJS = $(SRCS:.cpp=.o)
# RESOURCE_OBJS = $(RESOURCE_SRCS:.cc=.o)
# MEMORY_OBJS = $(MEMORY_SRCS:.cc=.o)
# FD_OBJS = $(FD_SRCS:.cc=.o)
# UTIL_OBJS = $(UTIL_SRCS:.cc=.o)

# # 可执行文件
# TARGET = demo
# TEST_TARGETS = $(basename $(notdir $(TEST_SRCS)))

# # 默认目标
# all: $(TARGET) $(TEST_TARGETS)

# # 链接生成主程序
# $(TARGET): $(OBJS) $(RESOURCE_OBJS) $(MEMORY_OBJS) $(FD_OBJS) $(UTIL_OBJS)
# 	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# # 模式规则：生成每个测试程序
# $(TEST_TARGETS): %: test/%.o $(RESOURCE_OBJS) $(MEMORY_OBJS) $(FD_OBJS) $(UTIL_OBJS)
# 	$(CXX) $(CXXFLAGS) -o $@ $^ $(GTEST_LIB) $(LDFLAGS)

# # 编译测试文件为对象文件
# test/%.o: test/%.cc
# 	$(CXX) $(CXXFLAGS) -c $< -o $@

# # 编译主程序源文件
# %.o: %.cpp
# 	$(CXX) $(CXXFLAGS) -c $< -o $@

# %.o: %.cc
# 	$(CXX) $(CXXFLAGS) -c $< -o $@

# # 清理生成的文件
# clean:
# 	rm -f $(OBJS) $(RESOURCE_OBJS) $(MEMORY_OBJS) $(FD_OBJS) $(UTIL_OBJS) $(TARGET) $(TEST_TARGETS) test/*.o

# # 运行所有测试
# test: $(TEST_TARGETS)
# 	@for t in $(TEST_TARGETS); do \
# 		echo "Running $$t..."; \
# 		./$$t; \
# 	done

# .PHONY: all clean test



# # 编译器
# CXX = g++
# # 编译选项
# CXXFLAGS = -Wall -std=c++20 -g -Iinclude -pthread
# LDFLAGS = -lrt -pthread -lboost_system
# MEMORY_SRCS = include/future/future_all6.cc
# # 测试源文件
# TEST_SRCS = test/new_tests/future_promise_test.cc
# # 可执行文件名称
# TEST_TARGET = future_promise_test

# # 默认目标
# all: $(TEST_TARGET)

# # 编译和链接测试程序
# $(TEST_TARGET): $(TEST_SRCS)
# 	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# # 清理生成的文件
# clean:
# 	rm -f $(TEST_TARGET)

# # 运行测试
# test: $(TEST_TARGET)
# 	./$(TEST_TARGET)
# .PHONY: all clean test
# # 编译器
# CXX = g++
# # 编译选项
# CXXFLAGS = -Wall -std=c++20 -g -Iinclude -pthread -MMD
# LDFLAGS = -pthread -lboost_system -lboost_thread -lrt -lhwloc -lboost_program_options -laio  -lunwind

# # # 源文件
# # SRCS = \
# #     include/future/future_all7.cc \
# #     include/resource/resource.cc \
# #     test/new_tests/future_promise_test.cc

# SRCS = \
#     include/resource/resource.cc \
#     test/new_tests/future_promise_test.cc \
# 	include/util/backtrace.cc

# # 生成对象文件列表
# OBJS = $(SRCS:%.cc=build/%.o)
# DEPS = $(OBJS:.o=.d)

# # 可执行文件名称
# TEST_TARGET = build/future_promise_test
# # 默认目标
# all: $(TEST_TARGET)
# # 包含自动生成的依赖
# -include $(DEPS)
# # 创建构建目录结构
# $(shell mkdir -p $(dir $(OBJS)) 2>/dev/null)
# # 编译规则：将.cc文件编译为.o文件
# build/%.o: %.cc
# 	$(CXX) $(CXXFLAGS) -c $< -o $@
# # 链接可执行文件
# $(TEST_TARGET): $(OBJS)
# 	$(CXX) $^ -o $@ $(LDFLAGS)
# # 清理生成的文件
# clean:
# 	rm -rf build
# # 运行测试
# test: $(TEST_TARGET)
# 	./$(TEST_TARGET)
# .PHONY: all clean test

# 编译器
CXX = g++
# 编译选项
CXXFLAGS = -Wall -std=c++20 -g -Iinclude -pthread -MMD
LDFLAGS = -pthread -lboost_system -lboost_thread -lrt -lhwloc -lboost_program_options -laio  -lunwind

# 源文件
# SRCS = \
#     include/future/future_all7.cc \
#     include/resource/resource.cc \
#     test/new_tests/future_promise_test.cc

SRCS = \
    include/resource/resource.cc \
    test/new_tests/smp_test.cc \
	include/util/backtrace.cc \
	include/app/app-template.cc
	

# 生成对象文件列表
OBJS = $(SRCS:%.cc=build/%.o)
DEPS = $(OBJS:.o=.d)

# 可执行文件名称
TEST_TARGET = build/smp_test

# 默认目标
all: $(TEST_TARGET)

# 包含自动生成的依赖
-include $(DEPS)

# 创建构建目录结构
$(shell mkdir -p $(dir $(OBJS)) 2>/dev/null)

# 编译规则：将.cc文件编译为.o文件
build/%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 链接可执行文件
$(TEST_TARGET): $(OBJS)
	$(CXX) $^ -o $@ $(LDFLAGS)

# 清理生成的文件
clean:
	rm -rf build

# 运行测试
test: $(TEST_TARGET)
	./$(TEST_TARGET)

.PHONY: all clean test