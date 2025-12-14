cmake_minimum_required(VERSION 3.20)

# Create a project with name 'oak-camera-service'
set(TARGET_NAME oak-camera-service)
project(${TARGET_NAME})

# Depthai dependency
find_package(depthai REQUIRED)
message(STATUS "Found depthai: ${depthai_DIR}")

# Dependencies (optional, only used for example)
find_package(OpenCV REQUIRED)

# Threading library (required for std::thread)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)


# Add source files
#add_executable("${TARGET_NAME}"
#    src/main.cpp
# "src/engine/Types.h" "src/engine/Modulebase.h" "src/engine/Enginemanager.h" "src/engine/Enginemanager.cpp" "src/Previewmodule.h" "src/Previewmodule.cpp")

file(GLOB_RECURSE SOURCES 
    "src/engine/*.cpp"
    "src/modules/*.cpp"
    "src/main.cpp"
)

add_executable("${TARGET_NAME}" ${SOURCES})

# Include directories
target_include_directories(${TARGET_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link with libraries
target_link_libraries(${TARGET_NAME}
    PUBLIC
        depthai::core
        ${OpenCV_LIBS} 
        Threads::Threads
)

# Set some errors
if(NOT MSVC)
    target_compile_options(${TARGET_NAME} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Werror=return-type>)
endif()

# Set compiler features (c++17)
set_property(TARGET ${TARGET_NAME} PROPERTY CXX_STANDARD 17)

# Windows - Add runtime dependencies
if(WIN32)
    if(CMAKE_VERSION VERSION_LESS "3.21")
        message(WARNING "CMake version less than 3.21 - automatic DLL handling not available. Make sure to copy required DLLs to the same folder as .exe")
    else()
        # TARGET_RUNTIME_DLLS generator expression available since CMake 3.21
        set(depthai_dll_libraries "$<TARGET_RUNTIME_DLLS:${TARGET_NAME}>")
        # Copy the required dlls
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD COMMAND
            ${CMAKE_COMMAND} -E copy_if_different ${depthai_dll_libraries} $<TARGET_FILE_DIR:${TARGET_NAME}>
            COMMAND_EXPAND_LISTS
        )
    endif()
endif()
