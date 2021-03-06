cmake_minimum_required(VERSION 3.0)
project(Lesson3)
set (CMAKE_CXX_STANDARD 14)

option(GLFW_BUILD_DOCS OFF)
option(GLFW_BUILD_EXAMPLES OFF)
option(GLFW_BUILD_TESTS OFF)
add_subdirectory("${PROJECT_SOURCE_DIR}/../Thirdparty/glfw" "${PROJECT_SOURCE_DIR}/Build/Thirdparty/glfw" )

option(ASSIMP_BUILD_ASSIMP_TOOLS OFF)
option(ASSIMP_BUILD_SAMPLES OFF)
option(ASSIMP_BUILD_TESTS OFF)
add_subdirectory("${PROJECT_SOURCE_DIR}/../Thirdparty/assimp" "${PROJECT_SOURCE_DIR}/Build/Thirdparty/assimp" )

option(BUILD_BULLET2_DEMOS OFF)
option(BUILD_CPU_DEMOS OFF)
option(BUILD_EXTRAS OFF)
option(BUILD_OPENGL3_DEMOS OFF)
option(BUILD_UNIT_TESTS OFF)
add_subdirectory("${PROJECT_SOURCE_DIR}/../Thirdparty/bullet" "${PROJECT_SOURCE_DIR}/Build/Thirdparty/bullet")

#if(MSVC)
#    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
#else()
#    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -std=c++11")
#    if(NOT WIN32)
#        set(GLAD_LIBRARIES dl)
#    endif()
#endif()

include_directories("${PROJECT_SOURCE_DIR}/Headers/"
                    "${PROJECT_SOURCE_DIR}/../Thirdparty/assimp/include/"
                    "${PROJECT_SOURCE_DIR}/../Thirdparty/bullet/src/"
                    "${PROJECT_SOURCE_DIR}/../Thirdparty/glad/include/"
                    "${PROJECT_SOURCE_DIR}/../Thirdparty/glfw/include/"
                    "${PROJECT_SOURCE_DIR}/../Thirdparty/glm/"
                    "${PROJECT_SOURCE_DIR}/../Thirdparty/stb/")

file(GLOB VENDORS_SOURCES "${PROJECT_SOURCE_DIR}/../Thirdparty/glad/src/glad.c")
file(GLOB PROJECT_HEADERS "${PROJECT_SOURCE_DIR}/Headers/*.hpp")
file(GLOB PROJECT_SOURCES "${PROJECT_SOURCE_DIR}/Sources/*.cpp")
file(GLOB PROJECT_SHADERS "${PROJECT_SOURCE_DIR}/Shaders/*.comp"
                          "${PROJECT_SOURCE_DIR}/Shaders/*.frag"
                          "${PROJECT_SOURCE_DIR}/Shaders/*.geom"
                          "${PROJECT_SOURCE_DIR}/Shaders/*.vert")
file(GLOB PROJECT_CONFIGS "${PROJECT_SOURCE_DIR}/CMakeLists.txt"
                          "${PROJECT_SOURCE_DIR}/.gitignore")

source_group("Headers" FILES ${PROJECT_HEADERS})
source_group("Shaders" FILES ${PROJECT_SHADERS})
source_group("Sources" FILES ${PROJECT_SOURCES})
source_group("Thirdpartys" FILES ${VENDORS_SOURCES})

add_definitions(-DGLFW_INCLUDE_NONE
                -DPROJECT_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\")
add_executable(${PROJECT_NAME} ${PROJECT_SOURCES} ${PROJECT_HEADERS}
                               ${PROJECT_SHADERS} ${PROJECT_CONFIGS}
                               ${VENDORS_SOURCES})
target_link_libraries(${PROJECT_NAME} assimp glfw
                      ${GLFW_LIBRARIES} ${GLAD_LIBRARIES}
                      BulletDynamics BulletCollision LinearMath)
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${PROJECT_NAME})

if (MSVC)
    set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
endif()
