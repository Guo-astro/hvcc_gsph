# Makefile for Razor-Thin HVCC GDISPH Workflow
# Automates building, running simulations, and creating animations

# Paths
WORKFLOW_DIR := $(shell pwd)
PROJECT_ROOT := $(WORKFLOW_DIR)/../../..
BUILD_DIR := $(PROJECT_ROOT)/build
SPH3D := $(BUILD_DIR)/sph3d

# Step 1: Relaxation
RELAX_DIR := $(WORKFLOW_DIR)/01_relaxation
RELAX_BUILD := $(RELAX_DIR)/build
RELAX_PLUGIN := $(RELAX_BUILD)/libdisk_relaxation_plugin.dylib
RELAX_CONFIG := $(RELAX_DIR)/config/production.json
RELAX_CONFIG_TEST := $(RELAX_DIR)/config/test.json
RELAX_OUTPUT := $(RELAX_DIR)/output/disk_relaxation

# Step 2: Flyby
FLYBY_DIR := $(WORKFLOW_DIR)/02_flyby
FLYBY_BUILD := $(FLYBY_DIR)/build
FLYBY_PLUGIN := $(FLYBY_BUILD)/librazor_thin_hvcc_gdisph_plugin.dylib
FLYBY_CONFIG := $(FLYBY_DIR)/config_gdisph.json

# Initial conditions
IC_DIR := $(WORKFLOW_DIR)/initial_conditions
IC_FILE := $(IC_DIR)/relaxed_disk.csv

# Animation
ANIM_SCRIPT := $(RELAX_DIR)/scripts/animate_relaxation.py
ANIM_OUTPUT := $(RELAX_DIR)/results/animations/relaxation_animation.mp4
ANIM_STATIC := $(RELAX_DIR)/results/plots/relaxation_comparison.png

# Colors for output
RED := \033[0;31m
GREEN := \033[0;32m
YELLOW := \033[1;33m
BLUE := \033[0;34m
NC := \033[0m # No Color

.PHONY: all help clean build-relax build-flyby build-all \
        run-relax run-relax-test run-flyby run-all \
        animate clean-output clean-all status check-deps

# Default target
all: help

##@ General

help: ## Display this help message
	@echo "$(BLUE)━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━$(NC)"
	@echo "$(BLUE)  Razor-Thin HVCC GDISPH Workflow - Makefile$(NC)"
	@echo "$(BLUE)━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━$(NC)"
	@echo ""
	@awk 'BEGIN {FS = ":.*##"; printf "Usage: make $(YELLOW)<target>$(NC)\n\n"} \
		/^[a-zA-Z_-]+:.*?##/ { printf "  $(GREEN)%-20s$(NC) %s\n", $$1, $$2 } \
		/^##@/ { printf "\n$(BLUE)%s$(NC)\n", substr($$0, 5) }' $(MAKEFILE_LIST)
	@echo ""

status: ## Show current workflow status
	@echo "$(BLUE)━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━$(NC)"
	@echo "$(BLUE)  Workflow Status$(NC)"
	@echo "$(BLUE)━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━$(NC)"
	@echo ""
	@echo "$(YELLOW)Step 1: Relaxation$(NC)"
	@if [ -f "$(RELAX_PLUGIN)" ]; then \
		echo "  ✓ Plugin built: $(GREEN)$(RELAX_PLUGIN)$(NC)"; \
	else \
		echo "  ✗ Plugin not built: $(RED)$(RELAX_PLUGIN)$(NC)"; \
	fi
	@if [ -d "$(RELAX_OUTPUT)" ]; then \
		RUNS=$$(ls -d $(RELAX_OUTPUT)/run_* 2>/dev/null | wc -l | tr -d ' '); \
		echo "  ℹ Simulation runs: $$RUNS"; \
		if [ $$RUNS -gt 0 ]; then \
			LATEST=$$(ls -td $(RELAX_OUTPUT)/run_* | head -1); \
			SNAPSHOTS=$$(ls $$LATEST/outputs/csv/*.csv 2>/dev/null | wc -l | tr -d ' '); \
			echo "  ℹ Latest run snapshots: $$SNAPSHOTS"; \
		fi \
	fi
	@echo ""
	@echo "$(YELLOW)Step 2: Flyby$(NC)"
	@if [ -f "$(FLYBY_PLUGIN)" ]; then \
		echo "  ✓ Plugin built: $(GREEN)$(FLYBY_PLUGIN)$(NC)"; \
	else \
		echo "  ✗ Plugin not built: $(RED)$(FLYBY_PLUGIN)$(NC)"; \
	fi
	@if [ -f "$(IC_FILE)" ]; then \
		echo "  ✓ Initial conditions: $(GREEN)$(IC_FILE)$(NC)"; \
	else \
		echo "  ✗ Initial conditions not ready: $(RED)$(IC_FILE)$(NC)"; \
	fi
	@echo ""
	@echo "$(YELLOW)Animation$(NC)"
	@if [ -f "$(ANIM_STATIC)" ]; then \
		echo "  ✓ Static plot: $(GREEN)$(ANIM_STATIC)$(NC)"; \
	else \
		echo "  ✗ Static plot not created"; \
	fi
	@if [ -f "$(ANIM_OUTPUT)" ]; then \
		echo "  ✓ Animation: $(GREEN)$(ANIM_OUTPUT)$(NC)"; \
	else \
		echo "  ✗ Animation not created"; \
	fi
	@echo ""

check-deps: ## Check if required dependencies are available
	@echo "$(BLUE)Checking dependencies...$(NC)"
	@command -v cmake >/dev/null 2>&1 || { echo "$(RED)✗ cmake not found$(NC)"; exit 1; }
	@echo "$(GREEN)✓ cmake$(NC)"
	@command -v make >/dev/null 2>&1 || { echo "$(RED)✗ make not found$(NC)"; exit 1; }
	@echo "$(GREEN)✓ make$(NC)"
	@test -f "$(SPH3D)" || { echo "$(RED)✗ sph3d not found at $(SPH3D)$(NC)"; exit 1; }
	@echo "$(GREEN)✓ sph3d$(NC)"
	@command -v python3 >/dev/null 2>&1 || { echo "$(RED)✗ python3 not found$(NC)"; exit 1; }
	@echo "$(GREEN)✓ python3$(NC)"
	@python3 -c "import matplotlib" 2>/dev/null || { echo "$(YELLOW)⚠ matplotlib not found (needed for animation)$(NC)"; }
	@python3 -c "import pandas" 2>/dev/null || { echo "$(YELLOW)⚠ pandas not found (needed for animation)$(NC)"; }
	@command -v ffmpeg >/dev/null 2>&1 || { echo "$(YELLOW)⚠ ffmpeg not found (needed for video animation)$(NC)"; }
	@echo "$(GREEN)All critical dependencies satisfied!$(NC)"

##@ Building

build-relax: ## Build relaxation plugin (Step 1)
	@echo "$(BLUE)━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━$(NC)"
	@echo "$(BLUE)  Building Relaxation Plugin$(NC)"
	@echo "$(BLUE)━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━$(NC)"
	@cd $(RELAX_DIR) && ./build.sh
	@echo "$(GREEN)✓ Relaxation plugin built successfully!$(NC)"
	@echo ""

build-flyby: ## Build flyby plugin (Step 2)
	@echo "$(BLUE)━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━$(NC)"
	@echo "$(BLUE)  Building Flyby Plugin$(NC)"
	@echo "$(BLUE)━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━$(NC)"
	@cd $(FLYBY_DIR) && ./build.sh
	@echo "$(GREEN)✓ Flyby plugin built successfully!$(NC)"
	@echo ""

build-all: build-relax build-flyby ## Build all plugins

rebuild-relax: ## Clean and rebuild relaxation plugin
	@rm -rf $(RELAX_BUILD)
	@$(MAKE) build-relax

rebuild-flyby: ## Clean and rebuild flyby plugin
	@rm -rf $(FLYBY_BUILD)
	@$(MAKE) build-flyby

rebuild-all: rebuild-relax rebuild-flyby ## Clean and rebuild all plugins

##@ Running Simulations

run-relax: build-relax ## Run full relaxation simulation (Step 1)
	@echo "$(BLUE)━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━$(NC)"
	@echo "$(BLUE)  Running Relaxation Simulation (Full)$(NC)"
	@echo "$(BLUE)━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━$(NC)"
	@echo "$(YELLOW)Config: $(RELAX_CONFIG)$(NC)"
	@echo "$(YELLOW)This may take several minutes...$(NC)"
	@echo ""
	cd $(PROJECT_ROOT) && $(SPH3D) $(RELAX_PLUGIN) $(RELAX_CONFIG)
	@echo ""
	@echo "$(GREEN)✓ Relaxation simulation complete!$(NC)"
	@$(MAKE) copy-ic

run-relax-test: build-relax ## Run quick relaxation test (Step 1)
	@echo "$(BLUE)━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━$(NC)"
	@echo "$(BLUE)  Running Relaxation Simulation (Quick Test)$(NC)"
	@echo "$(BLUE)━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━$(NC)"
	@echo "$(YELLOW)Config: $(RELAX_CONFIG_TEST)$(NC)"
	@echo ""
	cd $(PROJECT_ROOT) && $(SPH3D) $(RELAX_PLUGIN) $(RELAX_CONFIG_TEST)
	@echo ""
	@echo "$(GREEN)✓ Test relaxation complete!$(NC)"
	@$(MAKE) copy-ic

copy-ic: ## Copy latest relaxation output to initial_conditions
	@echo "$(YELLOW)Copying relaxed disk to initial conditions...$(NC)"
	@mkdir -p $(IC_DIR)
	@LATEST_RUN=$$(ls -td $(RELAX_OUTPUT)/run_* 2>/dev/null | head -1); \
	if [ -n "$$LATEST_RUN" ]; then \
		FINAL_CSV=$$(ls -t $$LATEST_RUN/outputs/csv/*.csv 2>/dev/null | head -1); \
		if [ -n "$$FINAL_CSV" ]; then \
			cp "$$FINAL_CSV" $(IC_FILE); \
			echo "$(GREEN)✓ Copied: $$FINAL_CSV$(NC)"; \
			echo "$(GREEN)  → $(IC_FILE)$(NC)"; \
		else \
			echo "$(RED)✗ No CSV output found$(NC)"; \
		fi \
	else \
		echo "$(RED)✗ No relaxation runs found$(NC)"; \
	fi

run-flyby: build-flyby ## Run flyby simulation (Step 2)
	@if [ ! -f "$(IC_FILE)" ]; then \
		echo "$(RED)✗ Initial conditions not found!$(NC)"; \
		echo "$(YELLOW)Run 'make run-relax' first$(NC)"; \
		exit 1; \
	fi
	@echo "$(BLUE)━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━$(NC)"
	@echo "$(BLUE)  Running GDISPH Flyby Simulation (Step 2)$(NC)"
	@echo "$(BLUE)━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━$(NC)"
	@echo "$(YELLOW)Initial conditions: $(IC_FILE)$(NC)"
	@echo ""
	cd $(PROJECT_ROOT) && $(SPH3D) $(FLYBY_PLUGIN) $(FLYBY_CONFIG)
	@echo ""
	@echo "$(GREEN)✓ Flyby simulation complete!$(NC)"

run-all: run-relax run-flyby ## Run complete workflow (Step 1 + Step 2)
	@echo ""
	@echo "$(GREEN)━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━$(NC)"
	@echo "$(GREEN)  Complete Workflow Finished!$(NC)"
	@echo "$(GREEN)━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━$(NC)"

##@ Animation

animate: ## Create relaxation animation and plots
	@echo "$(BLUE)━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━$(NC)"
	@echo "$(BLUE)  Creating Animation$(NC)"
	@echo "$(BLUE)━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━$(NC)"
	@if [ ! -d "$(RELAX_OUTPUT)" ]; then \
		echo "$(RED)✗ No relaxation output found!$(NC)"; \
		echo "$(YELLOW)Run 'make run-relax' or 'make run-relax-test' first$(NC)"; \
		exit 1; \
	fi
	cd $(RELAX_DIR) && python3 $(ANIM_SCRIPT)
	@echo ""
	@echo "$(GREEN)✓ Animation created!$(NC)"
	@if [ -f "$(ANIM_STATIC)" ]; then echo "  $(GREEN)→ $(ANIM_STATIC)$(NC)"; fi
	@if [ -f "$(ANIM_OUTPUT)" ]; then echo "  $(GREEN)→ $(ANIM_OUTPUT)$(NC)"; fi
	@echo ""

quick-vis: ## Quick visualization of latest relaxation snapshot
	@echo "$(BLUE)Creating quick visualization...$(NC)"
	@LATEST_RUN=$$(ls -td $(RELAX_OUTPUT)/run_* 2>/dev/null | head -1); \
	if [ -n "$$LATEST_RUN" ]; then \
		FINAL_CSV=$$(ls -t $$LATEST_RUN/outputs/csv/*.csv 2>/dev/null | head -1); \
		if [ -n "$$FINAL_CSV" ]; then \
			python3 -c "import pandas as pd; import matplotlib.pyplot as plt; \
			df = pd.read_csv('$$FINAL_CSV'); \
			plt.figure(figsize=(8,8)); \
			plt.scatter(df['x'], df['y'], c=df['dens'], s=1, cmap='viridis'); \
			plt.colorbar(label='Density'); \
			plt.xlabel('x'); plt.ylabel('y'); \
			plt.title('Latest Relaxation Snapshot'); \
			plt.axis('equal'); \
			plt.savefig('$(RELAX_DIR)/quick_snapshot.png', dpi=150); \
			print('Saved to $(RELAX_DIR)/quick_snapshot.png')"; \
		fi \
	fi

##@ Workflow Shortcuts

step1: run-relax animate ## Complete Step 1: relaxation + animation

step1-test: run-relax-test animate ## Quick Step 1 test: relaxation + animation

step2: run-flyby ## Complete Step 2: flyby simulation

workflow: run-all animate ## Run complete workflow with animation

workflow-test: run-relax-test run-flyby animate ## Quick workflow test

##@ Cleaning

clean-output: ## Clean simulation outputs (keeps plugins)
	@echo "$(YELLOW)Cleaning simulation outputs...$(NC)"
	@rm -rf $(RELAX_DIR)/output/disk_relaxation/run_*
	@rm -rf $(FLYBY_DIR)/output/run_*
	@rm -f $(IC_FILE)
	@echo "$(GREEN)✓ Output cleaned$(NC)"

clean-animations: ## Clean generated animations and plots
	@echo "$(YELLOW)Cleaning animations...$(NC)"
	@rm -f $(ANIM_OUTPUT) $(ANIM_STATIC)
	@rm -f $(RELAX_DIR)/quick_snapshot.png
	@echo "$(GREEN)✓ Animations cleaned$(NC)"

clean-build: ## Clean build directories
	@echo "$(YELLOW)Cleaning build directories...$(NC)"
	@rm -rf $(RELAX_BUILD)
	@rm -rf $(FLYBY_BUILD)
	@echo "$(GREEN)✓ Build directories cleaned$(NC)"

clean: clean-output clean-animations ## Clean outputs and animations (keeps builds)

clean-all: clean clean-build ## Clean everything (outputs, animations, builds)
	@echo "$(GREEN)✓ All cleaned$(NC)"

##@ Advanced

run-relax-bg: build-relax ## Run relaxation in background with logging
	@echo "$(BLUE)Starting relaxation in background...$(NC)"
	@mkdir -p $(RELAX_DIR)/logs
	@LOG_FILE="$(RELAX_DIR)/logs/relax_$$(date +%Y%m%d_%H%M%S).log"; \
	echo "$(YELLOW)Log file: $$LOG_FILE$(NC)"; \
	cd $(PROJECT_ROOT) && nohup $(SPH3D) $(RELAX_PLUGIN) $(RELAX_CONFIG) > $$LOG_FILE 2>&1 & \
	echo $$! > $(RELAX_DIR)/.relax_pid; \
	echo "$(GREEN)✓ Process started (PID: $$(cat $(RELAX_DIR)/.relax_pid))$(NC)"; \
	echo "$(YELLOW)Monitor with: tail -f $$LOG_FILE$(NC)"

stop-relax: ## Stop background relaxation simulation
	@if [ -f "$(RELAX_DIR)/.relax_pid" ]; then \
		PID=$$(cat $(RELAX_DIR)/.relax_pid); \
		echo "$(YELLOW)Stopping process $$PID...$(NC)"; \
		kill $$PID 2>/dev/null || echo "$(YELLOW)Process already stopped$(NC)"; \
		rm -f $(RELAX_DIR)/.relax_pid; \
		echo "$(GREEN)✓ Stopped$(NC)"; \
	else \
		echo "$(YELLOW)No background process found$(NC)"; \
	fi

install-python-deps: ## Install Python dependencies for animation
	@echo "$(BLUE)Installing Python dependencies...$(NC)"
	pip3 install matplotlib pandas numpy
	@echo "$(GREEN)✓ Python dependencies installed$(NC)"
	@echo "$(YELLOW)For video animation, also install ffmpeg:$(NC)"
	@echo "  brew install ffmpeg"

.DEFAULT_GOAL := help
