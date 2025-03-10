# Copyright (c) 2020, ArrayFire
# All rights reserved.
#
# This file is distributed under 3-clause BSD license.
# The complete license agreement can be obtained at:
# http://arrayfire.com/licenses/BSD-3-Clause

set(AF_TEST_WITH_MTX_FILES
    ON CACHE BOOL
    "Download and run tests on large matrices form sparse.tamu.edu")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules")

if(AF_TEST_WITH_MTX_FILES)
  include(download_sparse_datasets)
endif()

if(AF_WITH_EXTERNAL_PACKAGES_ONLY)
    dependency_check(GTest_FOUND)
else()
  af_dep_check_and_populate(${gtest_prefix}
    URI https://github.com/google/googletest.git
    REF release-1.8.1
  )

  # gtest targets cmake version 2.6 which throws warnings for policy CMP0042 on
  # newer cmakes. This sets the default global setting for that policy.
  set(CMAKE_POLICY_DEFAULT_CMP0042 NEW)
  if(WIN32)
    set(gtest_force_shared_crt ON
        CACHE INTERNAL "Required so that the libs Runtime is not set to MT DLL")
    set(BUILD_SHARED_LIBS OFF)
  endif()

  add_subdirectory(${${gtest_prefix}_SOURCE_DIR} ${${gtest_prefix}_BINARY_DIR} EXCLUDE_FROM_ALL)
  set_target_properties(gtest gtest_main
    PROPERTIES
      FOLDER "ExternalProjectTargets/gtest")
  add_library(GTest::gtest ALIAS gtest)
  if(UNIX)
    if("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU" AND
      CMAKE_CXX_COMPILER_VERSION VERSION_GREATER "10.3.0")
      target_compile_options(gtest PRIVATE -Wno-maybe-uninitialized)
      target_compile_options(gtest_main PRIVATE -Wno-maybe-uninitialized)
    endif()
  endif()

  # Hide gtest project variables
  mark_as_advanced(
    BUILD_SHARED_LIBS
    BUILD_GMOCK
    INSTALL_GTEST
    gmock_build_tests
    gtest_build_samples
    gtest_build_tests
    gtest_disable_pthreads
    gtest_force_shared_crt
    gtest_hide_internal_symbols
  )
endif()

if(NOT TARGET mmio)
  add_subdirectory(mmio)
endif()

# Reset the CXX flags for tests
set(CMAKE_CXX_STANDARD 11)

# TODO(pradeep) perhaps rename AF_USE_RELATIVE_TEST_DIR to AF_WITH_TEST_DATA_DIR
#               with empty default value
if(${AF_USE_RELATIVE_TEST_DIR})
  # RELATIVE_TEST_DATA_DIR is a User-visible option with default value of test/data directory
  # This code arm assumes user is responsible for providing the test data path
  set(RELATIVE_TEST_DATA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/data" CACHE
      STRING "Relative Test Data Directory")
  set(TESTDATA_SOURCE_DIR ${RELATIVE_TEST_DATA_DIR})
else(${AF_USE_RELATIVE_TEST_DIR})
  af_dep_check_and_populate(${testdata_prefix}
    URI https://github.com/arrayfire/arrayfire-data.git
    #pinv large data set update change
    REF 0144a599f913cc67c76c9227031b4100156abc25
  )
  set(TESTDATA_SOURCE_DIR "${${testdata_prefix}_SOURCE_DIR}")
endif(${AF_USE_RELATIVE_TEST_DIR})

if(AF_BUILD_CPU)
  list(APPEND enabled_backends "cpu")
endif(AF_BUILD_CPU)

if(AF_BUILD_CUDA)
  list(APPEND enabled_backends "cuda")
endif(AF_BUILD_CUDA)

if(AF_BUILD_OPENCL)
  list(APPEND enabled_backends "opencl")
endif(AF_BUILD_OPENCL)

if(AF_BUILD_UNIFIED)
  list(APPEND enabled_backends "unified")
endif(AF_BUILD_UNIFIED)


add_library(arrayfire_test OBJECT
  testHelpers.hpp
  arrayfire_test.cpp)

target_include_directories(arrayfire_test
  PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${ArrayFire_SOURCE_DIR}/include
    ${ArrayFire_BINARY_DIR}/include
    ${ArrayFire_SOURCE_DIR}/extern/half/include
    mmio
    $<TARGET_PROPERTY:Boost::boost,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:GTest::gtest,INTERFACE_INCLUDE_DIRECTORIES>)

if(WIN32)
  target_compile_options(arrayfire_test
    PRIVATE
      /bigobj
      /EHsc)
  target_compile_definitions(arrayfire_test
    PRIVATE
      WIN32_LEAN_AND_MEAN
      NOMINMAX)
endif()

target_compile_definitions(arrayfire_test
  PRIVATE
    TEST_RESULT_IMAGE_DIR="${CMAKE_BINARY_DIR}/test/"
    USE_MTX)

# Creates tests for all backends
#
# Creates a standard test for all backends. Most of the time you only need to
# specify the name of the source file to create a test.
#
# Parameters
# ----------
# 'CXX11'       If set the tests will be compiled using c++11. Tests should strive
#               to be C++98 compliant
# 'SRC'         The source files for the test
# 'LIBRARIES'   Libraries other than ArrayFire that need to be linked
# 'DEFINITIONS' Definitions that need to be defined
# 'BACKENDS'    Backends to target for this test. If not set then the test will
#               compiled againat all backends
function(make_test)
  set(options CXX11 SERIAL USE_MMIO NO_ARRAYFIRE_TEST)
  set(single_args SRC)
  set(multi_args LIBRARIES DEFINITIONS BACKENDS)
  cmake_parse_arguments(mt_args "${options}" "${single_args}" "${multi_args}" ${ARGN})

  get_filename_component(src_name ${mt_args_SRC} NAME_WE)
  foreach(backend ${enabled_backends})
    if(NOT "${mt_args_BACKENDS}" STREQUAL "" AND
       NOT ${backend} IN_LIST mt_args_BACKENDS)
      continue()
    endif()
    set(target "test_${src_name}_${backend}")

    if (${mt_args_NO_ARRAYFIRE_TEST})
      add_executable(${target} ${mt_args_SRC})
    else()
      add_executable(${target} ${mt_args_SRC} $<TARGET_OBJECTS:arrayfire_test>)
    endif()
    target_include_directories(${target}
      PRIVATE
        ${ArrayFire_SOURCE_DIR}/extern/half/include
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
      )
    target_link_libraries(${target}
      PRIVATE
        ${mt_args_LIBRARIES}
	      GTest::gtest
      )

    if(${backend} STREQUAL "unified")
      target_link_libraries(${target}
        PRIVATE
          af)
    else()
      target_link_libraries(${target}
        PRIVATE
          af${backend}
          )
    endif()

    if(${mt_args_CXX11})
      set_target_properties(${target}
        PROPERTIES
          CXX_STANDARD 11)
    endif(${mt_args_CXX11})

    set_target_properties(${target}
      PROPERTIES
        FOLDER "Tests"
        OUTPUT_NAME "${src_name}_${backend}")

    target_compile_definitions(${target}
      PRIVATE
        TEST_DIR="${TESTDATA_SOURCE_DIR}"
        AF_$<UPPER_CASE:${backend}>
        ${mt_args_DEFINITIONS}
      )
    target_link_libraries(${target} PRIVATE mmio)
    if(AF_TEST_WITH_MTX_FILES AND ${mt_args_USE_MMIO})
      target_compile_definitions(${target}
        PRIVATE
        MTX_TEST_DIR="${ArrayFire_BINARY_DIR}/extern/matrixmarket/"
        )
    endif()
    if(WIN32)
      target_compile_options(${target}
        PRIVATE
          /bigobj
          /EHsc)
      target_compile_definitions(${target}
        PRIVATE
          WIN32_LEAN_AND_MEAN
          NOMINMAX)
    endif()

    # TODO(umar): Create this executable separately
    if(NOT ${backend} STREQUAL "unified" OR ${target} STREQUAL "backend_unified")
      add_test(NAME ${target} COMMAND ${target})
	    if(${mt_args_SERIAL})
        set_tests_properties(${target}
          PROPERTIES
            RUN_SERIAL ON)
	    endif(${mt_args_SERIAL})
    endif()

  endforeach()
endfunction(make_test)

make_test(SRC anisotropic_diffusion.cpp)
make_test(SRC approx1.cpp)
make_test(SRC approx2.cpp)
make_test(SRC array.cpp CXX11)
make_test(SRC array_death_tests.cpp CXX11 SERIAL)
make_test(SRC arrayio.cpp)
make_test(SRC assign.cpp CXX11)
make_test(SRC backend.cpp CXX11)
make_test(SRC basic.cpp)
make_test(SRC bilateral.cpp)
make_test(SRC binary.cpp CXX11)
make_test(SRC blas.cpp)
make_test(SRC canny.cpp)
make_test(SRC cast.cpp)
make_test(SRC cholesky_dense.cpp SERIAL)
make_test(SRC clamp.cpp)
make_test(SRC compare.cpp)
make_test(SRC complex.cpp)
make_test(SRC confidence_connected.cpp CXX11)
make_test(SRC constant.cpp)
make_test(SRC convolve.cpp CXX11)
make_test(SRC corrcoef.cpp)
make_test(SRC covariance.cpp)
make_test(SRC diagonal.cpp)
make_test(SRC diff1.cpp)
make_test(SRC diff2.cpp)
make_test(SRC dog.cpp)
make_test(SRC dot.cpp)
make_test(SRC empty.cpp)
make_test(SRC event.cpp CXX11)
make_test(SRC fast.cpp)
make_test(SRC fft.cpp)
make_test(SRC fft_large.cpp)
make_test(SRC fft_real.cpp)
make_test(SRC fftconvolve.cpp)
make_test(SRC flat.cpp)
make_test(SRC flip.cpp)
make_test(SRC gaussiankernel.cpp)
make_test(SRC gen_assign.cpp)
make_test(SRC gen_index.cpp CXX11)
make_test(SRC getting_started.cpp)
make_test(SRC gfor.cpp)
make_test(SRC gradient.cpp)
make_test(SRC gray_rgb.cpp)
make_test(SRC half.cpp)
make_test(SRC hamming.cpp)
make_test(SRC harris.cpp)
make_test(SRC histogram.cpp)
make_test(SRC homography.cpp)
make_test(SRC hsv_rgb.cpp)
make_test(SRC iir.cpp)
make_test(SRC imageio.cpp)
make_test(SRC index.cpp CXX11)
make_test(SRC info.cpp)
make_test(SRC internal.cpp)
make_test(SRC inverse_deconv.cpp)
make_test(SRC inverse_dense.cpp SERIAL)
make_test(SRC iota.cpp)
make_test(SRC ireduce.cpp)
make_test(SRC iterative_deconv.cpp)
make_test(SRC jit.cpp CXX11)
make_test(SRC join.cpp)
make_test(SRC lu_dense.cpp SERIAL)
#make_test(manual_memory_test.cpp)
make_test(SRC match_template.cpp)
make_test(SRC math.cpp CXX11)
make_test(SRC matrix_manipulation.cpp)
make_test(SRC mean.cpp)
make_test(SRC meanshift.cpp)
make_test(SRC meanvar.cpp CXX11)
make_test(SRC medfilt.cpp)
make_test(SRC median.cpp)
make_test(SRC memory.cpp CXX11)
make_test(SRC memory_lock.cpp)
make_test(SRC missing.cpp)
make_test(SRC moddims.cpp)
make_test(SRC moments.cpp)
make_test(SRC morph.cpp)
make_test(SRC nearest_neighbour.cpp CXX11)
make_test(SRC nodevice.cpp CXX11)

if(OpenCL_FOUND)
  make_test(SRC ocl_ext_context.cpp
            LIBRARIES OpenCL::OpenCL OpenCL::cl2hpp
            BACKENDS "opencl"
            CXX11)
  make_test(SRC interop_opencl_custom_kernel_snippet.cpp
            LIBRARIES OpenCL::OpenCL
            BACKENDS "opencl"
            NO_ARRAYFIRE_TEST
            CXX11)
  make_test(SRC interop_opencl_external_context_snippet.cpp
            LIBRARIES OpenCL::OpenCL OpenCL::cl2hpp
            BACKENDS "opencl"
            NO_ARRAYFIRE_TEST
            CXX11)
endif()

if(CUDA_FOUND)
  foreach(backend ${enabled_backends})
    set(cuda_test_backends "cuda" "unified")
    if(${backend} IN_LIST cuda_test_backends)
      set(target test_cuda_${backend})
      if(${CMAKE_VERSION} VERSION_LESS 3.5.2)
        cuda_include_directories(
          ${ArrayFire_SOURCE_DIR}/include
          ${ArrayFire_BINARY_DIR}/include
          ${ArrayFire_SOURCE_DIR}/extern/half/include
          ${CMAKE_CURRENT_SOURCE_DIR}
        )
      endif()
      cuda_add_executable(${target} cuda.cu  $<TARGET_OBJECTS:arrayfire_test>)
      target_include_directories(${target} PRIVATE
        ${ArrayFire_SOURCE_DIR}/extern/half/include
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR})
      if(${backend} STREQUAL "unified")
        target_link_libraries(${target}
          ArrayFire::af)
      else()
        target_link_libraries(${target}
          ArrayFire::af${backend})
      endif()
      target_link_libraries(${target}
        mmio
        GTest::gtest)

      # Couldn't get Threads::Threads to work with this cuda binary. The import
      # target would not add the -pthread flag which is required for this
      # executable (on Ubuntu 18.04 anyway)
      check_cxx_compiler_flag(-pthread pthread_flag)
      if(pthread_flag)
        target_link_libraries(${target} -pthread)
      endif()

      set_target_properties(${target}
        PROPERTIES
        FOLDER "Tests"
        OUTPUT_NAME "cuda_${backend}")

      if(NOT ${backend} STREQUAL "unified")
        add_test(NAME ${target} COMMAND ${target})
      endif()
    endif()
  endforeach()
endif()


make_test(SRC orb.cpp)
make_test(SRC pad_borders.cpp CXX11)
make_test(SRC pinverse.cpp SERIAL)
make_test(SRC qr_dense.cpp SERIAL)
make_test(SRC random.cpp)
make_test(SRC rng_quality.cpp BACKENDS "cuda;opencl" SERIAL)
make_test(SRC range.cpp)
make_test(SRC rank_dense.cpp SERIAL)
make_test(SRC reduce.cpp CXX11)
make_test(SRC regions.cpp)
make_test(SRC reorder.cpp)
make_test(SRC replace.cpp CXX11)
make_test(SRC resize.cpp)
make_test(SRC rng_match.cpp CXX11 BACKENDS "unified")
make_test(SRC rotate.cpp)
make_test(SRC rotate_linear.cpp)
make_test(SRC sat.cpp)
make_test(SRC scan.cpp)
make_test(SRC scan_by_key.cpp)
make_test(SRC select.cpp CXX11)
make_test(SRC set.cpp CXX11)
make_test(SRC shift.cpp)
make_test(SRC gloh.cpp)
make_test(SRC sift.cpp)
make_test(SRC sobel.cpp)
make_test(SRC solve_dense.cpp       CXX11 SERIAL)
make_test(SRC sort.cpp)
make_test(SRC sort_by_key.cpp)
make_test(SRC sort_index.cpp)
make_test(SRC sparse.cpp SERIAL)
make_test(SRC sparse_arith.cpp      USE_MMIO)
make_test(SRC sparse_convert.cpp)
make_test(SRC stdev.cpp)
make_test(SRC susan.cpp)
make_test(SRC svd_dense.cpp         SERIAL)
make_test(SRC threading.cpp         CXX11 SERIAL)
make_test(SRC tile.cpp)
make_test(SRC topk.cpp              CXX11)
make_test(SRC transform.cpp)
make_test(SRC transform_coordinates.cpp)
make_test(SRC translate.cpp)
make_test(SRC transpose.cpp)
make_test(SRC transpose_inplace.cpp)
make_test(SRC triangle.cpp)
make_test(SRC unwrap.cpp)
make_test(SRC var.cpp)
make_test(SRC where.cpp)
make_test(SRC wrap.cpp)
make_test(SRC write.cpp)
make_test(SRC ycbcr_rgb.cpp)

foreach(backend ${enabled_backends})
  set(target "basic_c_${backend}")
  add_executable(${target} basic_c.c)
  if(${backend} STREQUAL "unified")
    target_link_libraries(${target}
      PRIVATE
      ArrayFire::af)
  else()
    target_link_libraries(${target}
      PRIVATE
      ArrayFire::af${backend})
  endif()
  add_test(NAME test_${target} COMMAND ${target})
endforeach()

if(AF_TEST_WITH_MTX_FILES)
  make_test(SRC matrixmarket.cpp USE_MMIO)
endif()

add_executable(print_info print_info.cpp)
if(AF_BUILD_UNIFIED)
  target_link_libraries(print_info ArrayFire::af)
elseif(AF_BUILD_OPENCL)
  target_link_libraries(print_info ArrayFire::afopencl)
elseif(AF_BUILD_CUDA)
  target_link_libraries(print_info ArrayFire::afcuda)
elseif(AF_BUILD_CPU)
  target_link_libraries(print_info ArrayFire::afcpu)
endif()

make_test(SRC jit_test_api.cpp)
