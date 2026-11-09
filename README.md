# Image Pipeline for AR Overlays

This project builds an image-processing pipeline whose output can be used as an overlay in an augmented reality application. Each video frame goes through a Gaussian blur, a Sobel edge filter and a threshold, which produces a black-and-white mask of object edges.

The goal is to implement the same pipeline in sequential, OpenMP, MPI and CUDA engines that return byte-identical results, and then measure whether fusing pipeline stages and limiting data transfers reduces latency.

## Setup

You need a C++17 compiler, CMake 3.24 or newer, Ninja, pkg-config, OpenMP and the FFmpeg development libraries.

On macOS, install the Xcode Command Line Tools (`xcode-select --install`) if you don't have them yet, then install the rest with Homebrew:

```sh
brew install cmake ninja pkg-config libomp ffmpeg
```

Clone the repository and configure the build. The first configuration downloads GoogleTest, so it needs internet access.

```sh
git clone https://github.com/JakubFr4czek/prir-projekt.git
cd prir-projekt
cmake -S . -B build -G Ninja -DOpenMP_ROOT="$(brew --prefix libomp)"
```

## Commands

Run these from the repository root after configuring:

```sh
# Build, recompiling only what changed
cmake --build build

# Rebuild everything from scratch
cmake --build build --clean-first

# Remove compiled files
cmake --build build --target clean

# Run
./build/pipeline
```

`pipeline` is currently an empty program that exits immediately without output. Image processing is added in later steps of the plan.

To reset the configuration as well, delete the build directory with `rm -rf build` and repeat the configure command from [Setup](#setup).
