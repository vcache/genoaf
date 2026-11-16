
# Collect source files
file(GLOB_RECURSE GENOAF_SRCS
     LIST_DIRECTORIES FALSE
     "${CMAKE_CURRENT_SOURCE_DIR}/sources/*.cpp")
list(FILTER GENOAF_SRCS EXCLUDE REGEX "_test.cpp$")

# Add library
add_library(genoaf ${GENOAF_SRCS})
target_link_libraries(genoaf
    "${CMAKE_THREAD_LIBS_INIT}"
    glm::glm
    Boost::headers
)

target_compile_features(genoaf PUBLIC ${GENOAF_CXX_STD})

target_include_directories(genoaf PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/sources"
)

target_compile_options(genoaf PRIVATE ${GENOAF_COMP_OPTS})

# Add unit tests
if (GTest_FOUND AND GENOAF_BUILD_TESTS)
    file(GLOB_RECURSE GENOAF_TESTS
         LIST_DIRECTORIES FALSE
         "${CMAKE_CURRENT_SOURCE_DIR}/sources/*_test.cpp")

    include(GoogleTest)

    add_executable(genoaf-tests ${GENOAF_TESTS})
    target_compile_features(genoaf-tests PUBLIC "${GENOAF_CXX_STD}")
    target_link_libraries(genoaf-tests genoaf GTest::gtest GTest::gtest_main)
    target_include_directories(genoaf-tests PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/sources")
    target_compile_options(genoaf-tests PRIVATE ${GENOAF_COMP_OPTS})
    gtest_add_tests(TARGET genoaf-tests)
endif()