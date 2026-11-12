cmake_minimum_required(VERSION 3.24)
project(prir-projekt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)   # performance is measured only in optimized builds
endif()

option(PIPELINE_WITH_OPENMP "OpenMP engine" ON)
option(PIPELINE_WITH_MPI    "MPI engine"    OFF)
option(PIPELINE_WITH_CUDA   "CUDA engine"   OFF)
option(PIPELINE_WITH_VIDEO  "Video support via the FFmpeg libraries" ON)

# Source files are added to the targets below as later steps of the plan create them.
# The MPI and CUDA blocks already name their sources, so those options stay OFF until the files exist.
add_library(pipeline_core STATIC
  src/common/pgm.cpp
  src/common/frame_source.cpp)
target_include_directories(pipeline_core PUBLIC include)

if(PIPELINE_WITH_VIDEO)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET libavformat libavcodec libswscale libavutil)
  target_sources(pipeline_core PRIVATE src/common/video.cpp)
  target_compile_definitions(pipeline_core PUBLIC PIPELINE_HAVE_VIDEO)
  target_link_libraries(pipeline_core PUBLIC PkgConfig::FFMPEG)
endif()

add_executable(pipeline src/common/main.cpp)
target_link_libraries(pipeline PRIVATE pipeline_core)

if(PIPELINE_WITH_OPENMP)
  find_package(OpenMP REQUIRED)
  target_compile_definitions(pipeline PRIVATE PIPELINE_HAVE_OPENMP)
  target_link_libraries(pipeline PRIVATE OpenMP::OpenMP_CXX)
endif()

if(PIPELINE_WITH_MPI)
  find_package(MPI REQUIRED COMPONENTS CXX)
  add_executable(pipeline_mpi src/mpi/main_mpi.cpp)
  target_link_libraries(pipeline_mpi PRIVATE pipeline_core MPI::MPI_CXX)
endif()

if(PIPELINE_WITH_CUDA)
  enable_language(CUDA)
  set(CMAKE_CUDA_STANDARD 17)
  set(PIPELINE_CUDA_ARCH native CACHE STRING "GPU architecture, e.g. 90")
  add_executable(pipeline_cuda src/cuda/main_cuda.cu)
  set_target_properties(pipeline_cuda PROPERTIES CUDA_ARCHITECTURES ${PIPELINE_CUDA_ARCH})
  target_link_libraries(pipeline_cuda PRIVATE pipeline_core)
endif()

include(CTest)
if(BUILD_TESTING)
  include(FetchContent)
  FetchContent_Declare(googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.18.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
  FetchContent_MakeAvailable(googletest)
  include(GoogleTest)

  add_executable(pipeline_tests
    tests/test_pgm.cpp
    tests/test_weights.cpp)
  target_link_libraries(pipeline_tests PRIVATE pipeline_core GTest::gtest_main)
  if(PIPELINE_WITH_OPENMP)
    target_compile_definitions(pipeline_tests PRIVATE PIPELINE_HAVE_OPENMP)
    target_link_libraries(pipeline_tests PRIVATE OpenMP::OpenMP_CXX)
  endif()
  gtest_discover_tests(pipeline_tests)

  if(PIPELINE_WITH_MPI)
    add_executable(test_mpi tests/test_mpi.cpp)
    target_link_libraries(test_mpi PRIVATE pipeline_core MPI::MPI_CXX GTest::gtest)
    add_test(NAME mpi_4_procs
             COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 4 $<TARGET_FILE:test_mpi>)
  endif()

  if(PIPELINE_WITH_CUDA)
    add_executable(test_cuda tests/test_cuda.cpp src/cuda/engine_cuda.cu)
    target_include_directories(test_cuda PRIVATE src/cuda)
    target_link_libraries(test_cuda PRIVATE pipeline_core GTest::gtest_main)
    set_target_properties(test_cuda PROPERTIES CUDA_ARCHITECTURES ${PIPELINE_CUDA_ARCH})
    gtest_discover_tests(test_cuda)
  endif()
endif()
