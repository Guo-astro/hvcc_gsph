"""Command-line interface tools for GSPH analysis toolkit."""

from .analyze import main as analyze_main
from .animate import main as animate_main

__all__ = ["analyze_main", "animate_main"]
