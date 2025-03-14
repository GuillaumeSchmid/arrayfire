# Copyright (c) 2021, ArrayFire
# All rights reserved.
#
# This file is distributed under 3-clause BSD license.
# The complete license agreement can be obtained at:
# http://arrayfire.com/licenses/BSD-3-Clause

cmake_minimum_required(VERSION 3.10.2)

include(CMakeModules/AF_vcpkg_options.cmake)

project(ArrayFire VERSION 3.9.0 LANGUAGES C CXX)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules")

include(AFconfigure_deps_vars)
include(AFBuildConfigurations)
include(AFInstallDirs)
include(CMakeDependentOption)
include(InternalUtils)
include(Version)
include(platform)
include(GetPrerequisites)
include(CheckCXXCompilerFlag)
include(CheckSymbolExists)
include(SplitDebugInfo)

# Use the function generate_product_version on Windows
# to attach version info in dll file attributes.
# Make sure to pass appropriate arguments for each backend
# to generate the correct resource file
include(generate_product_version)

set_policies(
  TYPE NEW
  POLICIES CMP0073
           CMP0074
           CMP0077
           CMP0079)
arrayfire_set_cmake_default_variables()

option(AF_WITH_EXTERNAL_PACKAGES_ONLY "Build ArrayFire with External packages only" OFF)
if(AF_WITH_EXTERNAL_PACKAGES_ONLY)
  set(AF_REQUIRED REQUIRED)
endif()

#Set Intel OpenMP as default MKL thread layer
set(MKL_THREAD_LAYER "Intel OpenMP" CACHE STRING "The thread layer to choose for MKL")

find_package(CUDA 9.0)
find_package(cuDNN 4.0)
find_package(OpenCL 1.2)
find_package(OpenGL)
find_package(glad CONFIG QUIET)
find_package(FreeImage)
find_package(Threads)
find_package(FFTW)
find_package(CBLAS)
find_package(LAPACKE)
find_package(Doxygen)
find_package(MKL)
find_package(spdlog QUIET ${AF_REQUIRED})
find_package(fmt QUIET ${AF_REQUIRED})
find_package(span-lite QUIET)
find_package(GTest)
find_package(CLBlast QUIET)
find_package(Boost 1.70 ${AF_REQUIRED})

# CLFFT used in ArrayFire requires a specific fork
#find_package(clFFT QUIET)

include(boost_package)
include(config_ccache)

option(AF_BUILD_CPU      "Build ArrayFire with a CPU backend"        ON)
option(AF_BUILD_CUDA     "Build ArrayFire with a CUDA backend"       ${CUDA_FOUND})
option(AF_BUILD_OPENCL   "Build ArrayFire with a OpenCL backend"     ${OpenCL_FOUND})
option(AF_BUILD_UNIFIED  "Build Backend-Independent ArrayFire API"   ON)
option(AF_BUILD_DOCS     "Create ArrayFire Documentation"            ${DOXYGEN_FOUND})
option(AF_BUILD_EXAMPLES "Build Examples"                            ON)
option(AF_WITH_CUDNN     "Use cuDNN for convolveNN functions"        ${cuDNN_FOUND})
option(AF_BUILD_FORGE
    "Forge libs are not built by default as it is not link time dependency" OFF)

option(AF_WITH_NONFREE  "Build ArrayFire nonfree algorithms"   OFF)
option(AF_WITH_LOGGING  "Build ArrayFire with logging support" ON)
option(AF_WITH_STACKTRACE  "Add stacktraces to the error messages." ON)
option(AF_CACHE_KERNELS_TO_DISK "Enable caching kernels to disk" ON)
option(AF_WITH_STATIC_MKL "Link against static Intel MKL libraries" OFF)
option(AF_WITH_STATIC_CUDA_NUMERIC_LIBS "Link libafcuda with static numeric libraries(cublas, cufft, etc.)" OFF)
option(AF_WITH_SPDLOG_HEADER_ONLY "Build ArrayFire with header only version of spdlog" OFF)
option(AF_WITH_FMT_HEADER_ONLY "Build ArrayFire with header only version of fmt" OFF)

if(AF_WITH_STATIC_CUDA_NUMERIC_LIBS)
  option(AF_WITH_PRUNE_STATIC_CUDA_NUMERIC_LIBS "Prune CUDA static libraries to reduce binary size.(WARNING: May break some libs on older CUDA toolkits for some compute arch)" OFF)
endif()

set(default_compute_library "FFTW/LAPACK/BLAS")
if(MKL_FOUND)
  set(default_compute_library "Intel-MKL")
endif()

af_multiple_option(NAME        AF_COMPUTE_LIBRARY
                   DEFAULT     ${default_compute_library}
                   DESCRIPTION "Compute library for signal processing and linear algebra routines"
                   OPTIONS     "Intel-MKL" "FFTW/LAPACK/BLAS")

if(WIN32)
  af_multiple_option(NAME         AF_STACKTRACE_TYPE
                     DEFAULT      "Windbg"
                     DESCRIPTION  "The type of backtrace features. Windbg(simple), None"
                     OPTIONS       "Windbg" "None")
else()
  af_multiple_option(NAME         AF_STACKTRACE_TYPE
                     DEFAULT      "Basic"
                     DESCRIPTION  "The type of backtrace features. Basic(simple), libbacktrace(fancy), addr2line(fancy), None"
                     OPTIONS       "Basic" "libbacktrace" "addr2line" "None")
endif()

option(AF_INSTALL_STANDALONE "Build installers that include all dependencies" OFF)

cmake_dependent_option(AF_WITH_RELATIVE_TEST_DIR "Use relative paths for the test data directory(For continious integration(CI) purposes only)" OFF
  "BUILD_TESTING" OFF)

cmake_dependent_option(AF_WITH_IMAGEIO "Build ArrayFire with Image IO support" ${FreeImage_FOUND}
                       "FreeImage_FOUND" OFF)
cmake_dependent_option(AF_BUILD_FRAMEWORK "Build an ArrayFire framework for Apple platforms.(Experimental)" OFF
                       "APPLE" OFF)

option(AF_WITH_STATIC_FREEIMAGE "Use Static FreeImage Lib" OFF)

set(AF_WITH_CPUID ON CACHE BOOL "Build with CPUID integration")

af_deprecate(BUILD_CPU             AF_BUILD_CPU)
af_deprecate(BUILD_CUDA            AF_BUILD_CUDA)
af_deprecate(BUILD_OPENCL          AF_BUILD_OPENCL)
af_deprecate(BUILD_UNIFIED         AF_BUILD_UNIFIED)
af_deprecate(BUILD_DOCS            AF_BUILD_DOCS)
af_deprecate(BUILD_NONFREE         AF_WITH_NONFREE)
af_deprecate(BUILD_EXAMPLES        AF_BUILD_EXAMPLES)
af_deprecate(USE_RELATIVE_TEST_DIR AF_WITH_RELATIVE_TEST_DIR)
af_deprecate(USE_FREEIMAGE_STATIC  AF_WITH_STATIC_FREEIMAGE)
af_deprecate(USE_CPUID             AF_WITH_CPUID)
if(DEFINED USE_CPU_MKL OR DEFINED USE_OPENCL_MKL)
  # Cannot use af_deprecated as it expects the new and old variables to store values of
  # same type. In this case, USE_*_MKL variables are BOOLs and AF_COMPUTE_LIBRARY is a STRING
  message(DEPRECATION
    "Variables USE_CPU_MKL/USE_OPENCL_MKL are deprecated. Use AF_COMPUTE_LIBRARY instead.")
  message(WARNING
    "USE_CPU_MKL/USE_OPENCL_MKL defined. These values take precendence over the value of
    AF_COMPUTE_LIBRARY until they are removed to preserve existing build behavior.")
  # Until USE_CPU_MKL and USE_OPENCL_MKL are removed, if they are defined, they take
  # precendence and cmake will check and report error if Intel-MKL is not found
  if(USE_CPU_MKL OR USE_OPENCL_MKL)
    get_property(doc CACHE AF_COMPUTE_LIBRARY PROPERTY HELPSTRING)
    set(AF_COMPUTE_LIBRARY "Intel-MKL" CACHE STRING "${doc}" FORCE)
  endif()
endif()

mark_as_advanced(
  AF_BUILD_FRAMEWORK
  AF_CACHE_KERNELS_TO_DISK
  AF_INSTALL_STANDALONE
  AF_WITH_CPUID
  AF_WITH_LOGGING
  AF_WITH_STACKTRACE
  AF_WITH_STATIC_FREEIMAGE
  AF_WITH_NONFREE
  AF_WITH_IMAGEIO
  AF_WITH_RELATIVE_TEST_DIR
  AF_TEST_WITH_MTX_FILES
  ArrayFire_DIR
  Boost_INCLUDE_DIR
  CLEAR CUDA_VERSION
  CUDA_HOST_COMPILER
  CUDA_SDK_ROOT_DIR
  CUDA_USE_STATIC_CUDA_RUNTIME
  CUDA_rt_LIBRARY
  SPDLOG_BUILD_EXAMPLES
  SPDLOG_BUILD_TESTING
  ADDR2LINE_PROGRAM
  Backtrace_LIBRARY
  AF_WITH_STATIC_MKL
  GIT
  Forge_DIR
  glad_DIR
  spdlog_DIR
  FG_BUILD_OFFLINE
  )

if(AF_COMPUTE_LIBRARY STREQUAL "Intel-MKL")
  set(BLA_VENDOR "Intel10_64lp")
  if(MKL_THREAD_LAYER STREQUAL "Sequential")
    set(BLA_VENDOR "${BLA_VENDOR}_seq")
  endif()
endif()
find_package(BLAS)
find_package(LAPACK)

# IF: the old USE_CPU_MKL/USE_OPENCL_MKL flags are present,
# THEN Irrespective of AF_COMPUTE_LIBRARY value, continue with MKL to preserve old
#      behavior. Once the deprecated USE_CPU_MKL/USE_OPENCL_MKL are removed in later
#      versions AF_COMPUTE_LIBRARY will take over total control of selecting CPU
#      compute backend.
#
# Note that the default value of AF_COMPUTE_LIBRARY is Intel-MKL.
# Also, cmake doesn't have short-circuit of OR/AND conditions in if
if(${AF_BUILD_CPU} OR ${AF_BUILD_OPENCL})
  if("${AF_COMPUTE_LIBRARY}" STREQUAL "Intel-MKL"
      OR "${AF_COMPUTE_LIBRARY}" STREQUAL "MKL")
    af_mkl_batch_check()
    dependency_check(MKL_FOUND "Please ensure Intel-MKL / oneAPI-oneMKL is installed")
    set(BUILD_WITH_MKL ON)
  elseif("${AF_COMPUTE_LIBRARY}" STREQUAL "FFTW/LAPACK/BLAS")
    dependency_check(FFTW_FOUND "FFTW not found")
    dependency_check(CBLAS_FOUND "CBLAS not found")
    if(UNIX AND NOT APPLE)
      dependency_check(LAPACK_FOUND "LAPACK not found")
    endif()
  endif()
endif()

#Configure forge submodule
#forge is included in ALL target if AF_BUILD_FORGE is ON
#otherwise, forge is not built at all
include(AFconfigure_forge_dep)

if(TARGET fmt::fmt AND AF_WITH_FMT_HEADER_ONLY)
  set_target_properties(fmt::fmt
    PROPERTIES
      INTERFACE_COMPILE_DEFINITIONS "FMT_HEADER_ONLY=1")
endif()

if(TARGET spdlog::spdlog OR AF_WITH_EXTERNAL_PACKAGES_ONLY)
  if(AF_WITH_SPDLOG_HEADER_ONLY)
    add_library(af_spdlog ALIAS spdlog::spdlog_header_only)
  else()
    add_library(af_spdlog ALIAS spdlog::spdlog)
  endif()
else()
  add_library(af_spdlog INTERFACE)
  af_dep_check_and_populate(${spdlog_prefix}
    URI https://github.com/gabime/spdlog.git
    REF v1.9.2
  )
  add_subdirectory(${${spdlog_prefix}_SOURCE_DIR} ${${spdlog_prefix}_BINARY_DIR} EXCLUDE_FROM_ALL)

  target_include_directories(af_spdlog INTERFACE "${${spdlog_prefix}_SOURCE_DIR}/include")
  if(TARGET fmt::fmt)
    set_target_properties(af_spdlog
      PROPERTIES
        INTERFACE_COMPILE_DEFINITIONS "SPDLOG_FMT_EXTERNAL")
  endif()
  if(AF_WITH_SPDLOG_HEADER_ONLY)
    set_target_properties(af_spdlog
      PROPERTIES
        INTERFACE_COMPILE_DEFINITIONS "$<TARGET_PROPERTY:af_spdlog,INTERFACE_COMPILE_DEFINITIONS>;SPDLOG_HEADER_ONLY")
  endif()
endif()

if(NOT TARGET glad::glad)
  af_dep_check_and_populate(${glad_prefix}
    URI https://github.com/arrayfire/glad.git
    REF main
  )
  add_subdirectory(${${glad_prefix}_SOURCE_DIR} ${${glad_prefix}_BINARY_DIR})

  add_library(af_glad STATIC $<TARGET_OBJECTS:af_glad_obj_lib>)
  target_link_libraries(af_glad PUBLIC ${CMAKE_DL_LIBS})
  target_include_directories(af_glad
    SYSTEM PUBLIC
      $<BUILD_INTERFACE:$<TARGET_PROPERTY:af_glad_obj_lib,INTERFACE_INCLUDE_DIRECTORIES>>)
endif()

if(NOT TARGET nonstd::span-lite)
  af_dep_check_and_populate(span-lite
    URI https://github.com/martinmoene/span-lite
    REF "ccf2351"
    )
  add_subdirectory(${span-lite_SOURCE_DIR} EXCLUDE_FROM_ALL)
endif()

af_dep_check_and_populate(${assets_prefix}
  URI https://github.com/arrayfire/assets.git
  REF master
)
set(ASSETS_DIR ${${assets_prefix}_SOURCE_DIR})

configure_file(
    ${ArrayFire_SOURCE_DIR}/CMakeModules/version.hpp.in
    ${ArrayFire_BINARY_DIR}/version.hpp
)

# when crosscompiling use the bin2cpp file from the native bin directory
if(CMAKE_CROSSCOMPILING)
  set(NATIVE_BIN_DIR "NATIVE_BIN_DIR-NOTFOUND"
    CACHE FILEPATH "Path to the Native build directory.")
  if(NATIVE_BIN_DIR)
    include(${NATIVE_BIN_DIR}/ImportExecutables.cmake)
  else()
    message(SEND_ERROR "Native Directory not found. Run cmake in a separate"
                       "directory and build the bin2cpp target.")
  endif()
else()
  add_executable(bin2cpp ${ArrayFire_SOURCE_DIR}/CMakeModules/bin2cpp.cpp
                         ${ArrayFire_SOURCE_DIR}/src/backend/common/util.cpp)

  # NOSPDLOG is used to remove the spdlog dependency from bin2cpp
  target_compile_definitions(bin2cpp PRIVATE NOSPDLOG)
  if(WIN32)
    target_compile_definitions(bin2cpp PRIVATE OS_WIN)
  elseif(APPLE)
    target_compile_definitions(bin2cpp PRIVATE OS_MAC)
  elseif(UNIX)
    target_compile_definitions(bin2cpp PRIVATE OS_LNX)
  endif()
  target_include_directories(bin2cpp PRIVATE
                             ${ArrayFire_SOURCE_DIR}/include
                             ${ArrayFire_BINARY_DIR}/include
                             ${ArrayFire_SOURCE_DIR}/src/backend)
  export(TARGETS bin2cpp FILE ${CMAKE_BINARY_DIR}/ImportExecutables.cmake)
endif()


if(NOT LAPACK_FOUND)
    if(APPLE)
        # UNSET THE VARIABLES FROM LAPACKE
        unset(LAPACKE_LIB CACHE)
        unset(LAPACK_LIB CACHE)
        unset(LAPACKE_INCLUDES CACHE)
        unset(LAPACKE_ROOT_DIR CACHE)
    endif()
endif()

add_subdirectory(src/backend/common)
add_subdirectory(src/api/c)
add_subdirectory(src/api/cpp)

conditional_directory(AF_BUILD_CPU     src/backend/cpu)
conditional_directory(AF_BUILD_CUDA    src/backend/cuda)
conditional_directory(AF_BUILD_OPENCL  src/backend/opencl)
conditional_directory(AF_BUILD_UNIFIED src/api/unified)

if(TARGET af)
  list(APPEND built_backends af)
endif()

if(TARGET afcpu)
  list(APPEND built_backends afcpu)
endif()

if(TARGET afcuda)
  list(APPEND built_backends afcuda)
endif()

if(TARGET afopencl)
  list(APPEND built_backends afopencl)
endif()

set_target_properties(${built_backends} PROPERTIES
                      VERSION "${ArrayFire_VERSION}"
                      SOVERSION "${ArrayFire_VERSION_MAJOR}")

if(AF_INSTALL_STANDALONE)

  # This flag enables the use of RUNPATH instead of RPATH which is the
  # preferred method to set the runtime lookup. Only doind this for
  # standalone builds because we include all libraries with the installers
  # and they are included in the same directory so the RUNPATH is set to
  # $ORIGIN. This avoid setting the linker path in ld.so.conf.d
  check_cxx_compiler_flag("-Wl,--enable-new-dtags" HAS_RUNPATH_FLAG)
  if(HAS_RUNPATH_FLAG)
    set_target_properties(${built_backends} PROPERTIES
      INSTALL_RPATH "$ORIGIN"
      LINK_OPTIONS "-Wl,--enable-new-dtags")
  endif()
endif()

# On some distributions the linker will not add a library to the ELF header if
# the symbols are not needed when the library was first parsed by the linker.
# This causes undefined references issues when linking with libraries which have
# circular dependencies.
if(UNIX AND NOT APPLE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  set_target_properties(${built_backends} PROPERTIES
                        LINK_FLAGS "-Wl,--no-as-needed")
endif()


find_library(Backtrace_LIBRARY backtrace
  DOC "libbacktrace.so file for more informative stacktraces. https://github.com/ianlancetaylor/libbacktrace")
find_program(ADDR2LINE_PROGRAM addr2line
  DOC "The path to the addr2line program for informative stacktraces")

foreach(backend ${built_backends})
  target_compile_definitions(${backend} PRIVATE AFDLL)
  if(AF_WITH_LOGGING)
    target_compile_definitions(${backend}
      PRIVATE AF_WITH_LOGGING)
  endif()
  if(AF_CACHE_KERNELS_TO_DISK)
    target_compile_definitions(${backend}
      PRIVATE AF_CACHE_KERNELS_TO_DISK)
  endif()
endforeach()

if(AF_BUILD_FRAMEWORK)
  set_target_properties(${built_backends}
    PROPERTIES
      FRAMEWORK TRUE
      FRAMEWORK_VERSION A
      MACOSX_FRAMEWORK_IDENTIFIER com.arrayfire.arrayfireFramework
      #MACOSX_FRAMEWORK_INFO_PLIST Info.plist
      #PUBLIC_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/include/arrayfire.h;${af_headers}"
      #XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer"
    )
endif()

install(DIRECTORY include/ DESTINATION ${AF_INSTALL_INC_DIR}
    COMPONENT headers
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN ".gitignore" EXCLUDE
)

## The ArrayFire version file is generated and won't be included above, install
## it separately.
install(FILES ${ArrayFire_BINARY_DIR}/include/af/version.h
              ${ArrayFire_BINARY_DIR}/include/af/compilers.h
        DESTINATION "${AF_INSTALL_INC_DIR}/af/"
        COMPONENT headers)

# install the examples irrespective of the AF_BUILD_EXAMPLES value
# only the examples source files are installed, so the installation of these
# source files does not depend on AF_BUILD_EXAMPLES
# when AF_BUILD_EXAMPLES is OFF, the examples source is installed without
# building the example executables
install(DIRECTORY examples/ #NOTE The slash at the end is important
    DESTINATION ${AF_INSTALL_EXAMPLE_DIR}
    COMPONENT examples)

install(DIRECTORY ${ASSETS_DIR}/examples/ #NOTE The slash at the end is important
    DESTINATION ${AF_INSTALL_EXAMPLE_DIR}
    COMPONENT examples)

install(DIRECTORY "${ArrayFire_SOURCE_DIR}/LICENSES/"
    DESTINATION LICENSES
    COMPONENT licenses)

foreach(backend CPU CUDA OpenCL Unified)
  string(TOUPPER ${backend} upper_backend)
  string(TOLOWER ${backend} lower_backend)
  if(AF_BUILD_${upper_backend})
    install(EXPORT ArrayFire${backend}Targets
            NAMESPACE ArrayFire::
            DESTINATION ${AF_INSTALL_CMAKE_DIR}
            COMPONENT ${lower_backend}_dev)

    export( EXPORT ArrayFire${backend}Targets
            NAMESPACE ArrayFire::
            FILE cmake/ArrayFire${backend}Targets.cmake)
  endif()
endforeach()

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${ArrayFire_BINARY_DIR}/ArrayFireConfigVersion.cmake"
  COMPATIBILITY SameMajorVersion
)

# This config file will be installed so we need to set the install_destination
# path relitive to the install path
set(INCLUDE_DIRS include)
set(CMAKE_DIR ${AF_INSTALL_CMAKE_DIR})
configure_package_config_file(
  ${ArrayFire_SOURCE_DIR}/CMakeModules/ArrayFireConfig.cmake.in
  cmake/install/ArrayFireConfig.cmake
  INSTALL_DESTINATION "${AF_INSTALL_CMAKE_DIR}"
  PATH_VARS INCLUDE_DIRS CMAKE_DIR
  )

install(FILES ${ArrayFire_BINARY_DIR}/cmake/install/ArrayFireConfig.cmake
              ${ArrayFire_BINARY_DIR}/ArrayFireConfigVersion.cmake
              DESTINATION ${AF_INSTALL_CMAKE_DIR}
              COMPONENT cmake)

if(BUILD_WITH_MKL AND AF_INSTALL_STANDALONE)
  if(TARGET MKL::ThreadingLibrary)
    get_filename_component(mkl_tl ${MKL_ThreadingLibrary_LINK_LIBRARY} REALPATH)
    install(FILES
      $<TARGET_FILE:MKL::ThreadingLibrary>
      ${mkl_tl}
      DESTINATION ${AF_INSTALL_LIB_DIR}
      COMPONENT mkl_dependencies)
  endif()

  if(NOT AF_WITH_STATIC_MKL AND TARGET MKL::Shared)
    if(NOT WIN32)
      get_filename_component(mkl_int ${MKL_Interface_LINK_LIBRARY} REALPATH)
      install(FILES
        $<TARGET_FILE:MKL::Interface>
        ${mkl_int}
        DESTINATION ${AF_INSTALL_LIB_DIR}
        COMPONENT mkl_dependencies)
    endif()

    get_filename_component(mkl_rnt ${MKL_RT_LINK_LIBRARY} REALPATH)
    get_filename_component(mkl_shd ${MKL_Core_LINK_LIBRARY} REALPATH)
    get_filename_component(mkl_tly ${MKL_ThreadLayer_LINK_LIBRARY} REALPATH)
    install(FILES
      ${mkl_rnt}
      ${mkl_shd}
      ${mkl_tly}
      $<TARGET_FILE:MKL::RT>
      $<TARGET_FILE:MKL::Shared>
      $<TARGET_FILE:MKL::ThreadLayer>
      ${MKL_RUNTIME_KERNEL_LIBRARIES}

      # This variable is used to add tbb.so.2 library because the main lib
      # is a linker script and not a symlink so it cant be resolved using
      # get_filename_component
      ${AF_ADDITIONAL_MKL_LIBRARIES}
      DESTINATION ${AF_INSTALL_LIB_DIR}
      COMPONENT mkl_dependencies)
  endif()
endif()

# This file will be used to create the config file for the build directory.
# These config files will be used by the examples to find the ArrayFire
# libraries
set(INCLUDE_DIRS "${ArrayFire_SOURCE_DIR}/include" "${ArrayFire_BINARY_DIR}/include")
set(CMAKE_DIR "${ArrayFire_BINARY_DIR}/cmake")
configure_package_config_file(
  ${ArrayFire_SOURCE_DIR}/CMakeModules/ArrayFireConfig.cmake.in
  ArrayFireConfig.cmake
  INSTALL_DESTINATION "${ArrayFire_BINARY_DIR}"
  PATH_VARS INCLUDE_DIRS CMAKE_DIR
  INSTALL_PREFIX "${ArrayFire_BINARY_DIR}"
  )

# Registers the current build directory with the user's cmake config. This will
# create a file at $HOME/.cmake/packages/ArrayFire which will point to this source
# build directory.
# TODO(umar): Disable for now. Causing issues with builds on windows.
#export(PACKAGE ArrayFire)

# Unset the visibility to avoid setting policy commands for older versions of
# CMake for examples and tests.
unset(CMAKE_CXX_VISIBILITY_PRESET)

configure_file(
  ${ArrayFire_SOURCE_DIR}/CMakeModules/CTestCustom.cmake
  ${ArrayFire_BINARY_DIR}/CTestCustom.cmake)

include(CTest)

# Handle depricated BUILD_TEST variable if found.
if(BUILD_TEST)
  set(BUILD_TESTING ${BUILD_TEST})
endif()

conditional_directory(BUILD_TESTING test)

conditional_directory(AF_BUILD_EXAMPLES examples)
conditional_directory(AF_BUILD_DOCS docs)

include(CPackConfig)

# VCPKG variables that aren't necessarily important
# for ArrayFire Development. They are marked hidden.
# If VCPKG is not used, marking them is not harmful
mark_as_advanced(
  VCPKG_APPLOCAL_DEPS
  VCPKG_BOOTSTRAP_OPTIONS
  VCPKG_INSTALL_OPTIONS
  VCPKG_MANIFEST_DIR
  VCPKG_MANIFEST_INSTALL
  VCPKG_MANIFEST_MODE
  VCPKG_OVERLAY_PORTS
  VCPKG_OVERLAY_TRIPLETS
  VCPKG_TARGET_TRIPLET
  X_VCPKG_APPLOCAL_DEPS_INSTALL
  X_VCPKG_APPLOCAL_DEPS_SERIALIZED
  Z_VCPKG_BUILTIN_POWERSHELL_PATH
  Z_VCPKG_PWSH_PATH
  Z_VCPKG_CL
  _VCPKG_INSTALLED_DIR
  )
