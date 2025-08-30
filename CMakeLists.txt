cmake_minimum_required(VERSION 3.28)

project(bio)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(BUILD_SHARED_LIBS OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if (PROJECT_IS_TOP_LEVEL AND UNIX)
  # Create symlink to compile_commands.json for IDE to pick it up
  execute_process(
        COMMAND ${CMAKE_COMMAND} -E create_symlink
            ${CMAKE_BINARY_DIR}/compile_commands.json
            ${CMAKE_CURRENT_SOURCE_DIR}/compile_commands.json
    )
endif()

set(PROJECT_NAME prj)
set(PROJECT_NAME_CAPS PRJ)

add_compile_definitions("$<$<CONFIG:Debug>:${PROJECT_NAME_CAPS}_DEBUG>")

set(SOURCES
  src/config/config.cpp
  src/render/gpu_buffer.cpp
  src/render/gpu_texture.cpp
  src/render/renderer.cpp
  src/render/shader.cpp
  src/app_state.cpp
  src/game_state.cpp
  src/gpu.cpp
  src/main.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

set_property(TARGET ${PROJECT_NAME} PROPERTY LINKER_TYPE SYSTEM)

# TODO: asan for MSVC
target_compile_options(${PROJECT_NAME}
  PRIVATE
  "$<$<CONFIG:Debug>:-fsanitize=address>"
  "$<$<CONFIG:Debug>:-fno-omit-frame-pointer>"
)
target_link_options(${PROJECT_NAME}
  PRIVATE
  "$<$<CONFIG:Debug>:-fsanitize=address>"
)

set(SDL_FOLDER external/SDL)
add_subdirectory(${SDL_FOLDER} EXCLUDE_FROM_ALL)

set(SDL_IMAGE_FOLDER external/SDL_image)
add_subdirectory(${SDL_IMAGE_FOLDER} EXCLUDE_FROM_ALL)

set(SDLSHADERCROSS_VENDORED ON)
set(SHADERCROSS_FOLDER external/SDL_shadercross)
add_subdirectory(${SHADERCROSS_FOLDER} EXCLUDE_FROM_ALL)

set(MAGIC_ENUM_DIR external/magic_enum)
add_subdirectory(${MAGIC_ENUM_DIR} EXCLUDE_FROM_ALL)

set(GLM_BUILD_TESTS OFF)
set(GLM_DIR external/glm)
add_subdirectory(${GLM_DIR} EXCLUDE_FROM_ALL)

target_include_directories(${PROJECT_NAME}
  PRIVATE
  ${PROJECT_SOURCE_DIR}/src
  ${PROJECT_SOURCE_DIR}/res
)
target_link_libraries(${PROJECT_NAME}
  PRIVATE
  SDL3::SDL3
  SDL3_image::SDL3_image
  SDL3_shadercross::SDL3_shadercross
  glm::glm
  magic_enum::magic_enum
)
target_compile_definitions(${PROJECT_NAME}
  PRIVATE
  SDL_MAIN_USE_CALLBACKS
)
