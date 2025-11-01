# Top-level headers that remain in include/
target_sources(sph_lib
    PUBLIC
        lane_emden.hpp
        lane_emden_2d.hpp
)

# Add subdirectories containing organized headers
add_subdirectory(core)
add_subdirectory(modules)
add_subdirectory(tree)
add_subdirectory(algorithms)
add_subdirectory(utilities)
add_subdirectory(kernel)
add_subdirectory(relaxation)
