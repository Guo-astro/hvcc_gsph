.PHONY: all build run clean

all: build run

build:
	@echo "Cleaning previous build directory..."
	rm -rf build
	@echo "Creating build directory..."
	mkdir build
	@echo "Configuring project with CMake..."
	cmake -B build \
		-DOpenMP_C_FLAGS=-fopenmp=lomp \
		-DOpenMP_CXX_FLAGS=-fopenmp=lomp \
		-DOpenMP_C_LIB_NAMES="libomp" \
		-DOpenMP_CXX_LIB_NAMES="libomp" \
		-DOpenMP_libomp_LIBRARY="/opt/homebrew/opt/libomp//lib/libomp.dylib" \
		-DOpenMP_CXX_FLAGS="-Xpreprocessor -fopenmp /opt/homebrew/opt/libomp/lib/libomp.dylib -I/opt/homebrew/opt/libomp/include" \
		-DOpenMP_CXX_LIB_NAMES="libomp" \
		-DOpenMP_C_FLAGS="-Xpreprocessor -fopenmp /opt/homebrew/opt/libomp/lib/libomp.dylib -I/opt/homebrew/opt/libomp/include"
	@echo "Building project..."
	$(MAKE) -C build

run:
	@echo "Running shock_tube..."
	./build/sph shock_tube_astro_unit 1

clean:
	@echo "Cleaning build directory..."
	rm -rf build
