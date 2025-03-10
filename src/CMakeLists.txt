target_sources(sph
    PRIVATE
        main.cpp
        bhtree.cpp
        exhaustive_search.cpp
        fluid_force.cpp
        gravity_force.cpp
        logger.cpp
        output.cpp
        pre_interaction.cpp
        sample_registry.cpp
        simulation.cpp
        solver.cpp
        timestep.cpp
        heating_cooling.cpp
        inplane_integration.cpp
)

add_subdirectory(disph)
add_subdirectory(gdisph)
add_subdirectory(gsph)
add_subdirectory(sample)

# Production folder for new simulation code:
add_subdirectory(relaxation)

# Copy sample data to build dir if needed
file(COPY ${CMAKE_SOURCE_DIR}/sample
     DESTINATION ${CMAKE_BINARY_DIR})


file(COPY ${CMAKE_SOURCE_DIR}/production_sims
     DESTINATION ${CMAKE_BINARY_DIR})


