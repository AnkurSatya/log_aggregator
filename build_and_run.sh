#!/bin/bash
#
# # --- VENV DETECTION ---
VENV_PATH="./.venv/bin/activate" # Adjust this to your venv folder name

if [ -f "$VENV_PATH" ]; then
    echo "--- Activating Python Venv ---"
    source "$VENV_PATH"
fi

# Now 'conan' is available in the PATH if the venv was activated
if ! command -v conan &> /dev/null; then
    echo "Error: Conan not found. Please install it in the venv."
    exit 1
fi
# ----------------------

# Configuration
BUILD_TYPE=${1:-Debug} # Defaults to Debug if no arg provided
BUILD_DIR="build-${BUILD_TYPE,,}"
CONAN_FILE="conanfile.txt"
PROFILE_FILE="$HOME/.conan2/profiles/default"
HASH_FILE="$BUILD_DIR/.conan_hash"

mkdir -p "$BUILD_DIR"

# 1. Generate a combined hash of your requirements and your profile
CURRENT_HASH=$(cat "$CONAN_FILE" "$PROFILE_FILE" | md5sum | awk '{print $1}')

# 2. Check if we need to run Conan
if [ ! -f "$HASH_FILE" ] || [ "$CURRENT_HASH" != "$(cat "$HASH_FILE")" ] || [ ! -f "$BUILD_DIR/conan_toolchain.cmake" ]; then
    echo "--- Config change detected. Running Conan Install ($BUILD_TYPE)... ---"

    if conan install . -s build_type="$BUILD_TYPE" --build=missing --output-folder="$BUILD_DIR"; then
        echo "$CURRENT_HASH" > "$HASH_FILE"
    else
        echo "Conan install failed!"
        exit 1
    fi
else
    echo "--- Conan dependencies are up to date. Skipping install. ---"
fi

# 3. Proceed to CMake
echo "--- Configuring CMake ---"
cmake -S . -B "$BUILD_DIR" \
    -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE"\
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# Create a symlink in the project root pointing to the debug compile commands
ln -sf "$ZED_WORKTREE_ROOT/$BUILD_DIR/compile_commands.json" "$ZED_WORKTREE_ROOT/compile_commands.json"

echo "--- Building ---"
cmake --build "$BUILD_DIR" -j$(nproc)

if [ $? -eq 0 ]; then
    echo "--- Build Successful! Starting App... ---"
    sudo "${BUILD_DIR}/log_aggregator"
else
    echo "--- Build Failed. ---"
    exit 1
fi
