file(GLOB production_sims_SOURCES *.cpp)
target_sources(sph_lib
    PRIVATE
        ${production_sims_SOURCES}
)
