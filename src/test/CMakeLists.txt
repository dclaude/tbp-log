# c++
set(cpp_dir ${CMAKE_CURRENT_SOURCE_DIR}/c++)
set(sources
   ${cpp_dir}/main.cpp
   ${cpp_dir}/EncoderTest.cpp
   ${cpp_dir}/SyncLoggerPerfTest.cpp
   ${cpp_dir}/AsyncLoggerPerfTest.cpp
   ${cpp_dir}/test/Context.cpp
   )
if (NOT ${MOCK_MODE} STREQUAL "PERF")
   # add tests that use mocks and that will "not work" if MOCK_PERF is defined
   set(sources
      ${sources}
      ${cpp_dir}/SyncLoggerTest.cpp # needs a SyncSink mock which uses MOCK_NPERF_VIRTUAL (the logger must not be a mock in performance tests)
      ${cpp_dir}/AsyncLoggerTest.cpp
      )
endif()
add_executable(${PROJECT_NAME}-test ${sources}) 
# IMPORTANT: log-test depends on tools but log "must not" depend on tools (to be able to use the logger elsewhere easily)
target_link_libraries(${PROJECT_NAME}-test log tools gtest gmock)
target_include_directories(${PROJECT_NAME}-test
   PRIVATE
   ../main/c++
   ${CMAKE_CURRENT_SOURCE_DIR}/include
   ${cpp_dir}
   )
target_include_directories(${PROJECT_NAME}-test
   SYSTEM
   PRIVATE
   ${gtest_dir}/include
   ${gmock_dir}/include
   )
if (WITH_SPDLOG)
   target_include_directories(${PROJECT_NAME}-test SYSTEM PRIVATE ${spdlog_dir}/include)
endif()

# test targets
set(cmd_args ${CMAKE_CURRENT_BINARY_DIR})
add_custom_target(${PROJECT_NAME}-check 
   ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-test ${cmd_args}
   DEPENDS ${PROJECT_NAME}-test
   WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
# multithreaded tests too slow under valgrind/memcheck
set(excluded_tests
   AsyncLoggerTest.FatalError
   )
join("${excluded_tests}" ":" excluded_tests_arg)
add_custom_target(${PROJECT_NAME}-memcheck 
   valgrind --leak-check=full --num-callers=40 --xml=yes --xml-file=${CMAKE_CURRENT_BINARY_DIR}/memcheck.xml ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-test --gtest_filter=-${excluded_tests_arg} ${cmd_args}
   DEPENDS ${PROJECT_NAME}-test
   WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

