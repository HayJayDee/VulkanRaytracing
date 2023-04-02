cmake_minimum_required(VERSION 3.10)

project(VulkanRaytracing)

set(CMAKE_CXX_STANDARD 11)
set(SOURCE_FILES src/main.cpp)

find_package(Vulkan REQUIRED)

add_subdirectory(libs/glfw)
add_subdirectory(libs/glm)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_include_directories(${PROJECT_NAME} PUBLIC include/ libs/glfw/include libs/glm ${Vulkan_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PUBLIC glm glfw ${Vulkan_LIBRARIES})
