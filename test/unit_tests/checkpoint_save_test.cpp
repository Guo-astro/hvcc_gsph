#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include "utilities/checkpoint_manager.hpp"
#include "core/simulation.hpp"
#include "core/parameters.hpp"

namespace fs = std::filesystem;

namespace sph {
namespace test {

/**
 * @brief Test fixture for checkpoint save functionality
 */
class CheckpointSaveTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test directory
        test_dir_ = fs::temp_directory_path() / "sph_checkpoint_test";
        fs::create_directories(test_dir_);
        
        // Initialize simple simulation
        sim_ = std::make_shared<Simulation>();
        params_ = std::make_shared<SPHParameters>();
        
        // Set basic parameters
        params_->dimension = 1;
        params_->sph_type = "DISPH";
        params_->gamma = 1.4;
        params_->time.current = 0.0;
        params_->time.dt = 0.001;
        params_->time.step = 0;
    }
    
    void TearDown() override {
        // Clean up test files
        if (fs::exists(test_dir_)) {
            fs::remove_all(test_dir_);
        }
    }
    
    /**
     * @brief Create a simple test simulation with N particles
     */
    void create_test_simulation(int N) {
        real dx = 1.0 / N;
        for (int i = 0; i < N; i++) {
            SPHParticle p;
            p.r[0] = i * dx;
            p.r[1] = 0.0;
            p.r[2] = 0.0;
            p.v[0] = 0.0;
            p.v[1] = 0.0;
            p.v[2] = 0.0;
            p.dens = 1.0;
            p.pres = 0.1;
            p.ene = 1.0;
            p.mass = 1.0 / N;
            sim_->add_particle(p);
        }
        params_->time.current = 0.5;
        params_->time.dt = 0.001;
        params_->time.step = 500;
    }
    
    std::shared_ptr<Simulation> sim_;
    std::shared_ptr<SPHParameters> params_;
    fs::path test_dir_;
};

// ============================================================================
// Basic Save Functionality Tests
// ============================================================================

TEST_F(CheckpointSaveTest, SaveCreatesFile) {
    // Given: A simple simulation state
    create_test_simulation(100);
    
    CheckpointManager mgr;
    std::string filepath = (test_dir_ / "test_checkpoint.chk").string();
    
    // When: Save checkpoint
    ASSERT_NO_THROW(mgr.save_checkpoint(filepath, *sim_, *params_));
    
    // Then: File should exist
    EXPECT_TRUE(fs::exists(filepath));
}

TEST_F(CheckpointSaveTest, SavedFileHasCorrectMagic) {
    // Given: A simulation
    create_test_simulation(50);
    
    CheckpointManager mgr;
    std::string filepath = (test_dir_ / "test_checkpoint.chk").string();
    mgr.save_checkpoint(filepath, *sim_, *params_);
    
    // When: Read file header
    std::ifstream file(filepath, std::ios::binary);
    ASSERT_TRUE(file.is_open());
    
    char magic[9] = {0};
    file.read(magic, 8);
    file.close();
    
    // Then: Magic number should be "SPHCHKPT"
    EXPECT_STREQ(magic, "SPHCHKPT");
}

TEST_F(CheckpointSaveTest, SavedFileHasCorrectVersion) {
    // Given: A simulation
    create_test_simulation(50);
    
    CheckpointManager mgr;
    std::string filepath = (test_dir_ / "test_checkpoint.chk").string();
    mgr.save_checkpoint(filepath, *sim_, *params_);
    
    // When: Read version from header
    std::ifstream file(filepath, std::ios::binary);
    file.seekg(8);  // Skip magic
    
    uint32_t version;
    file.read(reinterpret_cast<char*>(&version), sizeof(version));
    file.close();
    
    // Then: Version should be 1
    EXPECT_EQ(version, 1u);
}

TEST_F(CheckpointSaveTest, SaveIncludesAllParticles) {
    // Given: Simulation with known particle count
    const int N = 100;
    create_test_simulation(N);
    
    CheckpointManager mgr;
    std::string filepath = (test_dir_ / "test_checkpoint.chk").string();
    mgr.save_checkpoint(filepath, *sim_, *params_);
    
    // When: Read particle count from header
    std::ifstream file(filepath, std::ios::binary);
    file.seekg(296);  // Offset to particle_count field
    
    int64_t particle_count;
    file.read(reinterpret_cast<char*>(&particle_count), sizeof(particle_count));
    file.close();
    
    // Then: Particle count should match
    EXPECT_EQ(particle_count, N);
}

TEST_F(CheckpointSaveTest, SaveRecordsCorrectTime) {
    // Given: Simulation at specific time
    create_test_simulation(50);
    const real expected_time = 0.5;
    params_->time.current = expected_time;
    
    CheckpointManager mgr;
    std::string filepath = (test_dir_ / "test_checkpoint.chk").string();
    mgr.save_checkpoint(filepath, *sim_, *params_);
    
    // When: Read time from header
    std::ifstream file(filepath, std::ios::binary);
    file.seekg(272);  // Offset to time field
    
    double saved_time;
    file.read(reinterpret_cast<char*>(&saved_time), sizeof(saved_time));
    file.close();
    
    // Then: Time should match
    EXPECT_DOUBLE_EQ(saved_time, expected_time);
}

TEST_F(CheckpointSaveTest, SaveRecordsTimestep) {
    // Given: Simulation with specific timestep
    create_test_simulation(50);
    const real expected_dt = 0.001;
    params_->time.dt = expected_dt;
    
    CheckpointManager mgr;
    std::string filepath = (test_dir_ / "test_checkpoint.chk").string();
    mgr.save_checkpoint(filepath, *sim_, *params_);
    
    // When: Read dt from header
    std::ifstream file(filepath, std::ios::binary);
    file.seekg(280);  // Offset to dt field
    
    double saved_dt;
    file.read(reinterpret_cast<char*>(&saved_dt), sizeof(saved_dt));
    file.close();
    
    // Then: Timestep should match
    EXPECT_DOUBLE_EQ(saved_dt, expected_dt);
}

TEST_F(CheckpointSaveTest, SaveRecordsStepNumber) {
    // Given: Simulation at specific step
    create_test_simulation(50);
    const int64_t expected_step = 500;
    params_->time.step = expected_step;
    
    CheckpointManager mgr;
    std::string filepath = (test_dir_ / "test_checkpoint.chk").string();
    mgr.save_checkpoint(filepath, *sim_, *params_);
    
    // When: Read step from header
    std::ifstream file(filepath, std::ios::binary);
    file.seekg(288);  // Offset to step field
    
    int64_t saved_step;
    file.read(reinterpret_cast<char*>(&saved_step), sizeof(saved_step));
    file.close();
    
    // Then: Step should match
    EXPECT_EQ(saved_step, expected_step);
}

TEST_F(CheckpointSaveTest, SaveRecordsDimension) {
    // Given: Simulation with specific dimension
    create_test_simulation(50);
    const int expected_dim = 1;
    params_->dimension = expected_dim;
    
    CheckpointManager mgr;
    std::string filepath = (test_dir_ / "test_checkpoint.chk").string();
    mgr.save_checkpoint(filepath, *sim_, *params_);
    
    // When: Read dimension from header
    std::ifstream file(filepath, std::ios::binary);
    file.seekg(12);  // Offset to dimension field
    
    int32_t saved_dim;
    file.read(reinterpret_cast<char*>(&saved_dim), sizeof(saved_dim));
    file.close();
    
    // Then: Dimension should match
    EXPECT_EQ(saved_dim, expected_dim);
}

// ============================================================================
// File Size and Performance Tests
// ============================================================================

TEST_F(CheckpointSaveTest, FileSizeIsReasonable) {
    // Given: Simulation with 1000 particles
    const int N = 1000;
    create_test_simulation(N);
    
    CheckpointManager mgr;
    std::string filepath = (test_dir_ / "test_checkpoint.chk").string();
    mgr.save_checkpoint(filepath, *sim_, *params_);
    
    // When: Check file size
    auto file_size = fs::file_size(filepath);
    
    // Then: File size should be reasonable
    // Header (512) + params (~2KB) + particles (N × ~200 bytes) + checksum (32)
    // For 1000 particles: ~202 KB
    EXPECT_GT(file_size, 200'000);   // At least 200 KB
    EXPECT_LT(file_size, 300'000);   // Less than 300 KB
}

TEST_F(CheckpointSaveTest, SavePerformanceIsAcceptable) {
    // Given: Simulation with 10000 particles
    const int N = 10000;
    create_test_simulation(N);
    
    CheckpointManager mgr;
    std::string filepath = (test_dir_ / "test_checkpoint.chk").string();
    
    // When: Measure save time
    auto start = std::chrono::high_resolution_clock::now();
    mgr.save_checkpoint(filepath, *sim_, *params_);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Then: Should complete in less than 500ms
    EXPECT_LT(duration.count(), 500);
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(CheckpointSaveTest, SaveWithZeroParticles) {
    // Given: Empty simulation
    // (no particles added)
    
    CheckpointManager mgr;
    std::string filepath = (test_dir_ / "test_checkpoint.chk").string();
    
    // When: Save checkpoint
    ASSERT_NO_THROW(mgr.save_checkpoint(filepath, *sim_, *params_));
    
    // Then: File should exist
    EXPECT_TRUE(fs::exists(filepath));
    
    // And: Particle count should be 0
    std::ifstream file(filepath, std::ios::binary);
    file.seekg(296);
    int64_t count;
    file.read(reinterpret_cast<char*>(&count), sizeof(count));
    file.close();
    
    EXPECT_EQ(count, 0);
}

TEST_F(CheckpointSaveTest, SaveToNonExistentDirectory) {
    // Given: Path with non-existent directory
    create_test_simulation(50);
    
    CheckpointManager mgr;
    std::string filepath = (test_dir_ / "subdir" / "test.chk").string();
    
    // When/Then: Should create directory and save
    // (Or throw meaningful error - implementation dependent)
    // For now, expect it to fail gracefully
    // TODO: Update based on actual implementation behavior
}

TEST_F(CheckpointSaveTest, OverwriteExistingCheckpoint) {
    // Given: Existing checkpoint file
    create_test_simulation(50);
    
    CheckpointManager mgr;
    std::string filepath = (test_dir_ / "test_checkpoint.chk").string();
    mgr.save_checkpoint(filepath, *sim_, *params_);
    
    auto original_size = fs::file_size(filepath);
    
    // When: Save again with different data
    create_test_simulation(100);  // Different number of particles
    mgr.save_checkpoint(filepath, *sim_, *params_);
    
    auto new_size = fs::file_size(filepath);
    
    // Then: File should be overwritten with new size
    EXPECT_NE(original_size, new_size);
}

// ============================================================================
// Metadata Tests
// ============================================================================

TEST_F(CheckpointSaveTest, SaveIncludesTimestamp) {
    // Given: A simulation
    create_test_simulation(50);
    
    CheckpointManager mgr;
    std::string filepath = (test_dir_ / "test_checkpoint.chk").string();
    
    // When: Save checkpoint
    mgr.save_checkpoint(filepath, *sim_, *params_);
    
    // Then: Header should contain timestamp
    std::ifstream file(filepath, std::ios::binary);
    file.seekg(16);  // Offset to timestamp field
    
    char timestamp[64] = {0};
    file.read(timestamp, 64);
    file.close();
    
    // Timestamp should not be empty and should contain ISO 8601 format indicators
    std::string ts_str(timestamp);
    EXPECT_FALSE(ts_str.empty());
    EXPECT_NE(ts_str.find("2025"), std::string::npos);  // Year should be present
}

TEST_F(CheckpointSaveTest, SaveRecordsSPHType) {
    // Given: Simulation with specific SPH type
    create_test_simulation(50);
    params_->sph_type = "DISPH";
    
    CheckpointManager mgr;
    std::string filepath = (test_dir_ / "test_checkpoint.chk").string();
    mgr.save_checkpoint(filepath, *sim_, *params_);
    
    // When: Read SPH type from header
    std::ifstream file(filepath, std::ios::binary);
    file.seekg(208);  // Offset to sph_type field
    
    char sph_type[64] = {0};
    file.read(sph_type, 64);
    file.close();
    
    // Then: SPH type should match
    EXPECT_STREQ(sph_type, "DISPH");
}

} // namespace test
} // namespace sph
