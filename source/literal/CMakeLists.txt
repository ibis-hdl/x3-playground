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
  src/convert.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
  fmt::fmt
  range-v3::range-v3
)

include(CheckTypeSize)
check_type_size(__int128_t __INT128_T LANGUAGE CXX)
check_type_size(__uint128_t __UINT128_T LANGUAGE CXX)
configure_file(
  ${PROJECT_SOURCE_DIR}/include/literal/detail/int_types.hpp.in
  ${PROJECT_BINARY_DIR}/include/literal/detail/int_types.hpp
)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  target_compile_options(${PROJECT_NAME} PRIVATE "$<$<CONFIG:Debug>:/fsanitize=address>")
  target_link_options(${PROJECT_NAME} PRIVATE "$<$<CONFIG:Debug>:/fsanitize=address>")
else()
  target_compile_options(${PROJECT_NAME} PRIVATE "$<$<CONFIG:Debug>:-fsanitize=address,undefined>")
  target_link_options(${PROJECT_NAME} PRIVATE "$<$<CONFIG:Debug>:-fsanitize=address,undefined>")
endif()

target_include_directories(
  ${PROJECT_NAME} PRIVATE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
    $<BUILD_INTERFACE:${Boost_INCLUDE_DIR}>)
