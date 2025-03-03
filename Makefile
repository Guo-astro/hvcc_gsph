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
	@echo "Removing result folder"
	rm -rf result
	@echo "Running shock_tube..."

	# ./build/sph shock_tube ./sample/shock_tube/shock_tube.json 8
	# ./build/sph evrard ./sample/evrard/evrard.json 8
	./build/sph lane_emden ./sample/lane_emden/lane_emden.json 8
	# ./build/sph khi ./sample/khi/khi.json 8
	# ./build/sph shock_tube_2d ./sample/shock_tube_2d/shock_tube_2d.json 8
	# ./build/sph shock_tube_2p5d ./sample/shock_tube_2p5d/shock_tube_2p5d.json 8
	# ./build/sph shock_tube_strong_shock ./sample/shock_tube_strong_shock/shock_tube_strong_shock.json 8



clean:
	@echo "Cleaning build directory..."
	rm -rf build
