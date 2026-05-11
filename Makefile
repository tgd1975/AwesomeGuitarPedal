# Makefile for AwesomeStudioPedal ESP32 project
# Uses PlatformIO as the build system

.PHONY: all help build upload clean monitor info flash \
        build-esp32 upload-esp32 uploadfs-esp32 monitor-esp32 run-esp32 \
        test-esp32-button test-esp32-multipress test-esp32-serial test-esp32-profiles \
        test-esp32-pin-io test-esp32-ble-config test-esp32-ble-pairing test-esp32-leds test-esp32-gpio2-probe \
        build-nrf52840 upload-nrf52840 uploadfs-nrf52840 monitor-nrf52840 run-nrf52840 \
        test-nrf52840-button test-nrf52840-serial test-nrf52840-profiles test-nrf52840-leds \
        test-host test-flutter clean-test \
        flutter-get flutter-format flutter-analyze flutter-test flutter-build \
        flutter-run-chrome flutter-run-emulator flutter-run-device \
        flutter-build-ios flutter-build-web \
        docs docs-coverage coverage coverage-clean \
        format lint-markdown install-hooks

# Default target: show help
all: help

## Show this help message
help:
	@awk 'BEGIN {FS = ":.*##"; printf "\033[1mAwesomeStudioPedal\033[0m\n"} \
		/^[a-zA-Z_0-9-]+:.*?##/ { printf "  \033[36m%-32s\033[0m %s\n", $$1, $$2 } \
		/^##@/ { printf "\n\033[1m%s\033[0m\n", substr($$0, 5) }' $(MAKEFILE_LIST)

# Flutter is intentionally not installed in the dev container.
# If 'flutter: command not found', either:
#   a) You are on the host machine — install Flutter from app/README.md.
#   b) You are inside the dev container — run these targets from a host terminal.
FLUTTER     ?= flutter
EMULATOR_ID ?= emulator-5554

ESP32_ENV    ?= nodemcu-32s
NRF52840_ENV ?= feather-nrf52840
PORT         ?= /dev/ttyUSB0

# Use separate build dirs for host and dev container to avoid CMake cache
# conflicts caused by differing absolute paths (/home/... vs /workspaces/...).
ifeq ($(shell test -f /.dockerenv && echo yes),yes)
  HOST_BUILD_DIR = .vscode/build
else
  HOST_BUILD_DIR = .vscode/build-host
endif

COVERAGE_BUILD_DIR = .vscode/build-coverage
COVERAGE_INFO      = $(COVERAGE_BUILD_DIR)/coverage.info
COVERAGE_REPORT    = docs/coverage

##@ General

build: build-esp32 build-nrf52840 ## Build ALL targets (ESP32 + nRF52840)

upload: upload-esp32 ## Upload firmware to ESP32 (default target)

clean: ## Clean all build artifacts
	pio run --target clean

monitor: ## Open serial monitor (default: ESP32)
	pio device monitor

info: ## Show PlatformIO project info and available targets
	pio run --list-targets

flash: upload

##@ Firmware — ESP32

build-esp32: ## Build firmware for ESP32
	pio run -e $(ESP32_ENV)

upload-esp32: ## Upload firmware to ESP32
	pio run -e $(ESP32_ENV) --target upload

uploadfs-esp32: ## Upload filesystem image (data/) to ESP32
	pio run -e $(ESP32_ENV) --target uploadfs

monitor-esp32: ## Open serial monitor for ESP32
	pio device monitor -e $(ESP32_ENV)

run-esp32: build-esp32 upload-esp32 uploadfs-esp32 ## Build, upload firmware + filesystem, then monitor ESP32
	@echo "Build and upload complete. Starting monitor..."
	pio device monitor -e $(ESP32_ENV) || echo "Monitor failed, but build/upload succeeded"

##@ Firmware — nRF52840

build-nrf52840: ## Build firmware for nRF52840
	pio run -e $(NRF52840_ENV)

upload-nrf52840: ## Upload firmware to nRF52840
	pio run -e $(NRF52840_ENV) --target upload

uploadfs-nrf52840: ## Upload filesystem image (data/) to nRF52840
	pio run -e $(NRF52840_ENV) --target uploadfs

monitor-nrf52840: ## Open serial monitor for nRF52840
	pio device monitor -e $(NRF52840_ENV)

run-nrf52840: build-nrf52840 upload-nrf52840 uploadfs-nrf52840 ## Build, upload firmware + filesystem, then monitor nRF52840
	@echo "Build and upload complete. Starting monitor..."
	pio device monitor -e $(NRF52840_ENV) || echo "Monitor failed, but build/upload succeeded"

##@ Tests — Host

test-host: ## Run host unit tests (GoogleTest via CMake)
	cmake --build $(HOST_BUILD_DIR) --target pedal_tests && $(HOST_BUILD_DIR)/test/pedal_tests

coverage: ## Run host tests and generate HTML coverage report (requires lcov)
	cmake -B $(COVERAGE_BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug \
		-DCMAKE_CXX_FLAGS="--coverage" \
		-DCMAKE_EXE_LINKER_FLAGS="--coverage"
	cmake --build $(COVERAGE_BUILD_DIR) --target pedal_tests
	$(COVERAGE_BUILD_DIR)/test/pedal_tests
	lcov --capture --directory $(COVERAGE_BUILD_DIR) \
		--output-file $(COVERAGE_INFO) \
		--ignore-errors inconsistent,inconsistent,mismatch
	lcov --extract $(COVERAGE_INFO) \
		'*/lib/PedalLogic/*' \
		'*/lib/hardware/*' \
		--output-file $(COVERAGE_INFO) \
		--ignore-errors unused,inconsistent,inconsistent
	genhtml $(COVERAGE_INFO) --output-directory $(COVERAGE_REPORT)
	@echo "Coverage report: $(COVERAGE_REPORT)/index.html"

coverage-clean: ## Remove coverage build artifacts
	rm -rf $(COVERAGE_BUILD_DIR)

clean-test: ## Remove on-device test build artifacts
	rm -rf .pio/build/test

##@ Tests — ESP32 (device required)

test-esp32-button: uploadfs-esp32 ## Run on-device button tests (Unity, requires ESP32)
	pio test -e nodemcu-32s-test -v

test-esp32-multipress: uploadfs-esp32 ## Run on-device long-press / double-press tests (Unity, requires ESP32)
	pio test -e nodemcu-32s-multipress-test -v

test-esp32-serial: uploadfs-esp32 ## Run on-device serial output tests (Unity, requires ESP32)
	pio test -e nodemcu-32s-serial-test -v

test-esp32-profiles: uploadfs-esp32 ## Run on-device profile manager tests (Unity, requires ESP32)
	pio test -e nodemcu-32s-profilemanager-test -v

test-esp32-pin-io: uploadfs-esp32 ## Run on-device GPIO testrig for PinAction (Unity, requires ESP32)
	pio test -e nodemcu-32s-pin-io-test -v

test-esp32-ble-config: ## Run BLE config end-to-end tests (requires ESP32 + BLE host; set PORT=)
	pio run -e nodemcu-32s-ble-config-test --target upload
	PLATFORMIO_DATA_DIR=test/test_ble_config_esp32/data \
	    pio run -e nodemcu-32s-ble-config-test --target uploadfs
	python3 test/test_ble_config_esp32/runner.py --port $(PORT)

test-esp32-ble-pairing: ## Run BLE pairing-PIN smoke test (Linux/BlueZ; requires ESP32; set PORT=)
	python3 test/test_ble_pairing_esp32/runner.py --port $(PORT)

test-esp32-leds: ## Flash LED hardware diagnostic + open monitor (requires ESP32)
	pio run -e nodemcu-32s-leds-test --target upload
	pio device monitor -e nodemcu-32s-leds-test

test-esp32-gpio2-probe: ## Flash GPIO 2 toggle probe + open monitor (does this board have an onboard LED on GPIO 2?)
	pio run -e nodemcu-32s-gpio2-probe --target upload
	pio device monitor -e nodemcu-32s-gpio2-probe

##@ Tests — nRF52840 (device required)

test-nrf52840-button: uploadfs-nrf52840 ## Run on-device button tests (Unity, requires nRF52840)
	pio test -e feather-nrf52840-test -v

test-nrf52840-serial: uploadfs-nrf52840 ## Run on-device serial output tests (Unity, requires nRF52840)
	pio test -e feather-nrf52840-serial-test -v

test-nrf52840-profiles: uploadfs-nrf52840 ## Run on-device profile manager tests (Unity, requires nRF52840)
	pio test -e feather-nrf52840-profilemanager-test -v

test-nrf52840-leds: ## Flash LED hardware diagnostic + open monitor (requires nRF52840)
	pio run -e feather-nrf52840-leds-test --target upload
	pio device monitor -e feather-nrf52840-leds-test

##@ Flutter App  (flutter on PATH required — not available inside dev container)

flutter-get: ## Install Flutter dependencies (pub get)
	@command -v $(FLUTTER) >/dev/null 2>&1 || { \
		echo "ERROR: flutter not found on PATH."; \
		echo "  Inside dev container: run this from a host terminal (Flutter is not installed in the container by design)."; \
		echo "  On the host machine: install Flutter — see app/README.md for instructions."; \
		exit 1; }
	cd app && $(FLUTTER) pub get

flutter-format: flutter-get ## Auto-format all Dart files in app/
	cd app && dart format .

flutter-analyze: flutter-get ## Run flutter analyze
	cd app && $(FLUTTER) analyze

flutter-test: flutter-get ## Run Flutter unit and widget tests
	cd app && $(FLUTTER) test

flutter-build: flutter-get ## Build release APK (Android)
	cd app && $(FLUTTER) build apk --release

flutter-run-chrome: flutter-get ## Run app in Chromium — web mode, no BLE (use for UI development)
	cd app && $(FLUTTER) run -d chrome

flutter-run-emulator: flutter-get ## Run app on Android emulator (override: EMULATOR_ID=emulator-5554)
	cd app && $(FLUTTER) run -d $(EMULATOR_ID)

flutter-run-device: flutter-get ## Run app on the first connected physical device
	cd app && $(FLUTTER) run

flutter-build-ios: flutter-get ## Build release iOS app (requires macOS + Xcode)
	cd app && $(FLUTTER) build ios --release

flutter-build-web: flutter-get ## Build release web app
	cd app && $(FLUTTER) build web --release

# CI and pre-commit alias — not shown in help
test-flutter: flutter-test

##@ Documentation

docs: ## Generate API documentation with Doxygen
	doxygen Doxyfile
	@echo "Documentation generated in docs/api/"

docs-coverage: ## Show undocumented symbols in project source files
	@echo "Undocumented symbols in project source:"
	@doxygen Doxyfile 2>&1 | \
		grep "warning:.*is not documented" | \
		grep -v "\.pio\|/usr/\|ArduinoJson\|Polyfill\|Variant\|_deps" | \
		sort || true
	@echo "Done."

##@ Code Quality

format: ## Format all C++ files with clang-format
	find . -name "*.cpp" -o -name "*.h" -o -name "*.hpp" | grep -v \.pio | grep -v build | xargs clang-format -i
	@echo "All C++ files formatted successfully"

lint-markdown: ## Fix markdown linting issues (requires markdownlint-cli2)
	npx markdownlint-cli2 --fix "**/*.md" "#node_modules" "#.pio" "#build" "#.vscode" "#.claude/security-review-latest.md"
	@echo "Markdown linting complete"

##@ Maintenance

install-hooks: ## Install pre-commit hooks from scripts/
	cp scripts/pre-commit .git/hooks/pre-commit
	chmod +x .git/hooks/pre-commit
