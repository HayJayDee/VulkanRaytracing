cmake_minimum_required(VERSION 3.10)

project(VulkanRaytracing)

set(CMAKE_CXX_STANDARD 17)
set(SOURCE_FILES src/main.cpp src/application.cpp src/loader.cpp)
set(CMAKE_CXX_FLAGS_DEBUG_INIT "-Wall")

find_package(Vulkan REQUIRED)

add_subdirectory(libs/glfw)
add_subdirectory(libs/glm)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_compile_definitions(${PROJECT_NAME} PRIVATE $<$<CONFIG:Debug>:DEBUG=1>)

target_include_directories(${PROJECT_NAME} PUBLIC include/ libs/glfw/include libs/glm ${Vulkan_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PUBLIC glm glfw ${Vulkan_LIBRARIES})
