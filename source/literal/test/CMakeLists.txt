################################################################################
## IBIS/testsuite event trace project
##
## file: testsuite/event_trace/CMakeLists.txt
################################################################################

project(testrunner_literal LANGUAGES CXX)

add_executable(${PROJECT_NAME})

add_test(NAME test_literal
    COMMAND
        ${PROJECT_NAME} --log_level=error
)

target_sources(${PROJECT_NAME}
    PRIVATE
        testrunner_literal.cpp
        success_test.cpp
        lexeme_failure_test.cpp
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Boost::unit_test_framework
        x3playground::literal
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

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" AND NOT CLANG_MSVC_VARIANT)
  # clang-cl: note: AddressSanitizer doesn't support linking with debug runtime libraries yet
  target_compile_options(${PROJECT_NAME} PRIVATE $<$<CONFIG:Debug>:/fsanitize=address>)
  target_link_options(${PROJECT_NAME} PRIVATE $<$<CONFIG:Debug>:/fsanitize=address>)
elseif(NOT CLANG_MSVC_VARIANT)
  target_compile_options(${PROJECT_NAME} PRIVATE $<$<CONFIG:Debug>:-fsanitize=address,undefined>)
  target_link_options(${PROJECT_NAME} PRIVATE $<$<CONFIG:Debug>:-fsanitize=address,undefined>)
endif()

#set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "-v")
