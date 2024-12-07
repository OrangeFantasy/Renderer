cmake_minimum_required(VERSION 3.12)
project(VkRenderer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_SOURCE_DIR}/Binary/Debug)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_SOURCE_DIR}/Binary/Release)

# Vulkan
find_package(Vulkan REQUIRED)
if(Vulkan_FOUND)
    message(STATUS "Vulkan INCLUDE DIR: ${Vulkan_INCLUDE_DIR}")
    message(STATUS "Vulkan LIBRARY: ${Vulkan_LIBRARY}")
endif()

# glm
set(GLM_PATH "${CMAKE_SOURCE_DIR}/ThirdParty/glm" CACHE PATH "GLM Path")
list(APPEND CMAKE_PREFIX_PATH ${GLM_PATH}/cmake/glm)
find_package(glm REQUIRED)
message(STATUS "GLM INCLUDE DIR: ${GLM_INCLUDE_DIRS}")

# glfw
set(GLFW_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/ThirdParty/glfw-3.3.9.bin.WIN64/include")
set(GLFW_LIBRARY_DIR "${CMAKE_SOURCE_DIR}/ThirdParty/glfw-3.3.9.bin.WIN64/lib-vc2022")
set(GLFW_LIBRARIES "${GLFW_LIBRARY_DIR}/glfw3_mt.lib" "${GLFW_LIBRARY_DIR}/glfw3dll.lib")

# Header and source files
set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Source)
file(GLOB_RECURSE _HEADER_FILE_ ${SOURCE_DIR}/*.h ${SOURCE_DIR}/*.hpp)
file(GLOB_RECURSE _SOURCE_FILE_ ${SOURCE_DIR}/*.c ${SOURCE_DIR}/*.cpp)

set(SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Shaders)
file(GLOB_RECURSE SHADER_GLSL_FILE ${SHADER_DIR}/*.vert ${SHADER_DIR}/*.frag)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${_HEADER_FILE_} ${_SOURCE_FILE_} ${SHADER_GLSL_FILE})

# Target executable program
add_executable(${PROJECT_NAME} ${_HEADER_FILE_} ${_SOURCE_FILE_})

# Compile shaders
function(compile_shader TARGET_NAME SHADERS)
    set(GLSLC_PROGRAM $ENV{VK_SDK_PATH}/Bin/glslc.exe)
    foreach(SHADER ${SHADER_GLSL_FILE})
        get_filename_component(SHADER_NAME ${SHADER} NAME)
        string(REPLACE "." "_" SHADER_NAME ${SHADER_NAME})
        set(SPV_FILE "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/Compiled/${SHADER_NAME}.spv")

        add_custom_command(
            TARGET ${PROJECT_NAME} PRE_BUILD
            COMMAND ${GLSLC_PROGRAM} ${SHADER} -o ${SPV_FILE}
            COMMENT "Compiling ${SHADER} to ${SPV_FILE}"
            DEPENDS ${SHADER}
        )
    endforeach()
endfunction()

# compile_shader(${PROJECT_NAME} ${SHADER_GLSL_FILE})

# Compile definitions
target_compile_definitions(${PROJECT_NAME} PUBLIC VULKAN_VALIDATION_ENABLE)
target_compile_definitions(${PROJECT_NAME} PUBLIC SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Shaders/Compiled/")
# target_compile_definitions(${PROJECT_NAME} PUBLIC TARRAY_RANGED_FOR_CHECKS=1)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE 
    ${Vulkan_LIBRARY}
    ${GLFW_LIBRARIES}
)

target_include_directories(${PROJECT_NAME} PUBLIC 
    ${SOURCE_DIR}
    ${Vulkan_INCLUDE_DIR}
    ${GLM_INCLUDE_DIRS}
    ${GLFW_INCLUDE_DIR}
)
