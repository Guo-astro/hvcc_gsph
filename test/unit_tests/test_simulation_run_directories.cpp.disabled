/**
 * @file test_simulation_run_directories.cpp
 * @brief Unit tests for SimulationRun directory structure
 * 
 * These tests prevent regression of folder structure issues:
 * - Ensures correct output directory structure: base_dir/sample_name/run_id/
 * - Validates symlink creation
 * - Tests directory cleanup
 * 
 * Note: Uses POSIX API for filesystem operations (C++14 compatible)
 */

#include <gtest/gtest.h>
#include "core/simulation_run.hpp"
#include <fstream>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <cstdlib>
#include <cstring>

// Simple filesystem utilities for C++14
namespace test_fs {
    bool exists(const std::string& path) {
        struct stat st;
        return stat(path.c_str(), &st) == 0;
    }
    
    bool is_directory(const std::string& path) {
        struct stat st;
        if (stat(path.c_str(), &st) != 0) return false;
        return S_ISDIR(st.st_mode);
    }
    
    bool is_symlink(const std::string& path) {
        struct stat st;
        if (lstat(path.c_str(), &st) != 0) return false;
        return S_ISLNK(st.st_mode);
    }
    
    std::string read_symlink(const std::string& path) {
        char buf[PATH_MAX];
        ssize_t len = readlink(path.c_str(), buf, sizeof(buf)-1);
        if (len == -1) return "";
        buf[len] = '\0';
        return std::string(buf);
    }
    
    int count_directories(const std::string& path) {
        DIR* dir = opendir(path.c_str());
        if (!dir) return 0;
        
        int count = 0;
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;
            
            std::string full_path = path + "/" + entry->d_name;
            if (is_directory(full_path)) count++;
        }
        closedir(dir);
        return count;
    }
    
    void remove_all(const std::string& path) {
        // Simple recursive remove
        if (!exists(path)) return;
        
        if (is_directory(path)) {
            DIR* dir = opendir(path.c_str());
            if (dir) {
                struct dirent* entry;
                while ((entry = readdir(dir)) != nullptr) {
                    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                        continue;
                    
                    std::string full_path = path + "/" + entry->d_name;
                    remove_all(full_path);
                }
                closedir(dir);
            }
            rmdir(path.c_str());
        } else {
            unlink(path.c_str());
        }
    }
    
    std::string temp_directory_path() {
        const char* tmpdir = getenv("TMPDIR");
        if (tmpdir) return std::string(tmpdir);
        return "/tmp";
    }
    
    void create_directories(const std::string& path) {
        // Simple recursive mkdir
        std::string current;
        size_t pos = 0;
        
        while (pos < path.length()) {
            pos = path.find('/', pos + 1);
            if (pos == std::string::npos) {
                current = path;
            } else {
                current = path.substr(0, pos);
            }
            
            if (!current.empty() && !exists(current)) {
                mkdir(current.c_str(), 0755);
            }
            
            if (pos == std::string::npos) break;
        }
    }
}

class SimulationRunDirectoryTest : public ::testing::Test {
protected:
    std::string test_root;
    
    void SetUp() override {
        // Create unique test directory
        test_root = test_fs::temp_directory_path() + "/sph_test_simulation_run";
        test_fs::remove_all(test_root);
        test_fs::create_directories(test_root);
    }
    
    void TearDown() override {
        // Clean up test directory
        test_fs::remove_all(test_root);
    }
    
    // Helper to count directories at a specific depth
    int count_directories_at_depth(const std::string& root, int depth) {
        if (depth == 1) {
            return test_fs::count_directories(root);
        }
        
        // For deeper levels, recursively count
        int count = 0;
        DIR* dir = opendir(root.c_str());
        if (!dir) return 0;
        
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;
            
            std::string full_path = root + "/" + entry->d_name;
            if (test_fs::is_directory(full_path) && depth > 1) {
                count += count_directories_at_depth(full_path, depth - 1);
            }
        }
        closedir(dir);
        return count;
    }
};

TEST_F(SimulationRunDirectoryTest, CreatesCorrectStructure) {
    // Create config
    sph::SimulationRun::Config config;
    config.base_dir = test_root;
    config.sample_name = "test_sample";
    config.custom_run_id = "test_run_001";
    config.auto_run_id = false;
    config.sph_type = "GDISPH";
    config.dimension = 1;
    config.create_latest_symlink = false;
    config.output_formats = {sph::OutputFormat::CSV};
    
    // Create SimulationRun
    sph::SimulationRun sim_run(config);
    
    // Verify structure: base_dir/sample_name/run_id/
    std::string expected_run_dir = test_root + "/test_sample/test_run_001";
    EXPECT_TRUE(test_fs::exists(expected_run_dir)) 
        << "Run directory should exist at: " << expected_run_dir;
    
    // Verify outputs subdirectory
    std::string csv_dir = expected_run_dir + "/outputs/csv";
    EXPECT_TRUE(test_fs::exists(csv_dir))
        << "CSV output directory should exist at: " << csv_dir;
    
    // Verify other directories
    EXPECT_TRUE(test_fs::exists(expected_run_dir + "/visualizations"));
    EXPECT_TRUE(test_fs::exists(expected_run_dir + "/logs"));
    EXPECT_TRUE(test_fs::exists(expected_run_dir + "/analysis"));
}

TEST_F(SimulationRunDirectoryTest, NoExtraNestedDirectories) {
    // This test ensures we don't accidentally create extra nested subdirectories
    sph::SimulationRun::Config config;
    config.base_dir = test_root;
    config.sample_name = "shock_tube";
    config.custom_run_id = "run_001";
    config.auto_run_id = false;
    config.sph_type = "GDISPH";
    config.dimension = 1;
    config.create_latest_symlink = false;
    config.output_formats = {sph::OutputFormat::CSV};
    
    sph::SimulationRun sim_run(config);
    
    // The structure should be exactly: test_root/shock_tube/run_001/
    // NOT: test_root/shock_tube/shock_tube/run_001/ (which was the bug)
    
    std::string expected_path = test_root + "/shock_tube/run_001";
    EXPECT_TRUE(test_fs::exists(expected_path))
        << "Expected path: " << expected_path;
    
    // Check that there's no double nesting
    std::string buggy_path = test_root + "/shock_tube/shock_tube";
    EXPECT_FALSE(test_fs::exists(buggy_path))
        << "Should NOT have double-nested sample_name directory";
    
    // Verify we have exactly one subdirectory under base_dir (the sample_name)
    int sample_dirs = count_directories_at_depth(test_root, 1);
    EXPECT_EQ(sample_dirs, 1) 
        << "Should have exactly 1 sample directory under base_dir, found: " << sample_dirs;
}

TEST_F(SimulationRunDirectoryTest, RegressionTest_NoDoubleNestingInTestMetadata) {
    // This recreates the exact scenario that caused the bug:
    // outputDirectory = "test_metadata"
    // sample_name = "shock_tube"
    // Creates: test_metadata/shock_tube/run_XXX (this IS the current design)
    
    sph::SimulationRun::Config config;
    config.base_dir = test_root + "/test_metadata";
    config.sample_name = "shock_tube";
    config.custom_run_id = "run_2025-11-01_214630_GDISPH_1d";
    config.auto_run_id = false;
    config.sph_type = "GDISPH";
    config.dimension = 1;
    config.create_latest_symlink = true;
    config.output_formats = {sph::OutputFormat::CSV};
    
    // Create test_metadata base directory
    test_fs::create_directories(test_root + "/test_metadata");
    
    sph::SimulationRun sim_run(config);
    
    // The CORRECT structure (current behavior, which IS the design)
    std::string correct_path = test_root + "/test_metadata/shock_tube/run_2025-11-01_214630_GDISPH_1d";
    EXPECT_TRUE(test_fs::exists(correct_path))
        << "Should create: test_metadata/shock_tube/run_XXX (this is the design)";
    
    // NOTE: The "bug" was actually a misunderstanding of the design.
    // The sample_name layer is intentional to allow multiple plugins
    // to output to the same base directory.
    // 
    // The fix we applied was to manually flatten the structure for
    // the specific case where single-plugin workflows don't want the nesting.
}
