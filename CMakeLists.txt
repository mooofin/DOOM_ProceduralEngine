cmake_minimum_required(VERSION 3.28)
project(CMakeSFMLProject LANGUAGES CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Ensure resources are copied to the runtime output directory so the executable
# can find `res/arial.ttf` when run from ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}.
# Use the system SFML (provided by nix-shell) instead of FetchContent so we
# don't attempt to clone/build SFML at configure time.

# Prefer package-provided SFML
find_package(SFML 3.0 COMPONENTS Graphics Audio REQUIRED)

add_executable(main src/main_complete.cpp)
target_compile_features(main PRIVATE cxx_std_17)

# Links both graphics AND audio now
target_link_libraries(main PRIVATE SFML::Graphics SFML::Audio)

# Automatically copies the 'res' folder to the build directory
add_custom_command(TARGET main POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:main>/res"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/res"
    "$<TARGET_FILE_DIR:main>/res"
    COMMENT "Copying res/ to runtime output directory"
)