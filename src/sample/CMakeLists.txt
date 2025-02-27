file(GLOB SAMPLE_SOURCES *.cpp)

target_sources(sph
    PRIVATE
        ${SAMPLE_SOURCES}
)