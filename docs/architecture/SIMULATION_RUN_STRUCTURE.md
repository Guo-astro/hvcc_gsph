# Simulation Output Organization - Improved Structure

## Problem with Current Approach

**Current structure separates critical components:**
- ❌ Configs in `configs/benchmarks/`
- ❌ Outputs in `visualizations/shock_tube/DISPH/1D/`
- ❌ Initial conditions in code (not saved)
- ❌ No metadata (git hash, parameters, timestamps)
- ❌ Hard to reproduce exact simulation later

## Proposed Structure: Self-Contained Simulation Runs

### Directory Structure

```
simulations/
├── shock_tube/
│   ├── run_2025-11-01_143052_disph_1d/          # Timestamped run
│   │   ├── metadata.json                         # Complete run info
│   │   ├── config.json                           # Exact config used
│   │   ├── initial_conditions.csv                # IC snapshot (t=0)
│   │   ├── outputs/                              # Simulation outputs
│   │   │   ├── csv/                              # CSV format
│   │   │   │   ├── 00000.csv
│   │   │   │   ├── 00001.csv
│   │   │   │   └── ...
│   │   │   └── binary/                           # Binary format
│   │   │       ├── 00000.sph
│   │   │       └── ...
│   │   ├── visualizations/                       # Generated plots/animations
│   │   │   ├── density_evolution.mp4
│   │   │   ├── final_state.png
│   │   │   └── shock_comparison.png
│   │   ├── logs/                                 # Execution logs
│   │   │   ├── stdout.log
│   │   │   ├── energy.log
│   │   │   └── performance.log
│   │   └── analysis/                             # Analysis results
│   │       ├── conservation_check.txt
│   │       └── shock_position.csv
│   │
│   ├── run_2025-11-01_150324_gsph_1d/           # Different method
│   │   └── ...
│   │
│   └── latest -> run_2025-11-01_150324_gsph_1d/ # Symlink to latest
│
├── sedov_taylor/
│   ├── run_2025-11-01_161045_gsph_3d/
│   │   ├── metadata.json
│   │   ├── config.json
│   │   ├── initial_conditions.csv
│   │   ├── outputs/
│   │   │   ├── csv/
│   │   │   └── binary/
│   │   ├── visualizations/
│   │   ├── logs/
│   │   └── analysis/
│   └── latest -> run_2025-11-01_161045_gsph_3d/
│
└── production/                                    # Important production runs
    ├── galaxy_collision_highres/
    │   ├── run_2025-10-15_v1.2.3/               # Tagged with version
    │   └── run_2025-10-20_v1.2.4/
    └── ...
```

### Metadata File Format

**`metadata.json`** - Complete reproducibility information:

```json
{
  "run_info": {
    "run_id": "shock_tube_2025-11-01_143052_disph_1d",
    "sample_name": "shock_tube",
    "description": "Standard Sod shock tube with DISPH method",
    "created_at": "2025-11-01T14:30:52Z",
    "completed_at": "2025-11-01T14:31:15Z",
    "duration_seconds": 23.4,
    "user": "guo",
    "hostname": "kakuiwamatsus-MacBook-Pro"
  },
  
  "code_version": {
    "git_hash": "a3f2b1c",
    "git_branch": "main",
    "git_dirty": false,
    "build_date": "2025-11-01T12:00:00Z",
    "compiler": "AppleClang 15.0.0",
    "cmake_version": "3.28.1"
  },
  
  "simulation_params": {
    "sph_type": "DISPH",
    "dimension": 1,
    "particle_count": 500,
    "gamma": 1.4,
    "cfl_sound": 0.3,
    "end_time": 0.2,
    "output_interval": 0.02,
    "neighbor_number": 50
  },
  
  "unit_system": {
    "type": "SI",
    "length_unit": "m",
    "time_unit": "s",
    "mass_unit": "kg",
    "length_factor": 1.0,
    "time_factor": 1.0,
    "mass_factor": 1.0
  },
  
  "initial_conditions": {
    "type": "sod_shock_tube",
    "rho_left": 1.0,
    "rho_right": 0.125,
    "p_left": 1.0,
    "p_right": 0.1,
    "discontinuity_position": 0.0
  },
  
  "output": {
    "formats": ["csv", "binary"],
    "snapshot_count": 11,
    "snapshot_times": [0.0, 0.028, 0.047, 0.066, 0.085, 0.104, 0.123, 0.142, 0.161, 0.189, 0.208],
    "total_output_size_mb": 0.61
  },
  
  "performance": {
    "total_timesteps": 2156,
    "wall_time_seconds": 23.4,
    "timesteps_per_second": 92.1,
    "particles_per_second": 46050
  },
  
  "validation": {
    "energy_conservation_error": 1.2e-6,
    "mass_conservation_error": 0.0,
    "shock_position_error": 0.003
  }
}
```

## Implementation Design

### 1. Simulation Run Manager Class

```cpp
class SimulationRun {
public:
    struct Config {
        std::string base_dir = "simulations";
        std::string sample_name;
        std::string sph_type;
        int dimension;
        bool save_initial_conditions = true;
        bool save_config = true;
        bool save_metadata = true;
        bool create_latest_symlink = true;
        std::vector<OutputFormat> output_formats = {OutputFormat::CSV, OutputFormat::BINARY};
    };
    
    SimulationRun(const Config& config);
    
    // Directory management
    std::string get_run_directory() const;
    std::string get_outputs_directory(OutputFormat format) const;
    std::string get_visualizations_directory() const;
    std::string get_logs_directory() const;
    
    // Metadata management
    void save_metadata(const Metadata& metadata);
    void save_config(const json& config);
    void save_initial_conditions(const std::vector<SPHParticle>& particles);
    
    // Output writers
    std::unique_ptr<OutputWriter> create_writer(OutputFormat format);
    
private:
    std::string m_run_id;
    std::string m_run_directory;
    Config m_config;
    
    std::string generate_run_id() const;
    void create_directory_structure();
};
```

### 2. Directory Path Generation

```cpp
// Auto-generate run ID with timestamp
std::string generate_run_id(const std::string& sample_name,
                            const std::string& sph_type,
                            int dimension) {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&time_t);
    
    std::ostringstream oss;
    oss << "run_"
        << std::put_time(&tm, "%Y-%m-%d_%H%M%S")
        << "_" << to_lower(sph_type)
        << "_" << dimension << "d";
    
    return oss.str();
}

// Full directory structure:
// simulations/{sample_name}/{run_id}/outputs/{format}/
std::string get_full_path(const std::string& sample_name,
                          const std::string& run_id,
                          const std::string& subdir = "") {
    std::ostringstream path;
    path << "simulations/" << sample_name << "/" << run_id;
    if (!subdir.empty()) {
        path << "/" << subdir;
    }
    return path.str();
}
```

### 3. Metadata Generation

```cpp
class MetadataGenerator {
public:
    static json generate_metadata(
        const std::string& run_id,
        const SimulationRun::Config& config,
        const SPHParameters& params,
        const SimulationStats& stats
    ) {
        json metadata;
        
        // Run info
        metadata["run_info"] = {
            {"run_id", run_id},
            {"sample_name", config.sample_name},
            {"created_at", get_timestamp_iso8601()},
            {"user", get_username()},
            {"hostname", get_hostname()}
        };
        
        // Code version
        metadata["code_version"] = {
            {"git_hash", get_git_hash()},
            {"git_branch", get_git_branch()},
            {"git_dirty", is_git_dirty()},
            {"compiler", get_compiler_version()},
            {"cmake_version", CMAKE_VERSION}
        };
        
        // Simulation parameters
        metadata["simulation_params"] = {
            {"sph_type", sph_type_to_string(params.type)},
            {"dimension", DIM},
            {"gamma", params.physics.gamma},
            {"cfl_sound", params.cfl.sound},
            // ... all parameters
        };
        
        return metadata;
    }
};
```

### 4. Usage in Solver

```cpp
void Solver::run() {
    // Create simulation run
    SimulationRun::Config run_config;
    run_config.sample_name = m_sample_name;
    run_config.sph_type = sph_type_to_string(m_param->type);
    run_config.dimension = DIM;
    run_config.output_formats = {OutputFormat::CSV, OutputFormat::BINARY};
    
    SimulationRun sim_run(run_config);
    
    WRITE_LOG << "Simulation run directory: " << sim_run.get_run_directory();
    
    // Save initial conditions
    if (run_config.save_initial_conditions) {
        sim_run.save_initial_conditions(m_sim->get_particles());
    }
    
    // Save config
    if (run_config.save_config) {
        sim_run.save_config(m_config_json);
    }
    
    // Create output writers for all formats
    std::vector<std::unique_ptr<OutputWriter>> writers;
    for (auto format : run_config.output_formats) {
        writers.push_back(sim_run.create_writer(format));
    }
    
    // Main simulation loop
    auto start_time = std::chrono::high_resolution_clock::now();
    int total_steps = 0;
    
    while (m_sim->get_time() < m_param->time.end) {
        timestep();
        total_steps++;
        
        if (should_output()) {
            for (auto& writer : writers) {
                writer->write_snapshot(m_sim);
            }
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time);
    
    // Save metadata
    SimulationStats stats{
        .total_timesteps = total_steps,
        .wall_time_seconds = duration.count(),
        .snapshot_count = writers[0]->get_snapshot_count()
    };
    
    auto metadata = MetadataGenerator::generate_metadata(
        sim_run.get_run_id(), run_config, *m_param, stats
    );
    sim_run.save_metadata(metadata);
    
    WRITE_LOG << "Simulation complete. Results in: " << sim_run.get_run_directory();
}
```

## Benefits of This Structure

### 1. Complete Reproducibility ✅
- All files needed to reproduce run in one place
- Git hash tracks exact code version
- Config file shows exact parameters used
- Initial conditions saved for verification

### 2. Easy Comparison ✅
```bash
# Compare two runs
diff simulations/shock_tube/run_A/metadata.json \
     simulations/shock_tube/run_B/metadata.json

# Check what changed
git diff $(cat simulations/shock_tube/run_A/metadata.json | jq -r '.code_version.git_hash') \
         $(cat simulations/shock_tube/run_B/metadata.json | jq -r '.code_version.git_hash')
```

### 3. Archive-Ready ✅
```bash
# Archive complete run
tar -czf shock_tube_run_2025-11-01.tar.gz \
    simulations/shock_tube/run_2025-11-01_143052_disph_1d/

# Share with collaborators (everything needed is included)
```

### 4. Quick Access to Latest ✅
```bash
# Always work with latest run
python3 analysis/plot.py simulations/shock_tube/latest/outputs/binary/
```

### 5. Production Run Tracking ✅
```bash
# Tag important runs
cp -r simulations/shock_tube/latest \
      simulations/production/shock_tube_v1.2.3_publication/
```

## Configuration File Updates

### New JSON Parameters

```json
{
  "sampleName": "shock_tube",
  "runDescription": "Standard Sod problem with DISPH",
  
  "output": {
    "baseDirectory": "simulations",
    "autoRunId": true,
    "runIdPrefix": "run",
    "createLatestSymlink": true,
    "formats": ["csv", "binary"],
    "saveInitialConditions": true,
    "saveConfig": true,
    "saveMetadata": true
  },
  
  "SPHType": "disph",
  "endTime": 0.2,
  "outputInterval": 0.02,
  
  "... other parameters ..."
}
```

### Backward Compatibility

Old configs still work with defaults:
```json
{
  "outputDirectory": "results/DISPH/shock_tube/1D"  // Old style still works
}
```

New configs use structured approach:
```json
{
  "sampleName": "shock_tube",
  "output": {
    "autoRunId": true  // New style: auto-generates run directory
  }
}
```

## Migration Strategy

### Phase 1: Implement Core Structure (Week 1)
- [ ] Create `SimulationRun` class
- [ ] Implement directory generation
- [ ] Add metadata generation
- [ ] Test with shock tube

### Phase 2: Enhance Metadata (Week 2)
- [ ] Add git version tracking
- [ ] Add performance metrics
- [ ] Add validation metrics
- [ ] Auto-save logs

### Phase 3: Analysis Tools (Week 3)
- [ ] Update Python readers to use new structure
- [ ] Create run comparison tools
- [ ] Add run search/query utilities
- [ ] Update documentation

## Python Analysis Tools

### Find and Load Runs

```python
from pathlib import Path
import json

class SimulationRunFinder:
    """Find and load simulation runs"""
    
    def __init__(self, base_dir="simulations"):
        self.base_dir = Path(base_dir)
    
    def find_latest(self, sample_name: str):
        """Find latest run for a sample"""
        latest_link = self.base_dir / sample_name / "latest"
        if latest_link.exists():
            return SimulationRunReader(latest_link)
        return None
    
    def find_all(self, sample_name: str):
        """Find all runs for a sample"""
        sample_dir = self.base_dir / sample_name
        runs = []
        for run_dir in sorted(sample_dir.glob("run_*")):
            runs.append(SimulationRunReader(run_dir))
        return runs
    
    def find_by_date(self, sample_name: str, date: str):
        """Find runs on specific date (YYYY-MM-DD)"""
        sample_dir = self.base_dir / sample_name
        runs = []
        for run_dir in sample_dir.glob(f"run_{date}*"):
            runs.append(SimulationRunReader(run_dir))
        return runs

class SimulationRunReader:
    """Read a complete simulation run"""
    
    def __init__(self, run_directory):
        self.run_dir = Path(run_directory)
        self.metadata = self._load_metadata()
    
    def _load_metadata(self):
        with open(self.run_dir / "metadata.json") as f:
            return json.load(f)
    
    def get_config(self):
        with open(self.run_dir / "config.json") as f:
            return json.load(f)
    
    def get_output_reader(self, format="binary"):
        from analysis.binary_reader import BinarySimulationReader
        from analysis.readers import SimulationReader
        
        output_dir = self.run_dir / "outputs" / format
        if format == "binary":
            return BinarySimulationReader(output_dir)
        else:
            return SimulationReader(output_dir)
    
    def __repr__(self):
        run_id = self.metadata['run_info']['run_id']
        created = self.metadata['run_info']['created_at']
        return f"SimulationRun({run_id}, created={created})"

# Usage
finder = SimulationRunFinder()

# Get latest shock tube run
latest = finder.find_latest("shock_tube")
print(latest.metadata)

reader = latest.get_output_reader("binary")
data = reader.read_snapshot(0)

# Compare all runs
runs = finder.find_all("shock_tube")
for run in runs:
    print(f"{run.metadata['run_info']['run_id']}: "
          f"Energy error = {run.metadata['validation']['energy_conservation_error']}")
```

## Summary

### Proposed Structure Benefits

✅ **Complete Reproducibility** - All inputs, outputs, and metadata in one place  
✅ **Version Tracking** - Git hash, compiler version, exact parameters  
✅ **Easy Archiving** - Single directory contains everything  
✅ **Comparison-Friendly** - Compare runs with simple diff/scripts  
✅ **Production-Ready** - Tag and preserve important runs  
✅ **Analysis-Friendly** - Python tools to find and load runs  
✅ **Backward Compatible** - Old configs still work  

### Recommended Next Steps

1. **Implement `SimulationRun` class** - Core directory management
2. **Add metadata generation** - Git hash, parameters, performance
3. **Update solver integration** - Use new structure by default
4. **Create Python tools** - Run finder, loader, comparison
5. **Update documentation** - New structure guide

Would you like me to implement this structure? I can start with the `SimulationRun` class and integrate it into your solver.
