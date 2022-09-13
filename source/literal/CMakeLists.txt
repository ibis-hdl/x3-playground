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
  src/error_handler.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
  fmt::fmt
  range-v3::range-v3
  Boost::headers
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
  #BOOST_SPIRIT_X3_DEBUG  # FIXME something is missing
  #USE_IN_PARSER_CONVERT
  #USE_LEAF_ERROR_TRACE
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
  set(CLANG_MSVC_VARIANT 1)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "GNU")
  set(CLANG_GCC_VARIANT 1)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  # Clang on Linux uses libstdc++
  # Note: this collides with libc++ when using Clang: 'debug/safe_iterator.h' file not found
  target_compile_definitions(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Debug>:_GLIBCXX_DEBUG _GLIBCXX_DEBUG_PEDANTIC>
  )
endif()

# TODO Fix (most) of this issues
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND NOT CLANG_MSVC_VARIANT)
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

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" AND NOT CLANG_MSVC_VARIANT)
  # clang-cl: note: AddressSanitizer doesn't support linking with debug runtime libraries yet
  target_compile_options(${PROJECT_NAME} PRIVATE $<$<CONFIG:Debug>:/fsanitize=address>)
  target_link_options(${PROJECT_NAME} PRIVATE $<$<CONFIG:Debug>:/fsanitize=address>)
elseif(NOT CLANG_MSVC_VARIANT)
  target_compile_options(${PROJECT_NAME} PRIVATE $<$<CONFIG:Debug>:-fsanitize=address,undefined>)
  target_link_options(${PROJECT_NAME} PRIVATE $<$<CONFIG:Debug>:-fsanitize=address,undefined>)
endif()

target_include_directories(
  ${PROJECT_NAME} PRIVATE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
)

add_subdirectory(test)
