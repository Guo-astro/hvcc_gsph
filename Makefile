.PHONY: all build run clean

all: build run

build: build/CMakeCache.txt
	@echo "Building project..."
	$(MAKE) -C build

# Only run CMake if the cache is missing.
build/CMakeCache.txt:
	@echo "Creating build directory..."
	mkdir -p build
	@echo "Configuring project with CMake..."
	cmake -B build \
		-DOpenMP_C_FLAGS="-Xpreprocessor -fopenmp /opt/homebrew/opt/libomp/lib/libomp.dylib -I/opt/homebrew/opt/libomp/include" \
		-DOpenMP_CXX_FLAGS="-Xpreprocessor -fopenmp /opt/homebrew/opt/libomp/lib/libomp.dylib -I/opt/homebrew/opt/libomp/include" \
		-DOpenMP_C_LIB_NAMES=libomp \
		-DOpenMP_CXX_LIB_NAMES=libomp \
		-DOpenMP_libomp_LIBRARY="/opt/homebrew/opt/libomp/lib/libomp.dylib"
	@echo "Configuration complete."

run:
	@echo "Running shock_tube..."
	./build/sph shock_tube ./sample/shock_tube/shock_tube.json 1

clean:
	@echo "Cleaning build directory..."
	rm -rf build
