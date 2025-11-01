"""
GSPH Analysis and Visualization Toolkit

Tools for analyzing SPH simulation results including:
- Conservation checks (energy, momentum, angular momentum)
- Theoretical comparisons (shock tubes, Sedov-Taylor, etc.)
- Easy visualization and animation
"""

__version__ = "1.0.0"

from .readers import SimulationReader, ParticleSnapshot, EnergyHistory
from .conservation import ConservationAnalyzer
from .theoretical import TheoreticalComparison
from .plotting import ParticlePlotter, EnergyPlotter, AnimationMaker

__all__ = [
    'SimulationReader',
    'ParticleSnapshot',
    'EnergyHistory',
    'ConservationAnalyzer',
    'TheoreticalComparison',
    'ParticlePlotter',
    'EnergyPlotter',
    'AnimationMaker',
]
