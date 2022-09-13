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
        xxx_test.cpp
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        fmt::fmt
        range-v3::range-v3
        Boost::headers
        Boost::unit_test_framework
)
#set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "-v")
