###############################################################################
# Makefile for SPH Project
#
# This Makefile creates an out-of-source build in the "build_manual" folder.
# It also defines individual run targets for each sample based on the JSON
# files located in your project.
#
# Targets:
#   all                           : Build project and run the default sample (kernel_test)
#   build                         : Configure and build the project
#   run                           : Run the default executable (kernel_test)
#
# Individual sample run targets (examples):
#   run_kernel_test               : Run "kernel_test"
#   run_shock_tube                : Run "shock_tube" with its JSON and 8 threads
#   run_evrard                    : Run "evrard" sample, etc.
#
# To clean the build directory, run: make clean
###############################################################################

.PHONY: all build run clean run_kernel_test run_shock_tube run_evrard run_lane_emden run_thin_disk_3d run_thin_slice_poly_2_5d run_thin_slice_poly_2_5d_restart run_khi run_shock_tube_2d run_shock_tube_2p5d run_shock_tube_strong_shock run_sedov_taylor run_vacuum_test run_purtabation_damping

# Build and then run the default sample.
all: build run

# Build target: create the build directory (if needed), run CMake with the proper OpenMP flags, then build.
build: build_manual/CMakeCache.txt
	@echo "Building project in build_manual..."
	$(MAKE) -C build_manual

# Create build directory and configure with CMake.
build_manual/CMakeCache.txt:
	@echo "Creating build directory 'build_manual'..."
	mkdir -p build_manual
	@echo "Configuring project with CMake..."
	cmake -B build_manual \
		-DOpenMP_C_FLAGS="-Xpreprocessor -fopenmp /opt/homebrew/opt/libomp/lib/libomp.dylib -I/opt/homebrew/opt/libomp/include" \
		-DOpenMP_CXX_FLAGS="-Xpreprocessor -fopenmp /opt/homebrew/opt/libomp/lib/libomp.dylib -I/opt/homebrew/opt/libomp/include" \
		-DOpenMP_C_LIB_NAMES=libomp \
		-DOpenMP_CXX_LIB_NAMES=libomp \
		-DOpenMP_libomp_LIBRARY="/opt/homebrew/opt/libomp/lib/libomp.dylib"
	@echo "Configuration complete."

# Default run target: runs kernel_test sample.
run: run_kernel_test

# Run sample: kernel_test (default)
run_kernel_test:
	@echo "Running kernel_test sample..."
	./build_manual/sph kernel_test

# Run sample: razor_thin_hvcc
run_razor_thin_hvcc: build
	@echo "Running razor_thin_hvcc sample..."
	./build_manual/sph razor_thin_hvcc ./production_sims/razor_thin_hvcc/razor_thin_hvcc.json 8



# Run sample: run_razor_thin_sg_relaxation
run_razor_thin_sg_relaxation: build
	@echo "Running razor_thin_sg_relaxation sample..."
	./build_manual/sph razor_thin_sg_relaxation ./production_sims/razor_thin_sg_relaxation/razor_thin_sg_relaxation.json 8


# Run sample: thin_slice_poly_2_5d_relax
run_thin_slice_poly_2_5d_relax: build
	@echo "Running thin_slice_poly_2_5d_relax sample..."
	./build_manual/sph thin_slice_poly_2_5d_relax ./sample/thin_slice_poly_2_5d_relax/thin_slice_poly_2_5d_relax.json 8



# Run sample: shock_tube
run_shock_tube:
	@echo "Running shock_tube sample..."
	./build_manual/sph shock_tube ./sample/shock_tube/shock_tube.json 8

# Run sample: evrard
run_evrard:
	@echo "Running evrard sample..."
	./build_manual/sph evrard ./sample/evrard/evrard.json 8

# Run sample: lane_emden
run_lane_emden:
	@echo "Running lane_emden sample..."
	./build_manual/sph lane_emden ./sample/lane_emden/lane_emden.json 8

# Run sample: thin_disk_3d
run_thin_disk_3d:
	@echo "Running thin_disk_3d sample..."
	./build_manual/sph thin_disk_3d ./sample/thin_disk_3d/thin_disk_3d.json 8

# Run sample: thin_slice_poly_2_5d
run_thin_slice_poly_2_5d:
	@echo "Running thin_slice_poly_2_5d sample..."
	./build_manual/sph thin_slice_poly_2_5d ./sample/thin_slice_poly_2_5d/thin_slice_poly_2_5d.json 8

# Run sample: thin_slice_poly_2_5d_restart (restart from checkpoint)
run_thin_slice_poly_2_5d_restart:
	@echo "Running thin_slice_poly_2_5d restart sample..."
	./build_manual/sph thin_slice_poly_2_5d ./sample/thin_slice_poly_2_5d/thin_slice_poly_2_5d_restart.json 8

# Run sample: khi
run_khi:
	@echo "Running khi sample..."
	./build_manual/sph khi ./sample/khi/khi.json 8

# Run sample: shock_tube_2d
run_shock_tube_2d:
	@echo "Running shock_tube_2d sample..."
	./build_manual/sph shock_tube_2d ./sample/shock_tube_2d/shock_tube_2d.json 8

# Run sample: shock_tube_2p5d
run_shock_tube_2p5d:
	@echo "Running shock_tube_2p5d sample..."
	./build_manual/sph shock_tube_2p5d ./sample/shock_tube_2p5d/shock_tube_2p5d.json 8


# Run sample: shock_tube_strong_shock
run_shock_tube_strong_shock:
	@echo "Running shock_tube_strong_shock sample..."
	./build_manual/sph shock_tube_strong_shock ./sample/shock_tube_strong_shock/shock_tube_strong_shock.json 8

# Run sample: sedov_taylor
run_sedov_taylor:
	@echo "Running sedov_taylor sample..."
	./build_manual/sph sedov_taylor ./sample/sedov_taylor/sedov_taylor.json 8

# Run sample: vacuum_test
run_vacuum_test:
	@echo "Running vacuum_test sample..."
	./build_manual/sph vacuum_test ./sample/vacuum_test/vacuum_test.json 8

# Run sample: purtabation_damping
run_purtabation_damping:
	@echo "Running purtabation_damping sample..."
	./build_manual/sph purtabation_damping ./sample/purtabation_damping/purtabation_damping.json 8

# Clean target: remove the build_manual directory.
clean:
	@echo "Cleaning build directory..."
	rm -rf build_manual
