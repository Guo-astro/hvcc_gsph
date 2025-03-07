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
        simulation.cpp
        solver.cpp
        timestep.cpp
        sample_registry.cpp
        heating_cooling.cpp
        inplane_integration.cpp
    )

add_subdirectory(disph)
add_subdirectory(gdisph)
add_subdirectory(gsph)
add_subdirectory(sample)
file(COPY ${CMAKE_SOURCE_DIR}/sample
     DESTINATION ${CMAKE_BINARY_DIR})
