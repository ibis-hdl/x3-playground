project(literal LANGUAGES CXX)

find_package(Boost 1.78.0)
add_definitions(${Boost_LIB_DIAGNOSTIC_DEFINITIONS})
find_package(fmt 8.1.1 REQUIRED)
find_package(range-v3 0.11.0 REQUIRED)
find_package(Threads REQUIRED)

add_executable(${PROJECT_NAME})

target_sources(${PROJECT_NAME} PRIVATE
  src/literal.cpp 
  src/ast.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
  fmt::fmt 
  range-v3::range-v3
)

target_include_directories(
  ${PROJECT_NAME} PRIVATE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                          $<BUILD_INTERFACE:${Boost_INCLUDE_DIR}>)
