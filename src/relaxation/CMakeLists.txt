# Production-level code for density relaxation, Lane–Emden, etc.

# Add these new sources to 'sph' target:
target_sources(sph
    PRIVATE
        lane_emden_relaxation.cpp
        # you can add more production-level .cpp files if needed
)

# If you had headers in include/relaxation/*, that’s fine; 
# no special lines needed here for them, just ensure they are #included properly.
