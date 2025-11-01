# Sedov-Taylor blast wave benchmark

# Add source file to SPH library
target_sources(sph_lib PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/sedov_taylor.cpp
)
