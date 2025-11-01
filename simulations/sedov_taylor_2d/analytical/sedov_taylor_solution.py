"""
Sedov-Taylor Blast Wave Analytical Solution (2D and 3D)

Computes the self-similar analytical solution for a point explosion in a uniform medium.
Supports both 2D (cylindrical) and 3D (spherical) geometries.

References:
- Sedov (1959) - "Similarity and Dimensional Methods in Mechanics"
- Taylor (1950) - "The Formation of a Blast Wave by a Very Intense Explosion"
- Kamm & Timmes (2007) - "On Efficient Generation of Numerically Robust Sedov Solutions"
"""

import numpy as np
from scipy.integrate import solve_ivp
from scipy.interpolate import interp1d
import matplotlib.pyplot as plt
from dataclasses import dataclass
from typing import Tuple, Optional
import argparse
import json
from pathlib import Path


@dataclass
class SedovParameters:
    """Physical parameters for Sedov-Taylor blast wave"""
    E0: float = 1.0      # Total explosion energy
    rho0: float = 1.0    # Background density
    gamma: float = 1.4   # Adiabatic index
    dimension: int = 2   # 2D or 3D
    
    @property
    def omega(self) -> float:
        """Geometry factor: ω = 3/(j+2) where j=0(plane), 1(cylinder), 2(sphere)"""
        return 3.0 / (self.dimension + 1)
    
    @property
    def alpha(self) -> float:
        """Exponent for shock radius: R ∝ t^α"""
        return 2.0 / (self.dimension + 2)


class SedovTaylorSolution:
    """
    Analytical solution for Sedov-Taylor blast wave.
    
    The solution is self-similar with similarity variable ξ = r/R(t),
    where R(t) is the shock radius.
    """
    
    def __init__(self, params: SedovParameters):
        self.params = params
        self._compute_similarity_solution()
    
    def _compute_similarity_solution(self):
        """
        Solve the similarity ODEs to get the solution profile.
        
        The solution is characterized by:
        - Similarity variable: ξ = r / R(t)
        - Self-similar profiles: V(ξ), Z(ξ), G(ξ)
        where V = velocity/shock_velocity, Z = density/background, G = pressure
        """
        gamma = self.params.gamma
        omega = self.params.omega
        j = self.params.dimension - 1  # j=0(1D), 1(2D), 2(3D)
        
        # Shock jump conditions (Rankine-Hugoniot)
        V_shock = 2.0 / (gamma + 1)        # Velocity behind shock
        Z_shock = (gamma + 1) / (gamma - 1)  # Density jump
        G_shock = 2.0 / (gamma + 1)        # Pressure coefficient
        
        # Integration from shock (ξ=1) inward to origin (ξ=0)
        xi_span = (1.0, 1e-8)  # Integrate backward from shock
        y0 = [V_shock, Z_shock, G_shock]
        
        def similarity_odes(xi, y):
            """
            Similarity ODEs for Sedov-Taylor solution.
            y = [V, Z, G] where:
            - V = v / (ξ * dR/dt)  (dimensionless velocity)
            - Z = ρ / ρ₀           (density ratio)
            - G = P / (ρ₀ (dR/dt)²) (dimensionless pressure)
            """
            V, Z, G = y
            
            # Denominators (protect against division by zero)
            denom1 = gamma * G - (gamma - 1) * V**2 / 2
            denom2 = V - xi * omega
            
            if abs(denom2) < 1e-10 or abs(denom1) < 1e-10:
                return [0, 0, 0]  # Singular point
            
            # ODEs
            dV_dxi = -(2*G/Z + j*V*(V - xi*omega)/xi) / denom2
            dZ_dxi = (2*Z*V/xi + j*Z*(V - xi*omega)/xi) / denom2
            dG_dxi = -(2*gamma*G*V/xi + j*G*(V - xi*omega)/xi) / denom2
            
            return [dV_dxi, dZ_dxi, dG_dxi]
        
        # Solve ODEs
        sol = solve_ivp(
            similarity_odes, 
            xi_span, 
            y0,
            method='RK45',
            dense_output=True,
            rtol=1e-8,
            atol=1e-10,
            max_step=0.01
        )
        
        # Extract solution
        xi_vals = np.linspace(0, 1, 1000)
        solution = sol.sol(xi_vals)
        
        self.xi = xi_vals
        self.V = solution[0]
        self.Z = solution[1]
        self.G = solution[2]
        
        # Store shock position in similarity coordinates
        self.xi_shock = 1.0
        
        # Create interpolators
        self._V_interp = interp1d(xi_vals, self.V, kind='cubic', fill_value='extrapolate')
        self._Z_interp = interp1d(xi_vals, self.Z, kind='cubic', fill_value='extrapolate')
        self._G_interp = interp1d(xi_vals, self.G, kind='cubic', fill_value='extrapolate')
    
    def shock_radius(self, t: float) -> float:
        """
        Compute shock radius at time t.
        
        R(t) = ξ₀ (E₀ / ρ₀)^(1/(j+2)) t^(2/(j+2))
        
        where ξ₀ is determined from energy integral.
        """
        j = self.params.dimension - 1
        alpha = self.params.alpha
        
        # Energy constant ξ₀ (depends on gamma and dimension)
        # For gamma=1.4, 2D: ξ₀ ≈ 1.033
        # For gamma=5/3, 3D: ξ₀ ≈ 1.152
        xi0 = self._compute_energy_constant()
        
        factor = (self.params.E0 / self.params.rho0)**(1.0 / (j + 2))
        return xi0 * factor * t**alpha
    
    def _compute_energy_constant(self) -> float:
        """
        Compute ξ₀ from energy integral.
        
        The total energy is:
        E = Cⱼ ∫₀^R [ ½ρv² + ρe ] rʲ dr
        
        where Cⱼ = 2π (2D), 4π (3D) is the geometric factor.
        """
        gamma = self.params.gamma
        j = self.params.dimension - 1
        
        # Numerical integration of energy
        # E = ρ₀ R² (dR/dt)² * integral_term
        # This determines ξ₀ implicitly
        
        # For common cases, use tabulated values
        if self.params.dimension == 2 and abs(gamma - 1.4) < 0.01:
            return 1.033
        elif self.params.dimension == 3 and abs(gamma - 5.0/3.0) < 0.01:
            return 1.152
        else:
            # General case: compute from energy integral
            integrand = self.Z * (0.5 * self.V**2 + self.G / (gamma - 1))
            integral = np.trapz(integrand * self.xi**j, self.xi)
            
            # Geometric factor
            if j == 1:  # 2D
                C_j = 2 * np.pi
            elif j == 2:  # 3D
                C_j = 4 * np.pi
            else:
                C_j = 1.0
            
            return (1.0 / (C_j * integral))**(1.0 / (j + 2))
    
    def solution_at_time(self, r: np.ndarray, t: float) -> dict:
        """
        Compute solution at radius r and time t.
        
        Returns:
            Dictionary with 'r', 'rho', 'v', 'P', 'e', 'R_shock'
        """
        R = self.shock_radius(t)
        xi = r / R
        
        # Shock velocity
        dR_dt = self.params.alpha * R / t if t > 0 else 0
        
        # Solution profiles
        V_xi = self._V_interp(xi)
        Z_xi = self._Z_interp(xi)
        G_xi = self._G_interp(xi)
        
        # Physical quantities
        rho = self.params.rho0 * Z_xi
        v = V_xi * xi * dR_dt
        P = self.params.rho0 * dR_dt**2 * G_xi
        e = P / ((self.params.gamma - 1) * rho)
        
        # Zero out values beyond shock
        mask = r <= R
        rho = np.where(mask, rho, self.params.rho0)
        v = np.where(mask, v, 0)
        P = np.where(mask, P, 1e-6)
        e = np.where(mask, e, 1e-6)
        
        return {
            'r': r,
            'rho': rho,
            'v': v,
            'P': P,
            'e': e,
            'R_shock': R,
            'time': t
        }
    
    def plot_profiles(self, t: float, r_max: float = 0.5, ax=None):
        """Plot density, velocity, and pressure profiles at time t"""
        r = np.linspace(0, r_max, 500)
        sol = self.solution_at_time(r, t)
        
        if ax is None:
            fig, ax = plt.subplots(1, 3, figsize=(15, 4))
        
        ax[0].plot(sol['r'], sol['rho'], 'b-', linewidth=2)
        ax[0].axvline(sol['R_shock'], color='r', linestyle='--', label=f"Shock: R={sol['R_shock']:.3f}")
        ax[0].set_xlabel('r')
        ax[0].set_ylabel('Density ρ')
        ax[0].set_title(f'Density at t={t:.3f}')
        ax[0].legend()
        ax[0].grid(True, alpha=0.3)
        
        ax[1].plot(sol['r'], sol['v'], 'g-', linewidth=2)
        ax[1].axvline(sol['R_shock'], color='r', linestyle='--')
        ax[1].set_xlabel('r')
        ax[1].set_ylabel('Velocity v')
        ax[1].set_title(f'Velocity at t={t:.3f}')
        ax[1].grid(True, alpha=0.3)
        
        ax[2].plot(sol['r'], sol['P'], 'orange', linewidth=2)
        ax[2].axvline(sol['R_shock'], color='r', linestyle='--')
        ax[2].set_xlabel('r')
        ax[2].set_ylabel('Pressure P')
        ax[2].set_title(f'Pressure at t={t:.3f}')
        ax[2].grid(True, alpha=0.3)
        
        plt.tight_layout()
        return ax


def compare_with_simulation(run_dir: Path, params: SedovParameters, snapshot: int = -1):
    """
    Compare analytical solution with simulation results.
    
    Args:
        run_dir: Path to simulation run directory
        params: Sedov parameters
        snapshot: Snapshot number (-1 for latest)
    """
    import sys
    sys.path.append(str(Path(__file__).parent / '../../analysis'))
    from binary_reader import read_binary_snapshot
    
    # Read metadata to get time
    metadata_path = run_dir / 'metadata.json'
    with open(metadata_path) as f:
        metadata = json.load(f)
    
    # Find snapshot files
    binary_dir = run_dir / 'outputs' / 'binary'
    snapshot_files = sorted(binary_dir.glob('*.sph'))
    
    if not snapshot_files:
        print(f"No snapshots found in {binary_dir}")
        return
    
    # Select snapshot
    snapshot_file = snapshot_files[snapshot] if snapshot < len(snapshot_files) else snapshot_files[-1]
    
    # Read simulation data
    sim_data = read_binary_snapshot(str(snapshot_file))
    
    # Get time from filename or metadata
    t = float(snapshot_file.stem) * metadata['output_time']
    
    # Compute radial distance
    if params.dimension == 2:
        r_sim = np.sqrt(sim_data['x']**2 + sim_data['y']**2)
    else:  # 3D
        r_sim = np.sqrt(sim_data['x']**2 + sim_data['y']**2 + sim_data['z']**2)
    
    # Analytical solution
    r_analytical = np.linspace(0, r_sim.max(), 500)
    solver = SedovTaylorSolution(params)
    sol = solver.solution_at_time(r_analytical, t)
    
    # Create comparison plot
    fig, axes = plt.subplots(2, 3, figsize=(18, 10))
    
    # Row 1: Radial profiles
    axes[0, 0].scatter(r_sim, sim_data['rho'], s=1, alpha=0.5, label='Simulation')
    axes[0, 0].plot(sol['r'], sol['rho'], 'r-', linewidth=2, label='Analytical')
    axes[0, 0].set_xlabel('r')
    axes[0, 0].set_ylabel('Density ρ')
    axes[0, 0].set_title(f'Density Comparison (t={t:.3f})')
    axes[0, 0].legend()
    axes[0, 0].grid(True, alpha=0.3)
    
    v_sim = np.sqrt(sim_data['vx']**2 + sim_data['vy']**2 + 
                   (sim_data['vz']**2 if 'vz' in sim_data else 0))
    axes[0, 1].scatter(r_sim, v_sim, s=1, alpha=0.5, label='Simulation')
    axes[0, 1].plot(sol['r'], sol['v'], 'r-', linewidth=2, label='Analytical')
    axes[0, 1].set_xlabel('r')
    axes[0, 1].set_ylabel('Velocity |v|')
    axes[0, 1].set_title(f'Velocity Comparison')
    axes[0, 1].legend()
    axes[0, 1].grid(True, alpha=0.3)
    
    axes[0, 2].scatter(r_sim, sim_data['P'], s=1, alpha=0.5, label='Simulation')
    axes[0, 2].plot(sol['r'], sol['P'], 'r-', linewidth=2, label='Analytical')
    axes[0, 2].set_xlabel('r')
    axes[0, 2].set_ylabel('Pressure P')
    axes[0, 2].set_title(f'Pressure Comparison')
    axes[0, 2].legend()
    axes[0, 2].grid(True, alpha=0.3)
    
    # Row 2: 2D spatial distribution (if 2D or 3D)
    if params.dimension >= 2:
        # Density map
        sc1 = axes[1, 0].scatter(sim_data['x'], sim_data['y'], 
                                c=sim_data['rho'], s=1, cmap='viridis')
        plt.colorbar(sc1, ax=axes[1, 0], label='ρ')
        axes[1, 0].set_aspect('equal')
        axes[1, 0].set_xlabel('x')
        axes[1, 0].set_ylabel('y')
        axes[1, 0].set_title('Density Distribution')
        
        # Velocity map
        sc2 = axes[1, 1].scatter(sim_data['x'], sim_data['y'], 
                                c=v_sim, s=1, cmap='plasma')
        plt.colorbar(sc2, ax=axes[1, 1], label='|v|')
        axes[1, 1].set_aspect('equal')
        axes[1, 1].set_xlabel('x')
        axes[1, 1].set_ylabel('y')
        axes[1, 1].set_title('Velocity Distribution')
        
        # Pressure map
        sc3 = axes[1, 2].scatter(sim_data['x'], sim_data['y'], 
                                c=sim_data['P'], s=1, cmap='inferno')
        plt.colorbar(sc3, ax=axes[1, 2], label='P')
        axes[1, 2].set_aspect('equal')
        axes[1, 2].set_xlabel('x')
        axes[1, 2].set_ylabel('y')
        axes[1, 2].set_title('Pressure Distribution')
    
    plt.tight_layout()
    
    # Save figure
    output_path = run_dir / 'analysis' / f'sedov_comparison_t{t:.3f}.png'
    output_path.parent.mkdir(exist_ok=True)
    plt.savefig(output_path, dpi=150, bbox_inches='tight')
    print(f"Saved comparison to {output_path}")
    
    plt.show()


def main():
    parser = argparse.ArgumentParser(description='Sedov-Taylor analytical solution')
    parser.add_argument('--dimension', type=int, default=2, choices=[2, 3],
                       help='Dimension (2 or 3)')
    parser.add_argument('--gamma', type=float, default=1.4,
                       help='Adiabatic index')
    parser.add_argument('--time', type=float, default=0.1,
                       help='Time to evaluate solution')
    parser.add_argument('--plot', action='store_true',
                       help='Plot analytical profiles')
    parser.add_argument('--run', type=str,
                       help='Compare with simulation run directory')
    parser.add_argument('--snapshot', type=int, default=-1,
                       help='Snapshot number to compare (-1 for latest)')
    
    args = parser.parse_args()
    
    params = SedovParameters(
        dimension=args.dimension,
        gamma=args.gamma
    )
    
    solver = SedovTaylorSolution(params)
    
    if args.run:
        run_path = Path(args.run)
        if not run_path.is_absolute():
            # Resolve relative to simulations directory
            base_dir = Path(__file__).parent.parent
            run_path = base_dir / args.run
        
        compare_with_simulation(run_path, params, args.snapshot)
    
    elif args.plot:
        solver.plot_profiles(args.time)
        plt.show()
    
    else:
        # Print solution info
        R = solver.shock_radius(args.time)
        print(f"Sedov-Taylor Solution ({args.dimension}D)")
        print(f"  γ = {params.gamma}")
        print(f"  E₀ = {params.E0}")
        print(f"  ρ₀ = {params.rho0}")
        print(f"  t = {args.time}")
        print(f"  R(t) = {R:.4f}")
        print(f"  Scaling exponent α = {params.alpha:.4f}")


if __name__ == '__main__':
    main()
