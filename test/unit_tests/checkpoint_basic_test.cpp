#include <gtest/gtest.h>
#include "utilities/checkpoint_manager.hpp"
#include "core/simulation.hpp"
#include "core/parameters.hpp"
#include "core/particle.hpp"
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

namespace sph {

class CheckpointBasicTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test directory
        test_dir_ = "test_checkpoints";
        if (!fs::exists(test_dir_)) {
            fs::create_directory(test_dir_);
        }
        
        // Create basic parameters
        params_ = std::make_shared<SPHParameters>();
        params_->type = SPHType::SSPH;
        params_->physics.gamma = 5.0/3.0;
        params_->physics.neighbor_number = 50;
        params_->time.start = 0.0;
        params_->time.end = 1.0;
        params_->cfl.sound = 0.25;
        params_->cfl.force = 0.25;
    }
    
    void TearDown() override {
        // Clean up test files
        if (fs::exists(test_dir_)) {
            fs::remove_all(test_dir_);
        }
    }
    
    std::string test_dir_;
    std::shared_ptr<SPHParameters> params_;
};

TEST_F(CheckpointBasicTest, CheckpointDataCreation) {
    CheckpointData data;
    
    data.time = 0.5;
    data.dt = 0.001;
    data.step = 100;
    data.dimension = DIM;
    data.sph_type = "SSPH";
    data.simulation_name = "test_sim";
    data.created_at = "2025-01-01T00:00:00Z";
    data.params = params_;
    
    // Create a test particle
    SPHParticle p;
    for (int i = 0; i < DIM; ++i) {
        p.pos[i] = 1.0;
        p.vel[i] = 0.1;
    }
    p.dens = 1.0;
    p.mass = 1.0;
    p.sml = 0.1;
    
    data.particles.push_back(p);
    
    EXPECT_TRUE(data.is_valid());
    EXPECT_EQ(data.particles.size(), 1);
    
    std::string info = data.get_info();
    EXPECT_TRUE(info.find("test_sim") != std::string::npos);
    EXPECT_TRUE(info.find("SSPH") != std::string::npos);
}

TEST_F(CheckpointBasicTest, CheckpointManagerCreation) {
    CheckpointManager manager;
    
    // Check default config
    EXPECT_FALSE(manager.should_checkpoint(1.0));
    
    // Enable auto-checkpoint
    CheckpointManager::AutoCheckpointConfig config;
    config.enabled = true;
    config.interval = 0.5;
    config.max_keep = 3;
    
    manager.configure_auto_checkpoint(config);
    EXPECT_TRUE(manager.should_checkpoint(0.5));
}

TEST_F(CheckpointBasicTest, DISABLED_BasicSaveLoad) {
    // This test is disabled until we implement load_checkpoint()
    // For now, just test that we can create a checkpoint manager
    
    CheckpointManager manager;
    std::string checkpoint_path = test_dir_ + "/test.chk";
    
    // Create a simple simulation
    // Note: This will fail because Simulation requires proper initialization
    // We'll enable this test once we have better test fixtures
    
    SUCCEED() << "Placeholder for save/load test";
}

} // namespace sph
