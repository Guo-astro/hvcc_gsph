#!/usr/bin/env python3
"""
Workflow Structure Validator

This script validates that all workflows in simulations/workflows/ follow
the correct directory structure conventions.

Prevents regression of issues like:
- Using 'simulations' instead of 'workflow_logs'
- Incorrect nesting of output directories
- Missing required directories

Usage:
    python validate_workflow_structure.py
    python validate_workflow_structure.py --fix  # Auto-fix simple issues
"""

import sys
import argparse
from pathlib import Path
from typing import List, Tuple, Dict
import json

class Colors:
    """ANSI color codes for terminal output"""
    GREEN = '\033[92m'
    RED = '\033[91m'
    YELLOW = '\033[93m'
    BLUE = '\033[94m'
    RESET = '\033[0m'
    BOLD = '\033[1m'

def print_success(msg: str):
    print(f"{Colors.GREEN}✓{Colors.RESET} {msg}")

def print_error(msg: str):
    print(f"{Colors.RED}✗{Colors.RESET} {msg}")

def print_warning(msg: str):
    print(f"{Colors.YELLOW}⚠{Colors.RESET} {msg}")

def print_info(msg: str):
    print(f"{Colors.BLUE}ℹ{Colors.RESET} {msg}")

class WorkflowValidator:
    def __init__(self, workflows_root: Path, auto_fix: bool = False):
        self.workflows_root = workflows_root
        self.auto_fix = auto_fix
        self.errors: List[str] = []
        self.warnings: List[str] = []
        self.fixes_applied: List[str] = []
        
    def validate_all_workflows(self) -> bool:
        """Validate all workflows in the workflows directory"""
        print(f"\n{Colors.BOLD}Validating workflows in: {self.workflows_root}{Colors.RESET}\n")
        
        workflows = [d for d in self.workflows_root.iterdir() 
                    if d.is_dir() and not d.name.startswith('.')]
        
        if not workflows:
            print_error("No workflow directories found")
            return False
        
        all_valid = True
        for workflow_dir in sorted(workflows):
            if not self.validate_workflow(workflow_dir):
                all_valid = False
        
        self.print_summary()
        return all_valid
    
    def validate_workflow(self, workflow_dir: Path) -> bool:
        """Validate a single workflow directory"""
        workflow_name = workflow_dir.name
        print(f"\n{Colors.BOLD}Checking workflow: {workflow_name}{Colors.RESET}")
        
        valid = True
        
        # Check 1: Should have workflow_logs, NOT simulations (at top level)
        if not self.check_workflow_logs_directory(workflow_dir, workflow_name):
            valid = False
        
        # Check 2: Should have README.md
        if not self.check_readme(workflow_dir, workflow_name):
            valid = False
        
        # Check 3: Validate step directories
        if not self.validate_step_directories(workflow_dir, workflow_name):
            valid = False
        
        # Check 4: No stray 'simulations' directories
        if not self.check_no_stray_simulations_dirs(workflow_dir, workflow_name):
            valid = False
        
        return valid
    
    def check_workflow_logs_directory(self, workflow_dir: Path, workflow_name: str) -> bool:
        """Check that workflow_logs directory exists (not 'simulations')"""
        workflow_logs = workflow_dir / "workflow_logs"
        simulations_dir = workflow_dir / "simulations"
        
        if simulations_dir.exists() and not workflow_logs.exists():
            self.errors.append(
                f"{workflow_name}: Has 'simulations' directory instead of 'workflow_logs'"
            )
            print_error(f"Found 'simulations' directory instead of 'workflow_logs'")
            
            if self.auto_fix:
                simulations_dir.rename(workflow_logs)
                self.fixes_applied.append(
                    f"{workflow_name}: Renamed 'simulations' → 'workflow_logs'"
                )
                print_success(f"Auto-fixed: Renamed to 'workflow_logs'")
                return True
            else:
                print_info(f"Run with --fix to auto-rename")
                return False
        
        elif workflow_logs.exists():
            print_success("Has correct 'workflow_logs' directory")
            return True
        
        else:
            self.warnings.append(
                f"{workflow_name}: Missing 'workflow_logs' directory (may not have been run yet)"
            )
            print_warning("No 'workflow_logs' directory found")
            return True  # Not an error, just hasn't been run yet
    
    def check_readme(self, workflow_dir: Path, workflow_name: str) -> bool:
        """Check for README.md"""
        readme = workflow_dir / "README.md"
        if readme.exists():
            print_success("Has README.md")
            return True
        else:
            self.warnings.append(f"{workflow_name}: Missing README.md")
            print_warning("Missing README.md")
            return True  # Warning, not error
    
    def validate_step_directories(self, workflow_dir: Path, workflow_name: str) -> bool:
        """Validate step directories (01_*, 02_*, etc.)"""
        step_dirs = sorted([d for d in workflow_dir.iterdir() 
                           if d.is_dir() and d.name.startswith('0')])
        
        if not step_dirs:
            print_info("No step directories found (may be simple workflow)")
            return True
        
        all_valid = True
        for step_dir in step_dirs:
            if not self.validate_step_structure(step_dir, workflow_name):
                all_valid = False
        
        return all_valid
    
    def validate_step_structure(self, step_dir: Path, workflow_name: str) -> bool:
        """Validate the structure of a step directory"""
        step_name = step_dir.name
        print(f"  Checking step: {step_name}")
        
        valid = True
        
        # Expected directories
        expected_dirs = {
            'config': False,  # Not required
            'src': False,     # Not required
            'build': False,   # Not required (created during build)
        }
        
        # Check what exists
        for expected_dir, required in expected_dirs.items():
            dir_path = step_dir / expected_dir
            if dir_path.exists():
                print(f"    ✓ Has {expected_dir}/")
            elif required:
                self.errors.append(
                    f"{workflow_name}/{step_name}: Missing required directory '{expected_dir}'"
                )
                print_error(f"    Missing required directory: {expected_dir}/")
                valid = False
        
        # Check for output directories - should follow pattern:
        # <output_dir>/<sample_name>/run_*/
        self.check_output_structure(step_dir, workflow_name, step_name)
        
        return valid
    
    def check_output_structure(self, step_dir: Path, workflow_name: str, step_name: str):
        """Check output directory structure"""
        # Find config files to determine expected output directories
        config_dir = step_dir / "config"
        if not config_dir.exists():
            return
        
        for config_file in config_dir.glob("*.json"):
            try:
                with open(config_file) as f:
                    config = json.load(f)
                
                output_dir_name = config.get('outputDirectory', '')
                if not output_dir_name:
                    continue
                
                # Output directory might be absolute or relative
                if output_dir_name.startswith('/') or output_dir_name.startswith('simulations/'):
                    # Absolute path - can't validate easily
                    continue
                
                output_dir = step_dir / output_dir_name
                if not output_dir.exists():
                    continue
                
                # Check structure: should have sample_name subdirectories
                sample_dirs = [d for d in output_dir.iterdir() if d.is_dir()]
                
                for sample_dir in sample_dirs:
                    # Check for run_* directories
                    run_dirs = list(sample_dir.glob("run_*"))
                    
                    if run_dirs:
                        print(f"    ✓ Output structure: {output_dir.name}/{sample_dir.name}/run_*/")
                        
                        # Check a sample run directory structure
                        run_dir = run_dirs[0]
                        self.validate_run_directory(run_dir, workflow_name, step_name)
                
            except (json.JSONDecodeError, IOError) as e:
                print_warning(f"    Could not read config: {config_file.name}: {e}")
    
    def validate_run_directory(self, run_dir: Path, workflow_name: str, step_name: str):
        """Validate the structure of a run directory"""
        expected_subdirs = ['outputs', 'visualizations', 'logs', 'analysis']
        expected_files = ['config.json', 'metadata.json', 'initial_conditions.csv']
        
        # Check subdirectories
        missing_dirs = []
        for subdir in expected_subdirs:
            if not (run_dir / subdir).exists():
                missing_dirs.append(subdir)
        
        if missing_dirs:
            print_warning(f"      Run dir missing: {', '.join(missing_dirs)}")
        
        # Check for metadata.json (important for our fix)
        metadata_file = run_dir / "outputs" / "csv" / "metadata.json"
        if metadata_file.exists():
            print(f"      ✓ Has metadata.json (not per-frame)")
        else:
            # Check for old-style per-frame metadata
            csv_dir = run_dir / "outputs" / "csv"
            if csv_dir.exists():
                meta_files = list(csv_dir.glob("*.meta.json"))
                if len(meta_files) > 1:
                    self.errors.append(
                        f"{workflow_name}/{step_name}: Found {len(meta_files)} .meta.json files "
                        f"(should be 1 metadata.json)"
                    )
                    print_error(f"      Found {len(meta_files)} .meta.json files - should be 1 metadata.json")
    
    def check_no_stray_simulations_dirs(self, workflow_dir: Path, workflow_name: str) -> bool:
        """Check for stray 'simulations' directories that shouldn't exist"""
        # Allow 'simulations' only at top level if it's actually workflow_logs
        stray_simulations = []
        
        for dirpath in workflow_dir.rglob('simulations'):
            # Check if it's a deeply nested 'simulations' directory
            relative = dirpath.relative_to(workflow_dir)
            parts = relative.parts
            
            # If it's nested inside a step directory, it's suspicious
            if len(parts) > 2:  # e.g., 01_step/some/simulations
                stray_simulations.append(dirpath)
        
        if stray_simulations:
            for stray in stray_simulations:
                self.errors.append(
                    f"{workflow_name}: Stray 'simulations' directory at {stray.relative_to(workflow_dir)}"
                )
                print_error(f"Stray 'simulations' directory: {stray.relative_to(workflow_dir)}")
            return False
        
        return True
    
    def print_summary(self):
        """Print validation summary"""
        print(f"\n{Colors.BOLD}═══ Validation Summary ═══{Colors.RESET}\n")
        
        if self.fixes_applied:
            print(f"{Colors.BOLD}Fixes Applied:{Colors.RESET}")
            for fix in self.fixes_applied:
                print_success(fix)
            print()
        
        if self.errors:
            print(f"{Colors.BOLD}Errors Found: {len(self.errors)}{Colors.RESET}")
            for error in self.errors:
                print_error(error)
            print()
        
        if self.warnings:
            print(f"{Colors.BOLD}Warnings: {len(self.warnings)}{Colors.RESET}")
            for warning in self.warnings:
                print_warning(warning)
            print()
        
        if not self.errors and not self.warnings:
            print_success(f"{Colors.BOLD}All workflows are valid! ✨{Colors.RESET}")
        elif not self.errors:
            print_success(f"{Colors.BOLD}All workflows are valid (with {len(self.warnings)} warnings){Colors.RESET}")
        else:
            print_error(f"{Colors.BOLD}Validation failed with {len(self.errors)} errors{Colors.RESET}")

def main():
    parser = argparse.ArgumentParser(
        description="Validate workflow directory structure",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s                    # Validate all workflows
  %(prog)s --fix              # Validate and auto-fix issues
  %(prog)s --workflow shock_tube_workflow  # Validate specific workflow
        """
    )
    parser.add_argument(
        '--fix',
        action='store_true',
        help='Automatically fix simple issues (like renaming directories)'
    )
    parser.add_argument(
        '--workflow',
        type=str,
        help='Validate only a specific workflow'
    )
    
    args = parser.parse_args()
    
    # Find workflows directory
    script_dir = Path(__file__).parent
    workflows_root = script_dir.parent / "simulations" / "workflows"
    
    if not workflows_root.exists():
        print_error(f"Workflows directory not found: {workflows_root}")
        sys.exit(1)
    
    validator = WorkflowValidator(workflows_root, auto_fix=args.fix)
    
    if args.workflow:
        workflow_dir = workflows_root / args.workflow
        if not workflow_dir.exists():
            print_error(f"Workflow not found: {args.workflow}")
            sys.exit(1)
        
        success = validator.validate_workflow(workflow_dir)
        validator.print_summary()
    else:
        success = validator.validate_all_workflows()
    
    sys.exit(0 if success else 1)

if __name__ == '__main__':
    main()
