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
  src/leaf_errors.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
  fmt::fmt
  range-v3::range-v3
  Boost::headers
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  target_compile_definitions(${PROJECT_NAME} PRIVATE
    _GLIBCXX_DEBUG _GLIBCXX_DEBUG_PEDANTIC
  )
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang") # libc++
  target_compile_definitions(${PROJECT_NAME} PRIVATE
    _LIBCPP_DEBUG
  )
endif()

# ToDo Fix (most) of this issues
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  target_compile_options(${PROJECT_NAME} PRIVATE
    -Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic 
    -Wno-disabled-macro-expansion
    -Wno-padded
    -Wno-shadow
    -Wno-shadow-uncaptured-local
    -Wno-unused-const-variable
    -Wno-sign-conversion
    -Wno-global-constructors -Wno-exit-time-destructors
    -Wno-ctad-maybe-unsupported
  )
endif()

include(CheckTypeSize)
check_type_size(__int128_t __INT128_T LANGUAGE CXX)
check_type_size(__uint128_t __UINT128_T LANGUAGE CXX)
configure_file(
  ${PROJECT_SOURCE_DIR}/include/literal/convert/detail/int_types.hpp.in
  ${PROJECT_BINARY_DIR}/include/literal/convert/detail/int_types.hpp
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
)
