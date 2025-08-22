CXX = g++
CXXFLAGS = -Wall -Wextra -Wconversion -Werror -Wpedantic -std=c++17 -O2
BUILD_DIR = builds

create_build_dir:
ifeq ($(OS), Windows_NT)
	if not exist $(BUILD_DIR) mkdir $(BUILD_DIR)
else
	mkdir -p $(BUILD_DIR)
endif

build: create_build_dir fec.cpp
	$(CXX) $(CXXFLAGS) fec.cpp -o $(BUILD_DIR)/fec

run: build
	./$(BUILD_DIR)/fec

clean:
ifeq ($(OS), Windows_NT)
	rd /s /q $(BUILD_DIR)
else
	rm -rf $(BUILD_DIR)
endif


.SILENT:
