#==============================================================================
# Makefile for Managing Phoronix Test Suite Projects
# ==============================================================================

# --- User Configuration ---
# The default test suite to package or run.
# Can be overridden from the command line, e.g., `make run-suite SUITE=my-other-suite`
SUITE ?= mte-server

# --- System and Project Paths ---
PTS_DIR := /var/lib/phoronix-test-suite

PROJECT_DIR := $(CURDIR)
SUITE_FILE=$(PTS_DIR)/test-suites/local/$(SUITE)/suite-definition.xml

# --- PHONY Targets ---
# These are actions, not files.
.PHONY: help package apply run-suite run-test install-test

# ==============================================================================
# Main Targets
# ==============================================================================

## Prints this help message.
help:
	@echo "Usage: make <target> [OPTIONS...]"
	@echo ""
	@echo "Project Management Targets:"
	@echo "  package       Extracts the '$(SUITE)' suite and its local test dependencies from the system into this project folder."
	@echo "  apply         Applies all test profiles and suites from this project folder to the system's Phoronix path."
	@echo ""
	@echo "Execution Targets:"
	@echo "  run-suite     Runs the test suite defined by the SUITE variable (default: '$(SUITE)')."
	@echo "                Example: make run-suite SUITE=another-suite"
	@echo "  run-test      Runs an individual test profile."
	@echo "                Example: make run-test TEST=local/my-rocksdb-mod"
	@echo "  install-test  Installs dependencies for an individual test profile."
	@echo "                Example: make install-test TEST=local/my-nginx-mod"


print-state:
	@echo "'$(SUITE)' and its dependencies from $(PTS_DIR)..."

## Package the suite and its local dependencies from the system into the project folder.
package:
	@echo "Packaging suite '$(SUITE)' and its dependencies from $(PTS_DIR)..."
	@if [ ! -f "$(SUITE_FILE)" ]; then \
		echo "Error: Suite file not found at $(SUITE_FILE)"; \
		exit 1; \
	fi
	# Clean and create directory structure
	@mkdir -p $(PROJECT_DIR)/test-profiles/local
	@mkdir -p $(PROJECT_DIR)/test-suites/local
	# Copy the suite file
	@cp "$(SUITE_FILE)" $(PROJECT_DIR)/test-suites/local/$(SUITE)/
	# Find and copy dependencies
	@echo "Finding local test profile dependencies..."
	@for profile in $$(xmllint --xpath "//Entry/Identifier/text()" $(SUITE_FILE) | xargs -n1 | sort -u | grep 'local/'); do \
		echo "  -> Packaging $$profile"; \
		profile_name=$$(basename $$profile); \
		source_path=$(PTS_DIR)/test-profiles/$$profile; \
		dest_path=$(PROJECT_DIR)/test-profiles/local/$${profile_name}; \
		if [ -d "$$source_path" ]; then \
			rm -rf $$dest_path; \
			cp -r $$source_path $$dest_path; \
		else \
			echo "Warning: Profile '$$profile' not found, skipping."; \
		fi \
	done
	@echo "Packaging complete."


## Apply all test profiles and suites from this project to the system.
apply:
	@echo "Applying project files to $(PTS_DIR)..."
	@rsync -av --mkpath $(PROJECT_DIR)/test-profiles/ $(PTS_DIR)/test-profiles/
	@rsync -av --mkpath $(PROJECT_DIR)/test-suites/ $(PTS_DIR)/test-suites/
	@echo "Apply complete. Custom tests and suites are now available to Phoronix."


## Run a complete test suite.
run-suite:
	@echo "Running test suite: $(SUITE)..."
	@phoronix-test-suite batch-run $(SUITE)

## Run a complete test suite.
install-suite:
	@echo "Running test suite: $(SUITE)..."
	@phoronix-test-suite install $(SUITE)


## Run an individual test profile.
run-test:
	@if [ -z "$(TEST)" ]; then \
		echo "Error: Please specify a test. Usage: make run-test TEST=local/my-test-name"; \
		exit 1; \
	fi
	@echo "Running individual test: $(TEST)..."
	@phoronix-test-suite batch-run $(TEST)


## Install dependencies for an individual test profile.
install-test:
	@if [ -z "$(TEST)" ]; then \
		echo "Error: Please specify a test. Usage: make install-test TEST=local/my-test-name"; \
		exit 1; \
	fi
	@echo "Installing dependencies for test: $(TEST)..."
	@phoronix-test-suite install $(TEST)
