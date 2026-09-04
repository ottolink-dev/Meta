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

set(UNITTEST_SOURCES
  main.cpp
  test_attribute.cpp
  test_attribute_container.cpp
  test_serialization.cpp
  test_attribute_factory.cpp
  test_event.cpp
  test_container_group_sync.cpp
)

set(UNITTEST_LIBS
  meta
  GTest::gtest
)

if(META_ENABLE_QT_UI)
  set(CMAKE_AUTOMOC ON)
  list(APPEND UNITTEST_SOURCES test_design_registry.cpp)
  list(APPEND UNITTEST_LIBS meta_qt)
  find_package(Qt6 REQUIRED COMPONENTS Core Widgets)
  list(APPEND UNITTEST_LIBS Qt6::Core Qt6::Widgets)
endif()

add_executable(meta_unittests
  ${UNITTEST_SOURCES}
)

if(META_ENABLE_QT_UI)
  target_compile_definitions(meta_unittests PRIVATE META_ENABLE_QT_UI)
endif()

target_link_libraries(meta_unittests
  PRIVATE
    ${UNITTEST_LIBS}
)

include(GoogleTest)
gtest_discover_tests(meta_unittests)
