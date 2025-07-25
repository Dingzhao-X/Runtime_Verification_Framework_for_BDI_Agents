# 静态库构建 Makefile - 修复版
CXX = g++
CXXFLAGS = -std=c++17 -Wall -O2 -Iinclude -static-libgcc -static-libstdc++

# 目录定义
SRC_DIR = src
INC_DIR = include
OBJ_DIR = object

# 静态库名称
LIB_NAME = libtra_processor.a

# 源文件
SOURCES = $(SRC_DIR)/tra_to_pm_converter.cpp \
          $(SRC_DIR)/tra_updater.cpp \
          $(SRC_DIR)/prism_verifier.cpp \
          $(SRC_DIR)/integrated_tool.cpp \
          $(SRC_DIR)/file_lock.cpp \
          $(SRC_DIR)/log_manager.cpp

# 目标文件（放在object目录中）
OBJECTS = $(OBJ_DIR)/tra_to_pm_converter.o \
          $(OBJ_DIR)/tra_updater.o \
          $(OBJ_DIR)/prism_verifier.o \
          $(OBJ_DIR)/integrated_tool.o \
          $(OBJ_DIR)/file_lock.o \
          $(OBJ_DIR)/log_manager.o

# 头文件
HEADERS = $(INC_DIR)/common_types.h \
          $(INC_DIR)/tra_to_pm_converter.h \
          $(INC_DIR)/tra_updater.h \
          $(INC_DIR)/prism_verifier.h \
          $(INC_DIR)/integrated_tool.h \
          $(INC_DIR)/file_lock.h \
          $(INC_DIR)/log_manager.h

.PHONY: all clean install help dirs

# 默认目标：构建静态库
all: dirs $(LIB_NAME)

# 创建目录
dirs:
	@if not exist "$(OBJ_DIR)" mkdir "$(OBJ_DIR)"

# 构建静态库
$(LIB_NAME): dirs $(OBJECTS)
	ar rcs $@ $(OBJECTS)
	@echo Static library $(LIB_NAME) created successfully with all 4 components!

# 编译目标文件规则
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 清理
clean:
	@if exist "$(OBJ_DIR)\*.o" del /q "$(OBJ_DIR)\*.o"
	@if exist "$(LIB_NAME)" del /q "$(LIB_NAME)"
	@echo Cleaned build files

# 安装到父目录（方便使用）
install: $(LIB_NAME)
	copy $(LIB_NAME) ..\
	xcopy /Y /S include ..\include\
	@echo Library and headers installed to parent directory

# 显示帮助
help:
	@echo Available targets:
	@echo   all     - Build the static library
	@echo   clean   - Remove build files  
	@echo   install - Copy library and headers to parent directory
	@echo   dirs    - Create necessary directories
	@echo   help    - Show this help
	@echo.
	@echo Current configuration:
	@echo   Compiler: $(CXX)
	@echo   Flags: $(CXXFLAGS)
	@echo   Library: $(LIB_NAME) 