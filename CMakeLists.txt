cmake_minimum_required(VERSION 3.10)

project(VulkanRaytracing)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")
set(SOURCE_FILES src/main.cpp)
add_subdirectory(libs/glfw)
add_subdirectory(libs/glm)

find_package(Vulkan REQUIRED)

add_executable(${PROJECT_NAME} ${SROUCE_FILES})

target_include_directories(${PROJECT_NAME} PUBLIC libs/glfw/include libs/glm)
target_link_libraries(${PROJECT_NAME} PUBLIC glm glfw)
target_include_directories(${PROJECT_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PUBLIC ${Vulkan_LIBRARIES})