"""
Visualization and plotting tools for GSPH simulations.
"""

import numpy as np
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation, FFMpegWriter
from matplotlib.collections import PathCollection
from typing import List, Optional, Tuple, Callable
from pathlib import Path

# Support both package and script imports
try:
    from .readers import ParticleSnapshot, EnergyHistory, SimulationReader
    from .conservation import ConservationReport
    from .theoretical import ShockTubeSolution
except ImportError:
    from readers import ParticleSnapshot, EnergyHistory, SimulationReader
    from conservation import ConservationReport
    from theoretical import ShockTubeSolution


class ParticlePlotter:
    """Tools for plotting particle data."""
    
    def __init__(self, figsize: Tuple[float, float] = (10, 6)):
        """
        Initialize plotter.
        
        Args:
            figsize: Figure size (width, height)
        """
        self.figsize = figsize
    
    def plot_1d(self, snapshot: ParticleSnapshot,
                quantity: str = 'dens',
                theory: Optional[ShockTubeSolution] = None,
                ax: Optional[plt.Axes] = None,
                **kwargs) -> plt.Axes:
        """
        Plot 1D particle data.
        
        Args:
            snapshot: Particle snapshot
            quantity: Quantity to plot ('dens', 'pres', 'vel', 'ene')
            theory: Optional theoretical solution to overlay
            ax: Matplotlib axes (creates new if None)
            **kwargs: Additional plot arguments
        
        Returns:
            Matplotlib axes
        """
        if snapshot.dim != 1:
            raise ValueError("This method is for 1D simulations only")
        
        if ax is None:
            fig, ax = plt.subplots(figsize=self.figsize)
        
        x = snapshot.pos[:, 0]
        
        # Get quantity to plot
        quantity_map = {
            'dens': (snapshot.dens, 'Density', r'$\rho$'),
            'pres': (snapshot.pres, 'Pressure', r'$P$'),
            'vel': (snapshot.vel[:, 0], 'Velocity', r'$v_x$'),
            'ene': (snapshot.ene, 'Energy', r'$u$'),
        }
        
        if quantity not in quantity_map:
            raise ValueError(f"Unknown quantity: {quantity}. Choose from {list(quantity_map.keys())}")
        
        y, ylabel, label = quantity_map[quantity]
        
        # Plot simulation
        ax.scatter(x, y, alpha=0.7, s=20, label='Simulation', **kwargs)
        
        # Plot theory if provided
        if theory is not None:
            theory_y = getattr(theory, quantity.replace('vel', 'vel'))
            if quantity == 'vel':
                theory_y = theory.vel  # velocity is array in theory
            ax.plot(theory.x, theory_y, 'r-', linewidth=2, label='Analytical', alpha=0.8)
        
        ax.set_xlabel('Position')
        ax.set_ylabel(label)
        ax.set_title(f'{ylabel} at t = {snapshot.time:.4f}')
        ax.legend()
        ax.grid(True, alpha=0.3)
        
        return ax
    
    def plot_2d_scatter(self, snapshot: ParticleSnapshot,
                        quantity: str = 'dens',
                        ax: Optional[plt.Axes] = None,
                        cmap: str = 'viridis',
                        **kwargs) -> Tuple[plt.Axes, PathCollection]:
        """
        Plot 2D particle scatter colored by quantity.
        
        Args:
            snapshot: Particle snapshot
            quantity: Quantity to color by
            ax: Matplotlib axes
            cmap: Colormap name
            **kwargs: Additional scatter arguments
        
        Returns:
            Tuple of (axes, scatter collection)
        """
        if snapshot.dim != 2:
            raise ValueError("This method is for 2D simulations only")
        
        if ax is None:
            fig, ax = plt.subplots(figsize=self.figsize)
        
        quantity_map = {
            'dens': (snapshot.dens, 'Density', r'$\rho$'),
            'pres': (snapshot.pres, 'Pressure', r'$P$'),
            'vel': (np.linalg.norm(snapshot.vel, axis=1), 'Speed', r'$|v|$'),
            'ene': (snapshot.ene, 'Energy', r'$u$'),
        }
        
        if quantity not in quantity_map:
            raise ValueError(f"Unknown quantity: {quantity}")
        
        c, label, cbar_label = quantity_map[quantity]
        
        scatter = ax.scatter(snapshot.pos[:, 0], snapshot.pos[:, 1],
                            c=c, cmap=cmap, s=10, **kwargs)
        
        ax.set_xlabel('x')
        ax.set_ylabel('y')
        ax.set_title(f'{label} at t = {snapshot.time:.4f}')
        ax.set_aspect('equal')
        
        cbar = plt.colorbar(scatter, ax=ax)
        cbar.set_label(cbar_label)
        
        return ax, scatter
    
    def plot_2d_grid(self, snapshot: ParticleSnapshot,
                     quantity: str = 'dens',
                     grid_size: int = 100,
                     ax: Optional[plt.Axes] = None,
                     cmap: str = 'viridis',
                     **kwargs) -> Tuple[plt.Axes, plt.cm.ScalarMappable]:
        """
        Plot 2D data interpolated to grid.
        
        Args:
            snapshot: Particle snapshot
            quantity: Quantity to plot
            grid_size: Number of grid points per dimension
            ax: Matplotlib axes
            cmap: Colormap
            **kwargs: Additional imshow arguments
        
        Returns:
            Tuple of (axes, image)
        """
        if snapshot.dim != 2:
            raise ValueError("This method is for 2D simulations only")
        
        from scipy.interpolate import griddata
        
        if ax is None:
            fig, ax = plt.subplots(figsize=self.figsize)
        
        quantity_map = {
            'dens': (snapshot.dens, 'Density', r'$\rho$'),
            'pres': (snapshot.pres, 'Pressure', r'$P$'),
            'vel': (np.linalg.norm(snapshot.vel, axis=1), 'Speed', r'$|v|$'),
            'ene': (snapshot.ene, 'Energy', r'$u$'),
        }
        
        values, label, cbar_label = quantity_map[quantity]
        
        # Create grid
        x_min, x_max = snapshot.pos[:, 0].min(), snapshot.pos[:, 0].max()
        y_min, y_max = snapshot.pos[:, 1].min(), snapshot.pos[:, 1].max()
        
        xi = np.linspace(x_min, x_max, grid_size)
        yi = np.linspace(y_min, y_max, grid_size)
        xi, yi = np.meshgrid(xi, yi)
        
        # Interpolate
        zi = griddata(snapshot.pos, values, (xi, yi), method='cubic', fill_value=0)
        
        im = ax.imshow(zi, extent=[x_min, x_max, y_min, y_max],
                      origin='lower', cmap=cmap, aspect='auto', **kwargs)
        
        ax.set_xlabel('x')
        ax.set_ylabel('y')
        ax.set_title(f'{label} at t = {snapshot.time:.4f}')
        
        cbar = plt.colorbar(im, ax=ax)
        cbar.set_label(cbar_label)
        
        return ax, im
    
    def plot_3d_slice(self, snapshot: ParticleSnapshot,
                      quantity: str = 'dens',
                      slice_axis: int = 2,
                      slice_position: float = 0.0,
                      thickness: float = 0.1,
                      ax: Optional[plt.Axes] = None,
                      **kwargs) -> Tuple[plt.Axes, PathCollection]:
        """
        Plot 3D data as 2D slice.
        
        Args:
            snapshot: Particle snapshot
            quantity: Quantity to plot
            slice_axis: Axis perpendicular to slice (0=x, 1=y, 2=z)
            slice_position: Position of slice along slice_axis
            thickness: Thickness of slice
            ax: Matplotlib axes
            **kwargs: Additional scatter arguments
        
        Returns:
            Tuple of (axes, scatter collection)
        """
        if snapshot.dim != 3:
            raise ValueError("This method is for 3D simulations only")
        
        if ax is None:
            fig, ax = plt.subplots(figsize=self.figsize)
        
        # Select particles in slice
        mask = np.abs(snapshot.pos[:, slice_axis] - slice_position) < thickness / 2
        
        # Get plot axes (perpendicular to slice_axis)
        axes = [0, 1, 2]
        axes.remove(slice_axis)
        ax1, ax2 = axes
        
        quantity_map = {
            'dens': (snapshot.dens, 'Density', r'$\rho$'),
            'pres': (snapshot.pres, 'Pressure', r'$P$'),
            'vel': (np.linalg.norm(snapshot.vel, axis=1), 'Speed', r'$|v|$'),
            'ene': (snapshot.ene, 'Energy', r'$u$'),
        }
        
        c, label, cbar_label = quantity_map[quantity]
        
        scatter = ax.scatter(snapshot.pos[mask, ax1], snapshot.pos[mask, ax2],
                            c=c[mask], s=10, **kwargs)
        
        axis_names = ['x', 'y', 'z']
        ax.set_xlabel(axis_names[ax1])
        ax.set_ylabel(axis_names[ax2])
        ax.set_title(f'{label} slice at {axis_names[slice_axis]}={slice_position:.2f}, t={snapshot.time:.4f}')
        ax.set_aspect('equal')
        
        cbar = plt.colorbar(scatter, ax=ax)
        cbar.set_label(cbar_label)
        
        return ax, scatter


class EnergyPlotter:
    """Tools for plotting energy and conservation data."""
    
    @staticmethod
    def plot_energy_history(energy: EnergyHistory,
                           ax: Optional[plt.Axes] = None,
                           show_components: bool = True) -> plt.Axes:
        """
        Plot energy evolution.
        
        Args:
            energy: Energy history
            ax: Matplotlib axes
            show_components: Show kinetic/thermal/potential components
        
        Returns:
            Matplotlib axes
        """
        if ax is None:
            fig, ax = plt.subplots(figsize=(10, 6))
        
        ax.plot(energy.time, energy.total, 'k-', linewidth=2, label='Total')
        
        if show_components:
            ax.plot(energy.time, energy.kinetic, '--', label='Kinetic', alpha=0.7)
            ax.plot(energy.time, energy.thermal, '--', label='Thermal', alpha=0.7)
            if not np.allclose(energy.potential, 0):
                ax.plot(energy.time, energy.potential, '--', label='Potential', alpha=0.7)
        
        ax.set_xlabel('Time')
        ax.set_ylabel('Energy')
        ax.set_title('Energy Evolution')
        ax.legend()
        ax.grid(True, alpha=0.3)
        
        return ax
    
    @staticmethod
    def plot_energy_error(energy: EnergyHistory,
                         ax: Optional[plt.Axes] = None) -> plt.Axes:
        """
        Plot relative energy error.
        
        Args:
            energy: Energy history
            ax: Matplotlib axes
        
        Returns:
            Matplotlib axes
        """
        if ax is None:
            fig, ax = plt.subplots(figsize=(10, 6))
        
        error = energy.relative_error()
        
        ax.plot(energy.time, error * 100, 'r-', linewidth=2)
        ax.axhline(0, color='k', linestyle='--', alpha=0.5)
        
        ax.set_xlabel('Time')
        ax.set_ylabel('Relative Error (%)')
        ax.set_title('Energy Conservation Error')
        ax.grid(True, alpha=0.3)
        
        return ax
    
    @staticmethod
    def plot_conservation_report(report: ConservationReport,
                                 figsize: Tuple[float, float] = (15, 10)) -> plt.Figure:
        """
        Plot full conservation report.
        
        Args:
            report: Conservation report
            figsize: Figure size
        
        Returns:
            Matplotlib figure
        """
        if report.angular_momentum is not None:
            fig, axes = plt.subplots(2, 2, figsize=figsize)
            axes = axes.flatten()
        else:
            fig, axes = plt.subplots(1, 3, figsize=(15, 5))
        
        # Mass conservation
        axes[0].plot(report.time, report.mass_error * 100, 'b-', linewidth=2)
        axes[0].axhline(0, color='k', linestyle='--', alpha=0.5)
        axes[0].set_xlabel('Time')
        axes[0].set_ylabel('Relative Error (%)')
        axes[0].set_title('Mass Conservation')
        axes[0].grid(True, alpha=0.3)
        
        # Momentum conservation
        axes[1].plot(report.time, report.momentum_error * 100, 'g-', linewidth=2)
        axes[1].axhline(0, color='k', linestyle='--', alpha=0.5)
        axes[1].set_xlabel('Time')
        axes[1].set_ylabel('Relative Error (%)')
        axes[1].set_title('Momentum Conservation')
        axes[1].grid(True, alpha=0.3)
        
        # Energy conservation
        axes[2].plot(report.time, report.energy_error * 100, 'r-', linewidth=2)
        axes[2].axhline(0, color='k', linestyle='--', alpha=0.5)
        axes[2].set_xlabel('Time')
        axes[2].set_ylabel('Relative Error (%)')
        axes[2].set_title('Energy Conservation')
        axes[2].grid(True, alpha=0.3)
        
        # Angular momentum (if available)
        if report.angular_momentum is not None:
            axes[3].plot(report.time, report.angular_momentum_error * 100, 'm-', linewidth=2)
            axes[3].axhline(0, color='k', linestyle='--', alpha=0.5)
            axes[3].set_xlabel('Time')
            axes[3].set_ylabel('Relative Error (%)')
            axes[3].set_title('Angular Momentum Conservation')
            axes[3].grid(True, alpha=0.3)
        
        plt.tight_layout()
        return fig


class AnimationMaker:
    """Create animations from simulation snapshots."""
    
    def __init__(self, reader: SimulationReader):
        """
        Initialize animation maker.
        
        Args:
            reader: Simulation reader
        """
        self.reader = reader
    
    def animate_1d(self, quantity: str = 'dens',
                   output_file: Optional[str] = None,
                   fps: int = 10,
                   interval: int = 1,
                   theory_func: Optional[Callable] = None,
                   **kwargs) -> FuncAnimation:
        """
        Create 1D animation.
        
        Args:
            quantity: Quantity to animate
            output_file: Output filename (MP4)
            fps: Frames per second
            interval: Use every Nth snapshot
            theory_func: Function(snapshot) -> theory solution
            **kwargs: Additional plot arguments
        
        Returns:
            FuncAnimation object
        """
        snapshots = self.reader.read_all_snapshots()[::interval]
        
        fig, ax = plt.subplots(figsize=(10, 6))
        plotter = ParticlePlotter()
        
        # Determine y-axis limits
        all_values = []
        for snap in snapshots:
            if quantity == 'dens':
                all_values.append(snap.dens)
            elif quantity == 'pres':
                all_values.append(snap.pres)
            elif quantity == 'vel':
                all_values.append(snap.vel[:, 0])
            elif quantity == 'ene':
                all_values.append(snap.ene)
        
        y_min = min([v.min() for v in all_values])
        y_max = max([v.max() for v in all_values])
        y_range = y_max - y_min
        
        def animate(frame):
            ax.clear()
            snap = snapshots[frame]
            theory = theory_func(snap) if theory_func is not None else None
            plotter.plot_1d(snap, quantity, theory=theory, ax=ax, **kwargs)
            ax.set_ylim(y_min - 0.1 * y_range, y_max + 0.1 * y_range)
            return ax,
        
        anim = FuncAnimation(fig, animate, frames=len(snapshots), interval=1000//fps, blit=False)
        
        if output_file is not None:
            writer = FFMpegWriter(fps=fps, bitrate=1800)
            anim.save(output_file, writer=writer)
            print(f"Animation saved to {output_file}")
        
        return anim
    
    def animate_2d(self, quantity: str = 'dens',
                   output_file: Optional[str] = None,
                   fps: int = 10,
                   interval: int = 1,
                   mode: str = 'scatter',
                   **kwargs) -> FuncAnimation:
        """
        Create 2D animation.
        
        Args:
            quantity: Quantity to animate
            output_file: Output filename (MP4)
            fps: Frames per second
            interval: Use every Nth snapshot
            mode: 'scatter' or 'grid'
            **kwargs: Additional plot arguments
        
        Returns:
            FuncAnimation object
        """
        snapshots = self.reader.read_all_snapshots()[::interval]
        
        fig, ax = plt.subplots(figsize=(10, 8))
        plotter = ParticlePlotter()
        
        # Determine color limits
        all_values = []
        for snap in snapshots:
            if quantity == 'dens':
                all_values.append(snap.dens)
            elif quantity == 'pres':
                all_values.append(snap.pres)
            elif quantity == 'vel':
                all_values.append(np.linalg.norm(snap.vel, axis=1))
            elif quantity == 'ene':
                all_values.append(snap.ene)
        
        vmin = min([v.min() for v in all_values])
        vmax = max([v.max() for v in all_values])
        
        def animate(frame):
            ax.clear()
            snap = snapshots[frame]
            if mode == 'scatter':
                plotter.plot_2d_scatter(snap, quantity, ax=ax, vmin=vmin, vmax=vmax, **kwargs)
            else:
                plotter.plot_2d_grid(snap, quantity, ax=ax, vmin=vmin, vmax=vmax, **kwargs)
            return ax,
        
        anim = FuncAnimation(fig, animate, frames=len(snapshots), interval=1000//fps, blit=False)
        
        if output_file is not None:
            writer = FFMpegWriter(fps=fps, bitrate=1800)
            anim.save(output_file, writer=writer)
            print(f"Animation saved to {output_file}")
        
        return anim
