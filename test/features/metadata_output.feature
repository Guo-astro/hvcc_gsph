Feature: CSV Metadata Output
  As a simulation user
  I want metadata to be written only once per simulation run
  So that I don't have hundreds of redundant metadata files cluttering my output directory

  Background:
    Given the SPH simulation executable is available
    And the Riemann problems plugin is built

  Scenario: Single metadata file is generated
    Given a clean test output directory
    When I run a 1D Riemann problem simulation with test case 1
    Then exactly 1 file named "metadata.json" should exist in the CSV output directory
    And there should be 0 files matching the pattern "*.meta.json" in the CSV output directory
    And the metadata file should be readable JSON
    And at least 10 CSV snapshot files should be generated

  Scenario: Metadata contains correct structure
    Given a completed simulation run exists
    When I read the metadata.json file
    Then it should contain a "units" section
    And it should contain a "simulation" section
    And it should contain a "columns" array
    And the "units" section should have keys: length, time, mass, density, pressure, energy
    And the "simulation" section should have keys: dimension, particle_count
    And each item in "columns" should have keys: name, unit, description

  Scenario: Metadata file location is consistent across runs
    Given I run 3 simulations with different test cases
    When I check each run's CSV directory
    Then each should have exactly 1 "metadata.json" file
    And none should have files matching "0*.meta.json"

  Scenario: Metadata is written only on first snapshot
    Given a simulation configured to output every 0.001 time units for 0.05 total time
    When the simulation completes
    Then the CSV directory should contain approximately 50 CSV files
    And the CSV directory should contain exactly 1 metadata.json file
    And the metadata.json should be created before or with the first CSV file
