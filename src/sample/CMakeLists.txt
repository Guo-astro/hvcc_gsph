file(GLOB SAMPLE_SOURCES *.cpp)
target_sources(sph_lib
    PRIVATE
        ${SAMPLE_SOURCES}
)
