project(
  meta
  VERSION 0.0.1
  LANGUAGES CXX)

file(GLOB_RECURSE SOURCES ${PROJECT_SOURCE_DIR}/src/*.cpp)

add_library(${PROJECT_NAME} STATIC ${SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/include)

set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${PROJECT_VERSION})

target_compile_definitions(
  ${PROJECT_NAME}
  PUBLIC $<$<BOOL:${META_ENABLE_GLM_TYPES}>:META_ENABLE_GLM_TYPES>
  PUBLIC
    $<$<BOOL:${META_ENABLE_COLOR_GRADIENT_TYPES}>:META_ENABLE_COLOR_GRADIENT_TYPES>
  PUBLIC $<$<BOOL:${META_ENABLE_ARRAY_TYPES}>:META_ENABLE_ARRAY_TYPES>)

target_compile_definitions(
  ${PROJECT_NAME}
  PRIVATE $<$<CONFIG:Debug>:META_SPDLOG_LEVEL_TRACE>
          $<$<CONFIG:RelWithDebInfo>:META_SPDLOG_LEVEL_DEBUG>
          $<$<CONFIG:Release>:META_SPDLOG_LEVEL_INFO>)

target_link_libraries(${PROJECT_NAME} nlohmann_json::nlohmann_json
                      spdlog::spdlog)

if(META_ENABLE_GLM_TYPES OR META_ENABLE_ARRAY_TYPES)
  target_link_libraries(${PROJECT_NAME} glm::glm)
endif()
