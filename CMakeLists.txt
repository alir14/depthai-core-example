cmake_minimum_required(VERSION 3.20)

project(oak-camera-service VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# DepthAI dependency
find_package(depthai REQUIRED)
message(STATUS "Found depthai: ${depthai_DIR}")

# OpenCV (required for visualization)
find_package(OpenCV REQUIRED)
message(STATUS "Found OpenCV: ${OpenCV_DIR}")

# Threading
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Source files
set(ENGINE_SOURCES
    src/engine/EngineManager.cpp
    src/engine/CameraController.cpp
)

set(MODULE_SOURCES
    src/modules/PreviewModule.cpp
    src/modules/RecordModule.cpp
    src/modules/InferenceModule.cpp
)

set(MAIN_SOURCE
    src/main.cpp
)

# Main executable
add_executable(${PROJECT_NAME}
    ${MAIN_SOURCE}
    ${ENGINE_SOURCES}
    ${MODULE_SOURCES}
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        depthai::core
        ${OpenCV_LIBS}
        Threads::Threads
)

# Compiler warnings
if(NOT MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall -Wextra -Wpedantic
        $<$<COMPILE_LANGUAGE:CXX>:-Werror=return-type>
    )
endif()

# Windows DLL handling
if(WIN32)
    if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.21")
        set(depthai_dll_libraries "$<TARGET_RUNTIME_DLLS:${PROJECT_NAME}>")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different 
                ${depthai_dll_libraries} $<TARGET_FILE_DIR:${PROJECT_NAME}>
            COMMAND_EXPAND_LISTS
        )
    endif()
endif()

# Create output directories
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/models)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/recordings)

# Install target
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
