# Checkpoint and Resume Feature
#
# The checkpoint system allows saving and resuming long-running SPH simulations.
# This enables pause/resume capability, automatic checkpointing at intervals,
# and graceful handling of interruptions.

Feature: Simulation Checkpoint and Resume
  As a researcher running long SPH simulations
  I want to save checkpoints during execution
  So that I can resume simulations after interruptions

  Background:
    Given the SPH simulation framework is initialized
    And checkpoint functionality is enabled

  Scenario: Save checkpoint during simulation
    Given a Sedov-Taylor simulation is running
    And the simulation has 10000 particles
    When the simulation reaches time t=0.5
    And a manual checkpoint is requested
    Then a checkpoint file should be created
    And the checkpoint should contain 10000 particles
    And the checkpoint should record time=0.5
    And the checkpoint should record the current timestep value
    And the checkpoint file should be valid

  Scenario: Checkpoint file contains complete state
    Given a 2D shock tube simulation with 500 particles
    When a checkpoint is saved at t=0.25
    Then the checkpoint file should exist
    And the checkpoint should contain:
      | Field           | Value |
      | particle_count  | 500   |
      | time           | 0.25  |
      | dimension      | 2     |
      | sph_type       | DISPH |
    And the checkpoint should include all particle positions
    And the checkpoint should include all particle velocities
    And the checkpoint should include all particle densities
    And the checkpoint should include all particle energies
    And the checkpoint should include simulation parameters
    And the checkpoint should have a valid SHA-256 checksum

  Scenario: Resume from checkpoint produces identical results
    Given a Sedov-Taylor simulation configuration
    When I run the simulation continuously from t=0.0 to t=1.0
    And I save the final particle states as "continuous_results"
    And I run the same simulation from t=0.0 to t=0.5
    And I save a checkpoint at t=0.5
    And I resume from the checkpoint and run to t=1.0
    And I save the final particle states as "resumed_results"
    Then the particle positions in "resumed_results" should match "continuous_results" within 1e-12
    And the particle velocities in "resumed_results" should match "continuous_results" within 1e-12
    And the particle densities in "resumed_results" should match "continuous_results" within 1e-12
    And the particle energies in "resumed_results" should match "continuous_results" within 1e-10
    And the total energy should match within 1e-10

  Scenario: Auto-checkpoint at regular time intervals
    Given a simulation with checkpoint_interval=0.1
    And checkpoint_enabled is true
    When the simulation runs from t=0.0 to t=0.5
    Then checkpoints should exist at the following times:
      | Time |
      | 0.1  |
      | 0.2  |
      | 0.3  |
      | 0.4  |
      | 0.5  |
    And each checkpoint file should be valid
    And checkpoint names should follow the pattern "checkpoint_t{time:.3f}.chk"

  Scenario: Auto-checkpoint with configurable interval
    Given a simulation with checkpoint_interval=0.05
    When the simulation runs from t=0.0 to t=0.2
    Then 4 checkpoint files should exist
    And the checkpoint times should be [0.05, 0.10, 0.15, 0.20]

  Scenario: Checkpoint cleanup keeps only recent checkpoints
    Given a simulation with checkpoint_interval=0.1
    And max_keep=3 checkpoints configured
    When the simulation creates 5 checkpoints at t=[0.1, 0.2, 0.3, 0.4, 0.5]
    Then only 3 checkpoint files should exist
    And the existing checkpoints should be at t=[0.3, 0.4, 0.5]
    And checkpoints at t=[0.1, 0.2] should have been deleted

  Scenario: Checkpoint on simulation interruption (SIGINT)
    Given a simulation is running
    And auto_checkpoint_on_interrupt is enabled
    When the simulation reaches t=0.3
    And I send SIGINT signal (Ctrl+C)
    Then a checkpoint should be saved immediately
    And the checkpoint time should be approximately 0.3
    And the simulation should exit gracefully
    And the checkpoint file should be valid

  Scenario: Checkpoint on simulation termination (SIGTERM)
    Given a simulation is running  
    And auto_checkpoint_on_interrupt is enabled
    When I send SIGTERM signal
    Then a checkpoint should be saved immediately
    And the simulation should exit gracefully

  Scenario: Resume from checkpoint with different parameters fails gracefully
    Given a checkpoint from a simulation with gamma=1.4
    When I attempt to resume with gamma=1.6666
    Then the resume operation should fail
    And an error message should indicate parameter mismatch
    And the error should specify "gamma: checkpoint=1.4, current=1.6666"

  Scenario: Resume from corrupted checkpoint fails with clear error
    Given a checkpoint file exists
    When I corrupt the checkpoint file by modifying bytes
    And I attempt to resume from the corrupted checkpoint
    Then the resume operation should fail
    And an error message should indicate checksum validation failed
    And no simulation should start

  Scenario: Checkpoint file format version compatibility
    Given a checkpoint file with format version 1
    When the checkpoint format version is 1
    Then the checkpoint should load successfully
    
  Scenario: Checkpoint includes metadata for reproducibility
    Given a simulation running from git commit "a3f2b1c"
    When a checkpoint is saved
    Then the checkpoint metadata should include:
      | Field            | Example Value      |
      | created_at       | 2025-11-01T14:32:10Z |
      | simulation_name  | sedov_taylor_2d    |
      | sph_type         | DISPH              |
      | dimension        | 2                  |
      | format_version   | 1                  |

  Scenario: Checkpoint directory structure
    Given a simulation named "shock_tube"
    And checkpoint_directory is "checkpoints"
    When checkpoints are created
    Then checkpoint files should be in "simulations/shock_tube/{run_id}/checkpoints/"
    And each checkpoint file should have extension ".chk"
    And checkpoint filenames should include the time value

  Scenario: Resume updates simulation run metadata
    Given a simulation that started at t=0.0
    When a checkpoint is saved at t=0.5
    And the simulation is resumed from the checkpoint
    Then the run metadata should indicate resumption
    And the metadata should record the checkpoint file path
    And the metadata should record the original start time
    And the metadata should record the resume time

  Scenario: Multiple resume cycles maintain accuracy
    Given a simulation from t=0.0 to t=1.0 run continuously
    When I run with checkpoints at t=[0.2, 0.4, 0.6, 0.8]
    And I resume from each checkpoint sequentially
    Then the final state should match the continuous run within 1e-10

  Scenario: Checkpoint binary format is compact and efficient
    Given a simulation with 10000 particles
    When a checkpoint is saved
    Then the checkpoint file size should be less than 2 MB
    And the save operation should complete in less than 100 milliseconds
    And the load operation should complete in less than 100 milliseconds

  Scenario: Checkpoint validation before resume
    Given a checkpoint file
    When I call validate_checkpoint(filepath)
    Then it should check the magic number "SPHCHKPT"
    And it should verify the format version
    And it should validate the checksum
    And it should return a validation report with:
      | Field          | Type    |
      | is_valid       | boolean |
      | error_message  | string  |
      | warnings       | list    |

  Scenario: Checkpoint manager configuration
    Given a CheckpointManager instance
    When I configure it with:
      | Parameter              | Value      |
      | enabled                | true       |
      | interval               | 0.1        |
      | max_keep               | 5          |
      | on_interrupt           | true       |
      | directory              | checkpoints |
    Then should_checkpoint(0.15) should return true
    And should_checkpoint(0.05) should return false
    And generate_checkpoint_path should create paths like "checkpoints/checkpoint_t0.100.chk"

  Scenario: Checkpoint preserves custom particle attributes
    Given a simulation with custom particle attributes:
      | Attribute        | Type   |
      | magnetic_field   | vector |
      | chemical_species | array  |
    When a checkpoint is saved
    Then the checkpoint should include all custom attributes
    And resuming should restore all custom attributes exactly

# Edge Cases and Error Handling

  Scenario: Checkpoint to read-only filesystem
    Given the checkpoint directory is read-only
    When a checkpoint save is attempted
    Then the save should fail gracefully
    And an error should be logged
    And the simulation should continue if possible

  Scenario: Resume from checkpoint with missing particle data
    Given a checkpoint file with incomplete particle data
    When resume is attempted
    Then the operation should fail
    And an error should indicate data corruption

  Scenario: Checkpoint with zero particles
    Given an empty simulation with 0 particles
    When a checkpoint save is attempted
    Then it should save successfully
    And the particle count should be 0
    And resuming should create an empty simulation

  Scenario: Very large simulation checkpoint (1M particles)
    Given a simulation with 1,000,000 particles
    When a checkpoint is saved
    Then the operation should complete successfully
    And the file size should be approximately 200 MB
    And the save time should be less than 2 seconds

# Performance Tests

  Scenario: Checkpoint overhead on simulation performance
    Given a simulation running without checkpoints
    When I measure the time to reach t=1.0
    And I run the same simulation with checkpoint_interval=0.1
    Then the overhead should be less than 5%
    And the simulation accuracy should be unaffected
