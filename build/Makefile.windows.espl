# Windows兼容的编译Makefile - 场景A和B
CXX = g++
CXXFLAGS = -std=c++17 -Wall -O2 -static-libgcc -static-libstdc++

# 目录定义
LIB_DIR = ../lib
SCENARIO_A_DIR = ../scenario_a
SCENARIO_B_DIR = ../scenario_b
BUILD_DIR = .

# 静态库
LIB_PATH = $(LIB_DIR)/libtra_processor.a
LIB_INCLUDE = $(LIB_DIR)/include

# 目标文件
SCENARIO_A_TARGET = scenario_a_demo.exe
AGENT_SIMULATOR_TARGET = agent_simulator.exe
RUNTIME_VERIFIER_TARGET = runtime_verifier.exe

# 源文件
SCENARIO_A_SOURCE = $(SCENARIO_A_DIR)/scenario_a_demo.cpp
AGENT_SIMULATOR_SOURCE = $(SCENARIO_B_DIR)/agent_simulator.cpp
RUNTIME_VERIFIER_SOURCE = $(SCENARIO_B_DIR)/runtime_verifier.cpp

.PHONY: all scenario_a scenario_b clean help check_lib

# 默认目标：编译所有
all: check_lib scenario_a scenario_b

# 场景A目标
scenario_a: $(SCENARIO_A_TARGET) 

# 场景B目标
scenario_b: $(AGENT_SIMULATOR_TARGET) $(RUNTIME_VERIFIER_TARGET)

# 检查静态库是否存在
check_lib:
	@if not exist "$(LIB_PATH)" (echo ERROR: Static library not found at $(LIB_PATH) && echo Please run build_complete.bat first && exit /b 1)

# 编译scenario_a_demo.exe
$(SCENARIO_A_TARGET): $(SCENARIO_A_SOURCE) $(LIB_PATH)
	@echo Building $@ from $<...
	$(CXX) $(CXXFLAGS) -I$(LIB_INCLUDE) -o $@ $< -L$(LIB_DIR) -ltra_processor
	@echo ^ $@ built successfully!

# 编译agent_simulator.exe
$(AGENT_SIMULATOR_TARGET): $(AGENT_SIMULATOR_SOURCE) $(LIB_PATH)
	@echo Building $@ from $<...
	$(CXX) $(CXXFLAGS) -I$(LIB_INCLUDE) -o $@ $< -L$(LIB_DIR) -ltra_processor
	@echo ^ $@ built successfully!

# 编译runtime_verifier.exe
$(RUNTIME_VERIFIER_TARGET): $(RUNTIME_VERIFIER_SOURCE) $(LIB_PATH)
	@echo Building $@ from $<...
	$(CXX) $(CXXFLAGS) -I$(LIB_INCLUDE) -o $@ $< -L$(LIB_DIR) -ltra_processor
	@echo ^ $@ built successfully!

# 清理目标文件
clean:
	@if exist "*.exe" del /q "*.exe"
	@echo Cleaned all executables from build directory

# 显示帮助
help:
	@echo ========================================
	@echo     Build System for Scenarios A and B
	@echo ========================================
	@echo.
	@echo Available targets:
	@echo   all         - Build all executables (scenario_a + scenario_b)
	@echo   scenario_a  - Build scenario A programs (demo + test)
	@echo   scenario_b  - Build scenario B programs (agent + verifier)
	@echo   clean       - Remove all executables
	@echo   help        - Show this help
	@echo.
	@echo Individual targets:
	@echo   $(SCENARIO_A_TARGET)  - Full integrated scenario A tool
	@echo   $(AGENT_SIMULATOR_TARGET)   - Real-time agent simulator
	@echo   $(RUNTIME_VERIFIER_TARGET)   - Real-time runtime verifier
	@echo.
	@echo Prerequisites:
	@echo   - Static library must exist at: $(LIB_PATH)
	@echo   - Run build_complete.bat first if library is missing
	@echo.
	@echo Usage examples:
	@echo   make -f Makefile all
	@echo   make -f Makefile scenario_a
	@echo   make -f Makefile scenario_b
	@echo   make -f Makefile clean

# 显示构建状态
status:
	@echo ========================================
	@echo           Build Status
	@echo ========================================
	@echo.
	@echo Static Library:
	@if exist "$(LIB_PATH)" (echo   ^ $(LIB_PATH) - EXISTS) else (echo   X $(LIB_PATH) - MISSING)
	@echo.
	@echo Executables in build directory:
	@if exist "$(SCENARIO_A_TARGET)" (echo   ^ $(SCENARIO_A_TARGET) - EXISTS) else (echo   - $(SCENARIO_A_TARGET) - not built)
	@if exist "$(AGENT_SIMULATOR_TARGET)" (echo   ^ $(AGENT_SIMULATOR_TARGET) - EXISTS) else (echo   - $(AGENT_SIMULATOR_TARGET) - not built)
	@if exist "$(RUNTIME_VERIFIER_TARGET)" (echo   ^ $(RUNTIME_VERIFIER_TARGET) - EXISTS) else (echo   - $(RUNTIME_VERIFIER_TARGET) - not built)
	@echo.
	@echo To build missing executables, run: make -f Makefile all 