if(NOT META_ENABLE_UNITTESTS)
  return()
endif()

find_package(GTest QUIET)
if(NOT GTest_FOUND)
  include(FetchContent)
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
  )
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)
endif()

enable_testing()

add_executable(meta_unittests
  main.cpp
  test_attribute.cpp
  test_attribute_container.cpp
  test_serialization.cpp
  test_attribute_factory.cpp
  test_event.cpp
)

target_link_libraries(meta_unittests
  PRIVATE
    meta
    GTest::gtest
)

include(GoogleTest)
gtest_discover_tests(meta_unittests)
