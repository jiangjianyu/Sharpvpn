cmake_minimum_required(VERSION 2.8)
project(Sharpvpn)
if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")
    set(LINK_PTHREAD "")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    set(LINK_PTHREAD "pthread")
endif(CMAKE_SYSTEM_NAME MATCHES "Linux")
set(SOURCE_FILES src/main.cpp
        src/SharpTunnel.cpp
        src/SharpCrypto.cpp
        src/SharpVpn.cpp src/SharpPackageParser.cpp src/SharpPackageParser.h src/DaemonRuntime.cpp src/DaemonRuntime.h src/RouteTable.cpp src/RouteTable.h)

set(RUDP_FILES  src/rudp/ReliableSocket.cpp
        src/rudp/Package.h
        src/rudp/NetStruct.cpp
        src/rudp/TimeMarco.h
        src/rudp/pthread_wrapper/ConditionVariable.cpp
        src/rudp/pthread_wrapper/Mutex.cpp)

set(CHINADNS_FILES src/chinadns/local_ns_parser.c src/chinadns/chinadns.c src/chinadns/chinadns.h)
set(CONFIG_FILES sample/client.json sample/server.json sample/chnroute.txt)
set(CONFIG_PATH /etc/sharpvpn/)
set(INSTALL_PATH /usr/local/bin/)
set(CMAKE_FILES_DIRECTORY "build/CMakeFiles/")
add_executable(Sharpvpn ${SOURCE_FILES} ${RUDP_FILES} ${CHINADNS_FILES})
target_link_libraries(Sharpvpn glog sodium jsoncpp resolv ${LINK_PTHREAD})
set_target_properties(Sharpvpn PROPERTIES RUNTIME_OUTPUT_DIRECTORY "build")
install(TARGETS Sharpvpn Sharpvpn DESTINATION ${INSTALL_PATH})
install(DIRECTORY DESTINATION ${CONFIG_PATH})
install(FILES ${CONFIG_FILES} DESTINATION ${CONFIG_PATH})
#add_executable(TestCrypto src/SharpCrypto.cpp test/test_crypto.cpp)
#add_executable(TestRoute src/RouteTable.cpp test/test_route.cpp)
#add_executable(TestRudpServer ${RUDP_FILES} test/test_trans.cpp)
#add_executable(TestRudpClient ${RUDP_FILES} test/test_client.cpp)
#add_executable(TestFilter src/rudp/tcp_engine/TcpFilter.cpp src/rudp/pthread_wrapper/Mutex.cpp src/rudp/pthread_wrapper/ConditionVariable.cpp src/rudp/NetStruct.cpp src/SharpCrypto.cpp test/test_filter.cpp)
#add_executable(TestLibnet src/rudp/tcp_engine/TcpFilter.cpp src/rudp/pthread_wrapper/Mutex.cpp src/rudp/pthread_wrapper/ConditionVariable.cpp src/rudp/NetStruct.cpp src/SharpCrypto.cpp test/test_libnet.cpp)
#add_executable(TestTunnel src/SharpTunnel.cpp test/test_tunnel.cpp)
#target_link_libraries(TestCrypto glog sodium)
#target_link_libraries(TestRoute glog)
#target_link_libraries(TestTunnel glog)
#target_link_libraries(TestRudpServer glog)
#target_link_libraries(TestRudpClient glog)
#target_link_libraries(TestFilter glog pcap net sodium)
#target_link_libraries(TestLibnet glog pcap net sodium)