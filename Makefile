.PHONY: all build run clean

all: build run

build: build_manual/CMakeCache.txt
	@echo "Building project..."
	$(MAKE) -C build_manual

# Only run CMake if the cache is missing.
build_manual/CMakeCache.txt:
	@echo "Creating build directory..."
	mkdir -p build_manual
	@echo "Configuring project with CMake..."
	cmake -B build_manual \
		-DOpenMP_C_FLAGS="-Xpreprocessor -fopenmp /opt/homebrew/opt/libomp/lib/libomp.dylib -I/opt/homebrew/opt/libomp/include" \
		-DOpenMP_CXX_FLAGS="-Xpreprocessor -fopenmp /opt/homebrew/opt/libomp/lib/libomp.dylib -I/opt/homebrew/opt/libomp/include" \
		-DOpenMP_C_LIB_NAMES=libomp \
		-DOpenMP_CXX_LIB_NAMES=libomp \
		-DOpenMP_libomp_LIBRARY="/opt/homebrew/opt/libomp/lib/libomp.dylib"
	@echo "Configuration complete."

run:
	@echo "Running shock_tube..."
	./build_manual/sph test_razor_thin_blast_wave ./production_sims/test_razor_thin_blast_wave/test_razor_thin_blast_wave.json 8

	# ./build_manual/sph shock_tube ./sample/shock_tube/shock_tube.json 8
	# ./build_manual/sph evrard ./sample/evrard/evrard.json 8
	# ./build_manual/sph lane_emden ./sample/lane_emden/lane_emden.json 8
	# ./build_manual/sph thin_disk_3d ./sample/thin_disk_3d/thin_disk_3d.json 8
	# ./build_manual/sph thin_slice_poly_2_5d ./sample/thin_slice_poly_2_5d/thin_slice_poly_2_5d.json 8
	# ./build_manual/sph thin_slice_poly_2_5d ./sample/thin_slice_poly_2_5d/thin_slice_poly_2_5d_restart.json 8
	./build_manual/sph thin_slice_poly_2_5d_relax ./sample/thin_slice_poly_2_5d_relax/thin_slice_poly_2_5d_relax.json 8
	# ./build_manual/sph khi ./sample/khi/khi.json 8
	# ./build_manual/sph shock_tube_2d ./sample/shock_tube_2d/shock_tube_2d.json 8
	# ./build_manual/sph shock_tube_2p5d ./sample/shock_tube_2p5d/shock_tube_2p5d.json 8
	# ./build_manual/sph shock_tube_strong_shock ./sample/shock_tube_strong_shock/shock_tube_strong_shock.json 8
	# ./build_manual/sph sedov_taylor ./sample/sedov_taylor/sedov_taylor.json 8
	# ./build_manual/sph vacuum_test ./sample/vacuum_test/vacuum_test.json 8
	# ./build_manual/sph purtabation_damping ./sample/purtabation_damping/purtabation_damping.json 8

	





clean:
	@echo "Cleaning build directory..."
	rm -rf build
