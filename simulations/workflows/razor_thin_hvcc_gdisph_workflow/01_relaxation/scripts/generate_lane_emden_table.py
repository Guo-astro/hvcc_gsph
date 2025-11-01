#!/usr/bin/env python3
"""
Lane-Emden Table Generator for 2D Polytropic Disk

Solves the Lane-Emden equation for a 2D polytrope (n=1.5) and generates
a table of xi (dimensionless radius) vs theta (dimensionless density).

The 2D Lane-Emden equation is:
    d²θ/dξ² + (1/ξ) dθ/dξ + θ^n = 0

with boundary conditions:
    θ(0) = 1
    dθ/dξ(0) = 0

For a polytropic index n=1.5, this describes a self-gravitating disk
in hydrostatic equilibrium.

Usage:
    python generate_lane_emden_table.py [--output FILE] [--n-points N] [--xi-max XI]

References:
    - Binney & Tremaine, "Galactic Dynamics" (2008), Chapter 4
    - Chandrasekhar, "An Introduction to the Study of Stellar Structure" (1939)
"""

import numpy as np
from scipy.integrate import solve_ivp
import argparse
import sys


def lane_emden_2d(xi, y, n):
    """
    Lane-Emden equation in 2D cylindrical symmetry.
    
    Args:
        xi: Dimensionless radial coordinate
        y: [theta, dtheta_dxi] state vector
        n: Polytropic index
    
    Returns:
        [dtheta_dxi, d2theta_dxi2] derivatives
    """
    theta, dtheta = y
    
    # Avoid division by zero at xi=0
    if xi < 1e-10:
        # Use L'Hôpital's rule: lim(xi->0) dtheta/xi = 0
        d2theta = -theta**n
    else:
        d2theta = -(1.0/xi) * dtheta - theta**n
    
    # Ensure theta stays non-negative
    if theta < 0:
        theta = 0
        dtheta = 0
        d2theta = 0
    
    return [dtheta, d2theta]


def find_first_zero(sol, n_points=10000):
    """
    Find the first zero of theta(xi) by interpolation.
    
    Args:
        sol: ODE solution object from solve_ivp
        n_points: Number of points for dense evaluation
    
    Returns:
        xi1: Location of first zero
    """
    xi_dense = np.linspace(sol.t[0], sol.t[-1], n_points)
    theta_dense = sol.sol(xi_dense)[0]
    
    # Find where theta crosses zero
    zero_crossings = np.where(np.diff(np.sign(theta_dense)))[0]
    
    if len(zero_crossings) == 0:
        print(f"WARNING: No zero crossing found. Integration may need to go further.")
        print(f"  Last xi: {sol.t[-1]:.6f}, Last theta: {sol.y[0][-1]:.6e}")
        return sol.t[-1]
    
    # Linear interpolation to find more accurate zero
    idx = zero_crossings[0]
    xi_a, xi_b = xi_dense[idx], xi_dense[idx+1]
    theta_a, theta_b = theta_dense[idx], theta_dense[idx+1]
    
    # Linear interpolation: xi1 where theta = 0
    xi1 = xi_a - theta_a * (xi_b - xi_a) / (theta_b - theta_a)
    
    return xi1


def solve_lane_emden(n=1.5, xi_max=20.0, n_points=10000, verbose=True):
    """
    Solve the 2D Lane-Emden equation.
    
    Args:
        n: Polytropic index (default 1.5 for gamma=5/3)
        xi_max: Maximum xi to integrate to
        n_points: Number of output points
        verbose: Print progress information
    
    Returns:
        xi: Array of dimensionless radii
        theta: Array of dimensionless densities
        xi1: Location of first zero
    """
    if verbose:
        print(f"Solving 2D Lane-Emden equation for n={n}")
        print(f"  Integrating from xi=0 to xi={xi_max}")
        print(f"  Output points: {n_points}")
    
    # Initial conditions: theta(0) = 1, dtheta/dxi(0) = 0
    y0 = [1.0, 0.0]
    
    # Integration points (dense for accuracy)
    xi_span = (1e-6, xi_max)  # Start slightly above zero to avoid singularity
    
    # Use RK45 (adaptive Runge-Kutta) for accuracy
    # Stop when theta becomes negative (first zero)
    def theta_positive(xi, y):
        return y[0]  # theta
    theta_positive.terminal = True
    theta_positive.direction = -1  # Detect theta going negative
    
    sol = solve_ivp(
        lambda xi, y: lane_emden_2d(xi, y, n),
        xi_span,
        y0,
        method='RK45',
        events=theta_positive,
        dense_output=True,
        max_step=0.01,
        rtol=1e-10,
        atol=1e-12
    )
    
    if not sol.success:
        print(f"ERROR: Integration failed: {sol.message}")
        sys.exit(1)
    
    # Find first zero
    xi1 = find_first_zero(sol, n_points=10000)
    
    if verbose:
        print(f"  First zero at xi1 = {xi1:.6f}")
        print(f"  Integration steps: {len(sol.t)}")
    
    # Generate output points up to first zero
    xi = np.linspace(1e-6, xi1, n_points)
    theta = sol.sol(xi)[0]
    
    # Ensure theta is non-negative (numerical errors near zero)
    theta = np.maximum(theta, 0.0)
    
    # Add exact zero at the end
    xi = np.append(xi, xi1)
    theta = np.append(theta, 0.0)
    
    if verbose:
        print(f"  Output table: {len(xi)} points")
        print(f"  theta(0) = {theta[0]:.10f} (should be 1.0)")
        print(f"  theta(xi1) = {theta[-1]:.10e} (should be 0.0)")
    
    return xi, theta, xi1


def save_table(filename, xi, theta, n=1.5, xi1=None):
    """
    Save Lane-Emden table to CSV file.
    
    Args:
        filename: Output CSV filename
        xi: Array of dimensionless radii
        theta: Array of dimensionless densities
        n: Polytropic index (for header comment)
        xi1: Location of first zero (for header comment)
    """
    with open(filename, 'w') as f:
        # Header with metadata
        f.write("# Lane-Emden 2D solution for polytropic disk\n")
        f.write(f"# Polytropic index: n = {n}\n")
        f.write(f"# Adiabatic index: gamma = {(n+1)/n:.10f}\n")
        if xi1 is not None:
            f.write(f"# First zero: xi1 = {xi1:.10f}\n")
        f.write(f"# Number of points: {len(xi)}\n")
        f.write("# Columns: xi (dimensionless radius), theta (dimensionless density)\n")
        f.write("# Generated by: generate_lane_emden_table.py\n")
        f.write("#\n")
        
        # Column headers (no # for CSV parsers)
        f.write("xi,theta\n")
        
        # Data
        for x, t in zip(xi, theta):
            f.write(f"{x:.15e},{t:.15e}\n")
    
    print(f"Saved table to: {filename}")


def compare_with_existing(filename, xi_new, theta_new, tolerance=1e-6):
    """
    Compare newly generated table with existing file.
    
    Args:
        filename: Existing CSV file to compare with
        xi_new: New xi values
        theta_new: New theta values
        tolerance: Maximum relative error to accept
    
    Returns:
        bool: True if files match within tolerance
    """
    try:
        # Load existing file (skip comment lines)
        data = np.loadtxt(filename, delimiter=',', skiprows=1)
        xi_old = data[:, 0]
        theta_old = data[:, 1]
        
        print(f"\nComparing with existing file: {filename}")
        print(f"  Existing points: {len(xi_old)}")
        print(f"  New points: {len(xi_new)}")
        
        # Interpolate to compare (different number of points)
        from scipy.interpolate import interp1d
        
        # Remove duplicate xi values if any (keep unique)
        xi_new_unique, idx_unique = np.unique(xi_new, return_index=True)
        theta_new_unique = theta_new[idx_unique]
        
        # Interpolate new data to old xi points (only where overlap exists)
        xi_overlap = xi_old[xi_old <= xi_new_unique[-1]]
        f_new = interp1d(xi_new_unique, theta_new_unique, kind='cubic', fill_value='extrapolate')
        theta_new_interp = f_new(xi_overlap)
        theta_old_overlap = xi_old[:len(xi_overlap)]
        theta_old_vals = theta_old[:len(xi_overlap)]
        
        # Compute relative errors
        rel_errors = np.abs(theta_new_interp - theta_old_vals) / (np.abs(theta_old_vals) + 1e-15)
        max_rel_error = np.max(rel_errors)
        mean_rel_error = np.mean(rel_errors)
        
        print(f"  Max relative error: {max_rel_error:.3e}")
        print(f"  Mean relative error: {mean_rel_error:.3e}")
        
        if max_rel_error < tolerance:
            print(f"  ✓ Tables match within tolerance ({tolerance:.1e})")
            return True
        else:
            print(f"  ✗ Tables differ beyond tolerance ({tolerance:.1e})")
            return False
            
    except FileNotFoundError:
        print(f"  No existing file to compare: {filename}")
        return None
    except Exception as e:
        print(f"  Error comparing: {e}")
        return None


def main():
    parser = argparse.ArgumentParser(
        description='Generate Lane-Emden table for 2D polytropic disk (n=1.5)',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Generate table with default settings
  python generate_lane_emden_table.py
  
  # Generate with custom output file
  python generate_lane_emden_table.py --output my_table.csv
  
  # Generate with higher resolution
  python generate_lane_emden_table.py --n-points 10000
  
  # Compare with existing file
  python generate_lane_emden_table.py --compare lane_emden_2d_data.csv

Physics:
  For a 2D self-gravitating polytropic disk:
    Surface density: Σ(r) = Σ_c θ^n
    Pressure: P = K Σ^γ  where γ = (n+1)/n
    
  For n=1.5:
    γ = 5/3 (adiabatic index for monatomic gas)
    Equation: d²θ/dξ² + (1/ξ) dθ/dξ + θ^1.5 = 0
    
  Scaling:
    r = α ξ  (physical radius from dimensionless radius)
    Σ(r) = Σ_c θ(r/α)^1.5
        """
    )
    
    parser.add_argument('--output', '-o', default='lane_emden_2d_data.csv',
                        help='Output CSV filename (default: lane_emden_2d_data.csv)')
    parser.add_argument('--n-points', '-n', type=int, default=6620,
                        help='Number of output points (default: 6620)')
    parser.add_argument('--xi-max', type=float, default=20.0,
                        help='Maximum xi to integrate to (default: 20.0)')
    parser.add_argument('--polytropic-index', type=float, default=1.5,
                        help='Polytropic index n (default: 1.5)')
    parser.add_argument('--compare', metavar='FILE',
                        help='Compare output with existing file')
    parser.add_argument('--quiet', '-q', action='store_true',
                        help='Suppress progress output')
    
    args = parser.parse_args()
    
    # Solve Lane-Emden equation
    xi, theta, xi1 = solve_lane_emden(
        n=args.polytropic_index,
        xi_max=args.xi_max,
        n_points=args.n_points,
        verbose=not args.quiet
    )
    
    # Save to file
    save_table(args.output, xi, theta, n=args.polytropic_index, xi1=xi1)
    
    # Compare with existing file if requested
    if args.compare:
        compare_with_existing(args.compare, xi, theta)
    
    if not args.quiet:
        print("\n" + "="*60)
        print("Lane-Emden table generation complete!")
        print("="*60)
        print(f"\nTo use this table in your simulation:")
        print(f'  1. Copy to workflow: cp {args.output} xi_theta.csv')
        print(f'  2. Set in config.json: "laneEmdenTable": "path/to/xi_theta.csv"')
        print(f'  3. Enable relaxation: "useDensityRelaxation": true')


if __name__ == '__main__':
    main()
