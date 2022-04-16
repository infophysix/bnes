cmake_minimum_required(VERSION 3.23)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(bnes
	LANGUAGES CXX
)

include_directories(
	${PROJECT_SOURCE_DIR}/include
	${PROJECT_SOURCE_DIR}/src
)

set(SOURCE_FILES
	src/Bus.cpp
	src/Cartridge.cpp
	src/CPU.cpp
	src/GUI.cpp
	src/Logger.cpp
	src/main.cpp
	src/Mapper.cpp
	src/Mapper000.cpp
	src/PPU.cpp
)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME} PUBLIC ${SDL2_LIBRARIES})