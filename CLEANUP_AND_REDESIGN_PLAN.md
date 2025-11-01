# Codebase Cleanup & Checkpoint Redesign Plan

## Part 1: Root Directory Cleanup

### Current Problems
- 12+ scattered markdown documentation files
- Duplicated content between root and `docs/`
- Temporary/debug files in root
- No clear SSOT for documentation

### Proposed Structure
```
sphcode/
├── README.md                    # Main entry point (keep)
├── LICENSE                      # Legal (keep)
├── CMakeLists.txt              # Build (keep)
├── Makefile                    # Build (keep)
├── pyproject.toml              # Python deps (keep)
├── flake.nix                   # Nix config (keep)
├── .gitignore                  # Git (keep)
├── docs/                       # ALL documentation here
│   ├── README.md              # Documentation index
│   ├── QUICKSTART.md          # Getting started guide
│   ├── ARCHITECTURE.md        # System design
│   ├── CONTRIBUTING.md        # How to contribute
│   ├── user_guide/
│   ├── developer/
│   └── architecture/
├── scripts/                    # ALL scripts here
├── test/
├── src/
├── include/
└── simulations/
```

### Files to Delete
- [x] REFACTORING_*.md (7 files)
- [x] MIGRATION_*.md (2 files)
- [x] PLUGIN_*.md (3 files)
- [x] SAMPLES_MIGRATION_*.md (2 files)
- [x] WHERE_THINGS_ARE.md
- [x] REORGANIZATION_SUMMARY.md
- [x] QUICK_REFERENCE.md
- [x] disph_run.log
- [x] test_new_output_system.py
- [x] test_results.txt
- [x] disph_paper.pdf (move to docs/references/)

### Files to Consolidate
Merge into `docs/QUICKSTART.md`:
- Quick start content
- Basic usage examples

Merge into `docs/ARCHITECTURE.md`:
- Architecture details
- Build system
- Dimension handling

Merge into `docs/developer/CONTRIBUTING.md`:
- Development workflow
- Refactoring notes (archive only relevant parts)

## Part 2: Checkpoint/Resume System Redesign

### Current System Analysis

**What exists:**
- `checkpoint_file` parameter: Loads initial conditions from CSV
- `CheckpointModifier`: Modifies particles after loading (for flyby sims)
- No actual pause/resume capability

**What's missing:**
- Save simulation state at arbitrary time
- Resume from saved state
- Incremental checkpointing during long runs
- Automatic checkpoint on SIGINT/SIGTERM

### User Stories (BDD)

```gherkin
Feature: Simulation Pause and Resume
  As a researcher running long simulations
  I want to pause and resume simulations
  So that I can manage computational resources effectively

Scenario: Save checkpoint during simulation
  Given a simulation is running
  When the simulation reaches t=5.0
  Then a checkpoint file should be saved
  And the checkpoint should contain all particle state
  And the checkpoint should contain simulation time
  And the checkpoint should be resumable

Scenario: Resume from checkpoint
  Given a checkpoint file exists at t=5.0
  When I start a new simulation with resume=true
  Then the simulation should load the checkpoint
  And the simulation should continue from t=5.0
  And the results should be identical to uninterrupted run

Scenario: Auto-checkpoint on interrupt
  Given a simulation is running
  When I press Ctrl+C
  Then a checkpoint should be saved before exit
  And I should be able to resume later

Scenario: Periodic auto-checkpointing
  Given a simulation with checkpoint_interval=1.0
  When the simulation runs
  Then checkpoints should be saved at t=1.0, 2.0, 3.0, ...
  And old checkpoints should be optionally cleaned up
```

### Proposed Design

#### 1. Rename Current System
```cpp
// OLD (confusing)
checkpoint_file -> initial_conditions_file
CheckpointModifier -> InitialConditionsModifier

// This clarifies it's for LOADING initial state, not pause/resume
```

#### 2. New Checkpoint System
```cpp
namespace sph {

// Checkpoint data structure
struct CheckpointData {
    real time;
    real dt;
    int step;
    std::vector<SPHParticle> particles;
    SPHParameters params;  // Relevant runtime params
    
    // Metadata
    std::string simulation_name;
    std::string sph_type;
    int dimension;
    std::string created_at;  // ISO timestamp
};

// Checkpoint manager
class CheckpointManager {
public:
    // Save current state
    void save_checkpoint(
        const std::string& filepath,
        const Simulation& sim,
        const SPHParameters& params
    );
    
    // Load saved state
    CheckpointData load_checkpoint(const std::string& filepath);
    
    // Auto-checkpoint configuration
    struct AutoCheckpointConfig {
        bool enabled = false;
        real interval = 1.0;  // Save every 1.0 time units
        int max_keep = 3;     // Keep last 3 checkpoints
        bool on_interrupt = true;  // Save on Ctrl+C
    };
    
    void configure_auto_checkpoint(const AutoCheckpointConfig& config);
    bool should_checkpoint(real current_time) const;
    
private:
    AutoCheckpointConfig config_;
    real last_checkpoint_time_ = 0.0;
    std::deque<std::string> checkpoint_files_;
    
    void cleanup_old_checkpoints();
};

} // namespace sph
```

#### 3. Config File Format
```json
{
  "simulation": "shock_tube",
  
  // NEW: For resuming from saved state
  "resume": {
    "enabled": false,
    "checkpoint_file": "output/run_xyz/checkpoints/checkpoint_t5.0.chk"
  },
  
  // NEW: Auto-checkpoint during run
  "checkpointing": {
    "enabled": true,
    "interval": 10.0,          // Save every 10.0 time units
    "max_keep": 5,             // Keep last 5 checkpoints
    "on_interrupt": true,      // Save on Ctrl+C
    "directory": "checkpoints" // Subdir in run output
  },
  
  // RENAMED: Clarify this is for initial conditions
  "initial_conditions": {
    "file": "previous_run/00100.csv",  // Load from previous simulation
    "modifier": "infall_modifier"      // Apply modifications
  }
}
```

#### 4. File Format
Binary format using simple serialization:
```
[HEADER: 512 bytes]
  Magic: "SPHCHKPT"
  Version: uint32
  Timestamp: ISO string
  Simulation name: string
  SPH type: string
  Dimension: int
  Time: double
  Timestep: double
  Step: int64
  Particle count: int64
  
[PARAMETERS: variable]
  JSON-encoded SPHParameters
  
[PARTICLE DATA: N × sizeof(SPHParticle)]
  Binary dump of particle array
  
[CHECKSUM: 32 bytes]
  SHA-256 of all above data
```

#### 5. Integration with Solver
```cpp
class Solver {
private:
    std::unique_ptr<CheckpointManager> checkpoint_mgr_;
    
    void run() {
        initialize();
        
        // Check for resume
        if (config_.resume.enabled) {
            auto chk_data = checkpoint_mgr_->load_checkpoint(
                config_.resume.checkpoint_file
            );
            restore_from_checkpoint(chk_data);
        }
        
        // Main loop
        while (t < t_end) {
            integrate();
            update_time();
            
            // Auto-checkpoint
            if (checkpoint_mgr_->should_checkpoint(t)) {
                auto chk_path = create_checkpoint_path(t);
                checkpoint_mgr_->save_checkpoint(chk_path, *m_sim, *m_param);
            }
            
            // ... output, etc ...
        }
    }
    
    void restore_from_checkpoint(const CheckpointData& data) {
        m_sim->set_time(data.time);
        m_sim->set_dt(data.dt);
        m_sim->set_particles(data.particles);
        // Rebuild tree, neighbors, etc.
    }
};
```

### Implementation Plan (TDD/BDD)

#### Phase 1: Rename & Clarify Existing System
1. **Test**: Existing initial condition loading still works
2. **Refactor**: Rename `checkpoint_file` → `initial_conditions_file`
3. **Refactor**: Rename `CheckpointModifier` → `InitialConditionsModifier`
4. **Test**: Verify backward compatibility with old config files
5. **Document**: Update all docs to reflect new naming

#### Phase 2: Checkpoint Save/Load
1. **Test**: Write test for saving checkpoint
   ```cpp
   TEST(CheckpointManager, SaveAndLoadCheckpoint) {
       // Given: A simulation at t=5.0
       // When: Save checkpoint
       // Then: File exists and contains correct data
   }
   ```
2. **Implement**: `CheckpointManager::save_checkpoint()`
3. **Test**: Write test for loading checkpoint
4. **Implement**: `CheckpointManager::load_checkpoint()`
5. **Test**: Integration test - save at t=5, resume, verify identical results

#### Phase 3: Auto-Checkpointing
1. **Test**: Periodic checkpointing
   ```cpp
   TEST(CheckpointManager, PeriodicCheckpointing) {
       // Given: interval=1.0, simulation runs 0→5
       // Then: Checkpoints at t=1,2,3,4,5
   }
   ```
2. **Implement**: `should_checkpoint()` logic
3. **Test**: Max keep limit
4. **Implement**: `cleanup_old_checkpoints()`

#### Phase 4: Signal Handling
1. **Test**: SIGINT handler saves checkpoint
2. **Implement**: Signal handler registration
3. **Implement**: Graceful shutdown with checkpoint

#### Phase 5: Documentation & Examples
1. Update user guide with checkpoint/resume examples
2. Create example configs
3. Add troubleshooting section

### Test Cases

```cpp
// test/checkpoint_test.cpp
TEST(CheckpointManager, BasicSaveLoad) {
    // Create simple simulation state
    SPHParameters params;
    params.time.start = 0.0;
    params.physics.gamma = 1.4;
    
    Simulation sim(std::make_shared<SPHParameters>(params));
    // ... add particles ...
    sim.set_time(5.0);
    
    // Save
    CheckpointManager mgr;
    mgr.save_checkpoint("test.chk", sim, params);
    
    // Load
    auto data = mgr.load_checkpoint("test.chk");
    
    EXPECT_EQ(data.time, 5.0);
    EXPECT_EQ(data.particles.size(), sim.get_particle_num());
    // ... verify particle data ...
}

TEST(CheckpointManager, ResumeProducesSameResults) {
    // Run simulation 0→10, save at t=5
    auto results_continuous = run_simulation(0, 10, save_checkpoint_at=5.0);
    
    // Run 0→5, then resume 5→10
    run_simulation(0, 5);
    auto results_resumed = run_simulation(resume_from="checkpoint_t5.chk");
    
    // Results should be identical
    EXPECT_PARTICLES_EQUAL(results_continuous, results_resumed);
}
```

## Migration Path

### Week 1: Cleanup
- [ ] Create consolidated docs
- [ ] Delete redundant markdown files
- [ ] Move disph_paper.pdf to docs/references/
- [ ] Update README to point to new doc structure
- [ ] Remove temporary files

### Week 2: Checkpoint Rename ✅ **COMPLETE**
- [x] Rename checkpoint → initial_conditions in code
- [x] Update all config examples
- [x] Add deprecation warnings for old names
- [x] Update documentation

### Week 3: Checkpoint Implementation ✅ **COMPLETE**
- [x] Implement CheckpointManager
- [x] Add save/load functionality
- [x] Write unit tests (22/22 passing)
- [x] Integration with Solver

### Week 4: Auto-Checkpoint & Polish ✅ **IN PROGRESS**
- [x] Implement periodic checkpointing
- [x] Add checkpoint configuration (JSON)
- [x] Integrate auto-checkpoint into simulation loop
- [x] Implement resume from checkpoint
- [x] **BONUS**: Implement initial conditions loading via checkpoint format
- [ ] Add signal handler (SIGINT checkpoint save) - **NEXT**
- [x] Write comprehensive tests (22 checkpoint unit tests)
- [ ] Update documentation (Phase 7)
- [x] Remove legacy configs/ directory

## Success Criteria

### Cleanup
- [ ] Root has ≤5 markdown files
- [ ] All docs in `docs/` directory
- [ ] No duplication between root and docs/
- [ ] Clear SSOT for each topic
- [x] Remove legacy configs/ directory ✅

### Checkpoint System ✅ **MOSTLY COMPLETE**
- [x] Can save simulation state at any time ✅
- [x] Can resume from saved state ✅
- [ ] Resume produces identical results to continuous run ⏸️ (Task 32 deferred)
- [x] Auto-checkpoint works with configurable interval ✅
- [ ] Ctrl+C saves checkpoint before exit ⏳ (Phase 6)
- [ ] Clear documentation and examples ⏳ (Phase 7)
- [x] Old "checkpoint" renamed to "initial_conditions" ✅

### Current Status (2025-11-01)

**✅ Completed:**
- Phase 3: CheckpointManager with save/load (22 tests passing)
- Phase 4: Renamed checkpoint → initial_conditions 
- Phase 5: Solver integration with auto-checkpoint and resume
  - Resume configuration (resumeFromCheckpoint, resumeCheckpointFile)
  - Auto-checkpoint configuration (enableCheckpointing, checkpointInterval, etc.)
  - Checkpoint manager integration
  - Auto-checkpoint in simulation loop
  - Resume from checkpoint on startup
  - Initial conditions loading via checkpoint format (bonus)
  - All 22 checkpoint unit tests passing
  - Clean build with no errors

**🔄 In Progress:**
- Phase 6: Signal handling (SIGINT checkpoint save)

**⏸️ Pending:**
- Phase 1: Documentation cleanup (deferred)
- Phase 7: Final documentation and validation
