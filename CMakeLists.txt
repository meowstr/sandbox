cmake_minimum_required(VERSION 3.19)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)

project(Palooza VERSION 1.0)

# put everything in some convenient folders
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

list (APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/external)


set(ALSOFT_EXAMPLES OFF CACHE BOOL "")
set(ALSOFT_TESTS OFF CACHE BOOL "")
set(ALSOFT_UTILS OFF CACHE BOOL "")

#set(LIBTYPE SHARED CACHE BOOL "")

add_subdirectory(external/flip)
add_subdirectory(external/glad)
add_subdirectory(external/glm)
add_subdirectory(external/glfw)
add_subdirectory(external/openal-soft)
add_subdirectory(external/gameMap)
add_subdirectory(external/simpleMap)

find_package(Threads REQUIRED)
#find_package(glm REQUIRED)
#find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)
#find_package(OpenAL REQUIRED)
#find_package(Guile REQUIRED)

# configure asio
add_library(asio INTERFACE)
target_compile_definitions(asio INTERFACE ASIO_STANDALONE)
target_include_directories(asio INTERFACE external/asio-1.18.1/include)
target_link_libraries(asio INTERFACE ${CMAKE_THREAD_LIBS_INIT})

# configure entt
add_library(entt INTERFACE)
target_include_directories(entt INTERFACE external/entt/src)

# configure rang (colors in the terminal)
add_library(rang INTERFACE)
target_include_directories(rang INTERFACE external/rang/include)

# configure stb
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE external/stb)

# find all my files
file(GLOB CORE_SOURCES
	 "${PROJECT_SOURCE_DIR}/src/core/*.cpp"
	 "${PROJECT_SOURCE_DIR}/src/core/*.h"
     #"${PROJECT_SOURCE_DIR}/src/engine/*.cpp"
	 #"${PROJECT_SOURCE_DIR}/src/engine/*.hpp"
	 #"${PROJECT_SOURCE_DIR}/src/game/*.cpp"
	 #"${PROJECT_SOURCE_DIR}/src/game/*.hpp"
     )

file(GLOB GAME_SOURCES
	 "${PROJECT_SOURCE_DIR}/src/game/*.cpp"
	 "${PROJECT_SOURCE_DIR}/src/game/*.h"
     #"${PROJECT_SOURCE_DIR}/src/engine/*.cpp"
	 #"${PROJECT_SOURCE_DIR}/src/engine/*.hpp"
	 #"${PROJECT_SOURCE_DIR}/src/game/*.cpp"
	 #"${PROJECT_SOURCE_DIR}/src/game/*.hpp"
     )

file(GLOB LAUNCHER_SOURCES
	 "${PROJECT_SOURCE_DIR}/src/launcher/*.cpp"
	 "${PROJECT_SOURCE_DIR}/src/launcher/*.h"
     #"${PROJECT_SOURCE_DIR}/src/engine/*.cpp"
	 #"${PROJECT_SOURCE_DIR}/src/engine/*.hpp"
	 #"${PROJECT_SOURCE_DIR}/src/game/*.cpp"
	 #"${PROJECT_SOURCE_DIR}/src/game/*.hpp"
     )

add_library(core SHARED ${CORE_SOURCES})
target_include_directories(core PUBLIC src/core)
target_link_libraries(core PUBLIC rang glad OpenAL OpenGL::GL OpenGL::GLU glm::glm glfw stb simple-map)

add_executable(palooza ${LAUNCHER_SOURCES})
target_include_directories(palooza PUBLIC src/launcher)
target_link_libraries(palooza PUBLIC core)

add_library(stateHook SHARED src/stateHook/StateHook.cpp)
target_include_directories(stateHook PUBLIC src/game)
target_link_libraries(stateHook PUBLIC core)

add_library(game SHARED ${GAME_SOURCES})
target_include_directories(game PUBLIC src/game)
target_link_libraries(game PUBLIC stateHook)

# optimize out my cheap lambdas
# target_compile_options(palooza PRIVATE -O1)

add_custom_target(run
    COMMAND cd -P bin && ./palooza
    DEPENDS palooza game
    WORKING_DIRECTORY ${CMAKE_PROJECT_DIR}
)
