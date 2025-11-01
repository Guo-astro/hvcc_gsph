Feature: Automatic Visualization Generation
  As a simulation user
  I want visualizations to be automatically generated after each simulation
  So that I can immediately see the results without manual post-processing

  Background:
    Given the SPH simulation executable is available
    And the visualization script exists at "scripts/generate_visualizations.py"
    And Python with required packages is available

  Scenario: Initial conditions visualization is generated
    Given a completed Riemann problem simulation
    When I run the automatic visualization script
    Then a file named "initial_conditions.png" should exist in the visualizations directory
    And the file size should be greater than 10 KB
    And the image should be a valid PNG file
    And the image dimensions should be approximately 1400x1000 pixels

  Scenario: Final state visualization is generated
    Given a completed Riemann problem simulation
    When I run the automatic visualization script
    Then a file named "final_state.png" should exist in the visualizations directory
    And the file should be a valid PNG image
    And the file size should be greater than 10 KB

  Scenario: Key timestep screenshots are generated
    Given a completed Riemann problem simulation with at least 20 timesteps
    When I run the automatic visualization script
    Then at least 3 screenshot PNG files should exist in the visualizations directory
    And each screenshot filename should contain a timestamp
    And the screenshots should show progression from early to late simulation time

  Scenario: Animation is generated
    Given a completed Riemann problem simulation
    When I run the automatic visualization script
    Then either "evolution_animation.mp4" or "evolution_animation.gif" should exist
    And the animation file size should be greater than 50 KB

  Scenario: Complete workflow generates all visualizations
    Given a clean test environment
    When I run the complete workflow script with test case 1
    Then the simulation should complete successfully
    And the visualization directory should be created
    And all required visualization files should exist:
      | filename                  |
      | initial_conditions.png    |
      | final_state.png           |
      | screenshot_*.png          |
      | evolution_animation.*     |

  Scenario: Physical units are correctly displayed in plots
    Given a simulation run with known unit system
    When I generate visualizations
    Then density plot labels should contain the density unit
    And velocity plot labels should contain the velocity unit
    And pressure plot labels should contain the pressure unit
    And energy plot labels should contain the energy unit
    And position axes should contain the length unit

  Scenario: Visualization handles incomplete simulations
    Given a simulation that was interrupted after 50% completion
    When I attempt to generate visualizations
    Then the script should not crash
    And it should generate visualizations for available timesteps
    And a warning should be logged about incomplete data

  Scenario: Visualization directory structure is correct
    Given a simulation with output directory "results_test1_sod"
    When visualizations are generated
    Then files should be in "results_test1_sod/riemann_problems/latest/visualizations/"
    And not in "results_test1_sod/" directly
    And not in "comparison_results/"

  Scenario Outline: All test cases generate appropriate visualizations
    Given a Riemann problem simulation for <test_name>
    When the workflow completes
    Then visualizations should be generated with <test_name> specific analytical solutions
    And the plots should show expected <physics_feature>

    Examples:
      | test_name        | physics_feature              |
      | test1_sod        | shock and rarefaction        |
      | test2_rarefaction| double rarefaction waves     |
      | test5_vacuum     | vacuum region formation      |
