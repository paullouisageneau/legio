cmake_minimum_required(VERSION 3.7)
project(legio
	DESCRIPTION "Legio"
	VERSION 0.1.0
	LANGUAGES CXX)

file(GLOB_RECURSE LEGIO_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
file(GLOB_RECURSE LEGIO_IMPL_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/impl/*.cpp)

add_library(legio STATIC ${LEGIO_SOURCES} ${LEGIO_IMPL_SOURCES})
set_target_properties(legio PROPERTIES
	VERSION ${PROJECT_VERSION}
	CXX_STANDARD 17)

target_include_directories(legio PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(legio PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/legio)
target_include_directories(legio PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(legio PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/deps) # for cryptopp

add_subdirectory(deps/cryptopp EXCLUDE_FROM_ALL)
target_link_libraries(legio cryptopp-static)

if(CMAKE_SYSTEM_NAME MATCHES "Emscripten")
	set(WASM_OPTS
		"SHELL:-s WASM=1"
		"SHELL:-s BINARYEN_METHOD=native-wasm")

	target_compile_options(legio PUBLIC ${WASM_OPTS})
	target_link_options(legio PUBLIC ${WASM_OPTS})

	add_subdirectory(deps/datachannel-wasm EXCLUDE_FROM_ALL)
    target_link_libraries(legio datachannel-wasm)
else()
    option(NO_MEDIA "Disable media support in libdatachannel" ON)
    option(NO_EXPORT_HEADER "Disable export header for libjuice" ON) # causes build issues
    add_subdirectory(deps/libdatachannel EXCLUDE_FROM_ALL)
    target_link_libraries(legio LibDataChannel::LibDataChannel)
    target_link_libraries(legio LibJuice::LibJuice) # dependency of libdatachannel
endif()

file(GLOB_RECURSE LEGIO_PEER_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/peer/*.cpp)

add_executable(legio-peer ${LEGIO_PEER_SOURCES})
set_target_properties(legio-peer PROPERTIES
	VERSION ${PROJECT_VERSION}
	CXX_STANDARD 17)

target_link_libraries(legio-peer legio)

