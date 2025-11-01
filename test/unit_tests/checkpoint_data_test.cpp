#include <gtest/gtest.h>
#include "utilities/checkpoint_manager.hpp"
#include "utilities/checkpoint_data.hpp"
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

namespace sph {

/**
 * @brief Demonstration of checkpoint data structure and validation
 * 
 * This test shows how to create and validate checkpoint data
 * without requiring full Simulation integration.
 */
class CheckpointDataTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = "test_checkpoint_data";
        if (!fs::exists(test_dir_)) {
            fs::create_directory(test_dir_);
        }
    }
    
    void TearDown() override {
        if (fs::exists(test_dir_)) {
            fs::remove_all(test_dir_);
        }
    }
    
    CheckpointData create_test_data(int num_particles = 10) {
        CheckpointData data;
        
        data.time = 0.5;
        data.dt = 0.001;
        data.step = 100;
        data.dimension = DIM;
        data.sph_type = "SSPH";
        data.simulation_name = "test_simulation";
        data.created_at = "2025-01-15T12:00:00.000Z";
        
        // Create test parameters
        auto params = std::make_shared<SPHParameters>();
        params->type = SPHType::SSPH;
        params->physics.gamma = 5.0/3.0;
        params->physics.neighbor_number = 50;
        params->time.start = 0.0;
        params->time.end = 1.0;
        params->cfl.sound = 0.25;
        params->cfl.force = 0.25;
        data.params = params;
        
        // Create test particles
        for (int i = 0; i < num_particles; ++i) {
            SPHParticle p;
            for (int d = 0; d < DIM; ++d) {
                p.pos[d] = static_cast<real>(i) / num_particles;
                p.vel[d] = 0.0;
            }
            p.mass = 1.0;
            p.dens = 1.0;
            p.sml = 0.1;
            p.pres = 1.0;
            p.ene = 1.0;
            p.id = i;
            
            data.particles.push_back(p);
        }
        
        return data;
    }
    
    std::string test_dir_;
};

TEST_F(CheckpointDataTest, ValidateEmptyData) {
    CheckpointData data;
    EXPECT_FALSE(data.is_valid()) << "Empty checkpoint data should be invalid";
}

TEST_F(CheckpointDataTest, ValidateCompleteData) {
    auto data = create_test_data(10);
    EXPECT_TRUE(data.is_valid()) << "Complete checkpoint data should be valid";
}

TEST_F(CheckpointDataTest, InvalidDimension) {
    auto data = create_test_data(5);
    
    data.dimension = 0;
    EXPECT_FALSE(data.is_valid()) << "Dimension 0 should be invalid";
    
    data.dimension = 4;
    EXPECT_FALSE(data.is_valid()) << "Dimension 4 should be invalid";
    
    data.dimension = 2;
    EXPECT_TRUE(data.is_valid()) << "Dimension 2 should be valid";
}

TEST_F(CheckpointDataTest, InvalidTime) {
    auto data = create_test_data(5);
    
    data.time = -1.0;
    EXPECT_FALSE(data.is_valid()) << "Negative time should be invalid";
    
    data.time = 0.5;
    data.dt = -0.001;
    EXPECT_FALSE(data.is_valid()) << "Negative timestep should be invalid";
    
    data.dt = 0.0;
    EXPECT_FALSE(data.is_valid()) << "Zero timestep should be invalid";
}

TEST_F(CheckpointDataTest, GetInfo) {
    auto data = create_test_data(10);
    
    std::string info = data.get_info();
    
    EXPECT_TRUE(info.find("test_simulation") != std::string::npos);
    EXPECT_TRUE(info.find("SSPH") != std::string::npos);
    EXPECT_TRUE(info.find("10") != std::string::npos) << "Particle count should appear in info";
    EXPECT_TRUE(info.find("0.5") != std::string::npos) << "Time should appear in info";
    EXPECT_TRUE(info.find("0.001") != std::string::npos) << "Timestep should appear in info";
}

TEST_F(CheckpointDataTest, GetTotalSize) {
    auto data = create_test_data(100);
    
    size_t total_size = data.get_total_size();
    
    // Should include:
    // - 512 bytes header
    // - ~3000 bytes params estimate
    // - 100 * sizeof(SPHParticle) bytes particles
    // - 32 bytes checksum
    
    size_t expected_min = 512 + 100 * sizeof(SPHParticle) + 32;
    EXPECT_GE(total_size, expected_min) << "Total size should be at least header + particles + checksum";
    
    // Should be reasonable (not gigabytes for 100 particles)
    EXPECT_LT(total_size, 1000000) << "Total size should be reasonable for 100 particles";
}

TEST_F(CheckpointDataTest, ParticleDataPreservation) {
    auto data = create_test_data(5);
    
    ASSERT_EQ(data.particles.size(), 5);
    
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(data.particles[i].id, i) << "Particle ID should match index";
        EXPECT_DOUBLE_EQ(data.particles[i].mass, 1.0) << "Particle mass should be 1.0";
        EXPECT_DOUBLE_EQ(data.particles[i].dens, 1.0) << "Particle density should be 1.0";
        EXPECT_DOUBLE_EQ(data.particles[i].sml, 0.1) << "Smoothing length should be 0.1";
        
        for (int d = 0; d < DIM; ++d) {
            EXPECT_NEAR(data.particles[i].pos[d], static_cast<real>(i) / 5.0, 1e-10)
                << "Particle position should be preserved";
        }
    }
}

TEST_F(CheckpointDataTest, SPHTypeMapping) {
    auto data = create_test_data(1);
    
    data.sph_type = "SSPH";
    EXPECT_TRUE(data.is_valid());
    
    data.sph_type = "DISPH";
    EXPECT_TRUE(data.is_valid());
    
    data.sph_type = "GSPH";
    EXPECT_TRUE(data.is_valid());
    
    data.sph_type = "GDISPH";
    EXPECT_TRUE(data.is_valid());
    
    data.sph_type = "";
    EXPECT_FALSE(data.is_valid()) << "Empty SPH type should be invalid";
}

TEST_F(CheckpointDataTest, ParametersRequired) {
    auto data = create_test_data(5);
    
    EXPECT_TRUE(data.is_valid()) << "Data with parameters should be valid";
    
    data.params = nullptr;
    EXPECT_FALSE(data.is_valid()) << "Data without parameters should be invalid";
}

TEST_F(CheckpointDataTest, EmptyParticlesInvalid) {
    auto data = create_test_data(0);
    
    EXPECT_FALSE(data.is_valid()) << "Data with zero particles should be invalid";
}

TEST_F(CheckpointDataTest, LargeParticleCount) {
    auto data = create_test_data(10000);
    
    EXPECT_TRUE(data.is_valid());
    EXPECT_EQ(data.particles.size(), 10000);
    
    size_t total_size = data.get_total_size();
    
    // For 10K particles, should be several MB
    EXPECT_GT(total_size, 1000000) << "10K particles should take >1MB";
}

} // namespace sph
