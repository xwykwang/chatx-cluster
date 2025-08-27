set(SRC_LIST ./muduo_server.cpp)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
link_directories(/usr/local/lib)
add_executable(server ${SRC_LIST})

target_link_libraries(server 
    /usr/local/lib/libmuduo_net.a 
    /usr/local/lib/libmuduo_base.a
    pthread
)
#target_link_libraries(server muduo.net muduo.base pthread)