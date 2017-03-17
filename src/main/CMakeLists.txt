find_library(CPPFORMAT_LIBRARY NAMES cppformat PATHS ${cppformat_dir}/lib)
# IMPORTANT: do not add other 3rd-party dependency (boost, protobuf, ...)

# c++
set(cpp_dir ${CMAKE_CURRENT_SOURCE_DIR}/c++)
set(sources
   ${cpp_dir}/Categories.cpp
   ${cpp_dir}/FileWriter.cpp
   ${cpp_dir}/Injector.cpp
   ${cpp_dir}/Loggers.cpp
   ${cpp_dir}/SignalManager.cpp
   ${cpp_dir}/SyncSink.cpp
   )
add_library(${PROJECT_NAME} SHARED ${sources})
target_link_libraries(${PROJECT_NAME} common ${CPPFORMAT_LIBRARY} pthread stdc++fs)
target_include_directories(${PROJECT_NAME}
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include
   PRIVATE
   ${cpp_dir}
   )
target_include_directories(${PROJECT_NAME}
   SYSTEM
   PUBLIC
   ${cppformat_dir}/include
   )

