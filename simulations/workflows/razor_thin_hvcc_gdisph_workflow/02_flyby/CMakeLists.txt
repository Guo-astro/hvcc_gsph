cmake_minimum_required(VERSION 3.10)

# Add the simulation plugin
add_library(flyby_plugin SHARED src/plugin.cpp)

target_link_libraries(razor_thin_hvcc_gdisph_plugin
    sph_lib
)

set_target_properties(razor_thin_hvcc_gdisph_plugin PROPERTIES
    PREFIX ""
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/simulations/razor_thin_hvcc_gdisph"
)

# Install plugin
install(TARGETS razor_thin_hvcc_gdisph_plugin
    LIBRARY DESTINATION simulations/razor_thin_hvcc_gdisph
)

# Install config files
install(FILES
    config.json
    config_resume.json
    README.md
    DESTINATION simulations/razor_thin_hvcc_gdisph
)
