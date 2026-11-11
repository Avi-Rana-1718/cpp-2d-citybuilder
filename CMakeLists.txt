cmake_minimum_required(VERSION 3.20)
project(citybuilder VERSION 2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Check if local SFML cmake config exists; otherwise fetch prebuilt SFML 2.6.1 for MinGW GCC
if(EXISTS "${CMAKE_SOURCE_DIR}/SFML/lib/cmake/SFML")
    set(SFML_DIR "${CMAKE_SOURCE_DIR}/SFML/lib/cmake/SFML" CACHE PATH "SFML config dir" FORCE)
else()
    include(FetchContent)
    FetchContent_Declare(
        SFML
        URL https://github.com/SFML/SFML/releases/download/2.6.1/SFML-2.6.1-windows-gcc-13.1.0-mingw-64-bit.zip
    )
    FetchContent_Populate(SFML)
    set(SFML_DIR "${sfml_SOURCE_DIR}/lib/cmake/SFML" CACHE PATH "SFML config dir" FORCE)
endif()

find_package(SFML 2.6 COMPONENTS graphics audio window system REQUIRED)

# --- Main Executable ---
add_executable(citybuilder main.cpp src/HeadlessWindow.cpp)

target_include_directories(citybuilder PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/game
)

target_compile_options(citybuilder PRIVATE
    -Wall
    -Wextra
)

target_link_libraries(citybuilder PRIVATE
    sfml-graphics
    sfml-audio
    sfml-window
    sfml-system
)

add_custom_command(TARGET citybuilder POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:citybuilder>/assets"
    COMMENT "Copying assets to build directory"
)

if(WIN32)
    if(EXISTS "${CMAKE_SOURCE_DIR}/SFML/bin")
        set(SFML_BIN_DIR "${CMAKE_SOURCE_DIR}/SFML/bin")
    else()
        set(SFML_BIN_DIR "${sfml_SOURCE_DIR}/bin")
    endif()

    add_custom_command(TARGET citybuilder POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${SFML_BIN_DIR}"
            "$<TARGET_FILE_DIR:citybuilder>"
        COMMENT "Copying SFML DLLs to build directory"
    )
endif()

# --- Test Suite ---
enable_testing()

add_executable(citybuilder_tests tests/test_main.cpp)

target_include_directories(citybuilder_tests PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/game
    ${CMAKE_SOURCE_DIR}/tests
)

target_compile_options(citybuilder_tests PRIVATE
    -Wall
    -Wextra
)

target_link_libraries(citybuilder_tests PRIVATE
    sfml-graphics
    sfml-audio
    sfml-window
    sfml-system
)

add_test(NAME citybuilder_tests COMMAND citybuilder_tests)
