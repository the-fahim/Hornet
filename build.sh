# Configure the build directory with Ninja
cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Release

# Build the project
cmake --build build

# Run the compiled executable
./build/output