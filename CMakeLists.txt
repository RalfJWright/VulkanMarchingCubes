cmake_minimum_required(VERSION 3.20)

project(renderingnew
        VERSION 0.0.0.0)

file(GLOB_RECURSE SOURCES ${PROJECT_SOURCE_DIR}/src/*.cpp)
#set(CMAKE_CXX_FLAGS "-O3")
add_executable(${PROJECT_NAME} ${SOURCES})

set(Vulkan_INCLUDE_DIRS "${VULKAN_SDK_PATH}/include")
set(Vulkan_LIBRARIES "${VULKAN_SDK_PATH}/lib")
find_package(Vulkan REQUIRED)

include_directories("~/dev/stb")
include_directories("${PROJECT_SOURCE_DIR}/src/shared")
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

message("${CMAKE_SYSTEM_NAME} - ${CMAKE_CXX_COMPILER_ID}")

target_include_directories(${PROJECT_NAME} PUBLIC
  ${Vulkan_INCLUDE_DIRS}
)

target_link_directories(${PROJECT_NAME} PUBLIC
  ${Vulkan_LIBRARIES}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    glm::glm
    glfw
    vulkan
)