cmake_minimum_required(VERSION 3.25)

project(genoaf)

# Options
set(GENOAF_CXX_STD "cxx_std_23")
set(GENOAF_CXX_STD "${GENOAF_CXX_STD}" CACHE STRING "C++ standard")

set(GENOAF_COMP_OPTS -Wall -Wextra -pedantic -Werror)
set(GENOAF_COMP_OPTS "${GENOAF_COMP_OPTS}" CACHE STRING "C++ build options")

option(GENOAF_BUILD_TESTS "Build unit tests" ON)
option(GENOAF_BUILD_EXAMPLES "Build usage examples" ON)

# Dependencies
find_package(Boost CONFIG COMPONENTS headers REQUIRED)
find_package(glm REQUIRED)
find_package(Threads REQUIRED)

if (GENOAF_BUILD_TESTS)
    message(STATUS "Configuring unit-tests...")
    find_package(GTest REQUIRED)
endif()

# Additional options
add_compile_options("-fmacro-prefix-map=${CMAKE_SOURCE_DIR}/=.") # For shorter __FILE__

# TODO: can't use GLM_FORCE_INLINE because of string_casts
add_definitions(-DGLM_FORCE_CXX17 -DGLM_FORCE_RADIANS)

# The core library
add_subdirectory(library)

# Examples
# if(GENOAF_BUILD_EXAMPLES)
#     message(STATUS "Configuring examples...")
#     add_subdirectory(examples)
# endif()