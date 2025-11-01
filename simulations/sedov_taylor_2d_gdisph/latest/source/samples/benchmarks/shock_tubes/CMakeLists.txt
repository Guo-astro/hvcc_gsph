# Shock Tube benchmark samples
target_sources(sph_lib PRIVATE
    sod_shock_tube.cpp
    # sod_from_template.cpp  # TODO: Fix template API to match Simulation/SPHParameters actual interface
)
