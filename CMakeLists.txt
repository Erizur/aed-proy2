cmake_minimum_required(VERSION 3.10.0)
project(proyecto2 VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(USE_VENDORED
       "Use vendored packages instead of system-installed ones" OFF)

if(MSVC)
    set(USE_VENDORED ON)
endif()

if(USE_VENDORED)
  message(STATUS "Using vendored SDL")

  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

  set(SDLTTF_VENDORED ON)
  set(SDLIMAGE_VENDORED ON)
  set(SDLMIXER_VENDORED ON)

  add_subdirectory(${PROJECT_SOURCE_DIR}/vendored/SDL EXCLUDE_FROM_ALL)
  add_subdirectory(${PROJECT_SOURCE_DIR}/vendored/SDL_ttf EXCLUDE_FROM_ALL)
  add_subdirectory(${PROJECT_SOURCE_DIR}/vendored/SDL_image EXCLUDE_FROM_ALL)
  add_subdirectory(${PROJECT_SOURCE_DIR}/vendored/SDL_mixer EXCLUDE_FROM_ALL)
endif()

include_directories(include mapa)
add_executable(proyecto2 main.cpp)

set(BUILD_SHARED_LIBS OFF)

if(NOT USE_VENDORED)
    find_package(SDL3 REQUIRED CONFIG COMPONENTS SDL3-shared)
    find_package(SDL3_ttf REQUIRED CONFIG)
    find_package(SDL3_image REQUIRED)
endif()

target_compile_definitions(proyecto2 PRIVATE "DEBUG=$<IF:$<CONFIG:Debug>,1,0>")

target_link_libraries(proyecto2 PRIVATE SDL3::SDL3 SDL3_ttf::SDL3_ttf SDL3_image::SDL3_image)

if (UNIX AND NOT APPLE AND NOT WIN32)
    string(TOUPPER "${CMAKE_BUILD_TYPE}" UPPERCASE_BUILD_TYPE)

    if (UPPERCASE_BUILD_TYPE STREQUAL "DEBUG")
        set(ASSETS_INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}/assets/")
    else()
        set(ASSETS_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME}/")
    endif()
elseif(APPLE)
    # en macOS usamos los assets copiados a la carpeta de build
    set(ASSETS_INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}/assets/")
else()
    set(ASSETS_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}/assets/")
endif()

target_compile_definitions(proyecto2 PRIVATE
  ASSET_DATA="${ASSETS_INSTALL_DIR}"
)

add_custom_command(
    TARGET proyecto2
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/assets/" "${CMAKE_CURRENT_BINARY_DIR}/assets/"
    COMMENT "Copying assets folder to build directory"
)

install(TARGETS proyecto2 DESTINATION bin)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/assets/ DESTINATION "share/${PROJECT_NAME}/")

add_subdirectory(tests)

# Benchmark R-Tree vs busqueda lineal (consola, no requiere SDL)
add_executable(bench bench.cpp)
target_include_directories(bench PRIVATE include)

if(WIN32)
    # Define the DLLs we need to copy
    set(REQUIRED_DLLS
        "${CMAKE_BINARY_DIR}/vendored/SDL/SDL3.dll"
        "${CMAKE_BINARY_DIR}/vendored/SDL_ttf/SDL3_ttf.dll"
        "${CMAKE_BINARY_DIR}/vendored/SDL_image/SDL3_image.dll"
        "${CMAKE_BINARY_DIR}/vendored/SDL_mixer/SDL3_mixer.dll"
    )

    # Copy DLLs after build
    add_custom_command(TARGET proyecto2 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Copying DLLs to output directory..."
        COMMENT "Copying dependency DLLs"
    )

    foreach(DLL ${REQUIRED_DLLS})
        add_custom_command(TARGET proyecto2 POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${DLL}
                $<TARGET_FILE_DIR:proyecto2>
            COMMENT "Copying ${DLL}"
        )
    endforeach()

    # Copy mcfgthread since static builds dont automatically pass it by default.
    # Shame on you, nix! (jk just fix it and ill forgive you)
    find_file(MCFGTHREAD_DLL
        NAMES libmcfgthread-2.dll libmcfgthread-1.dll
        PATHS
        $ENV{MCFGTHREAD_DLL}
    )

    if(MCFGTHREAD_DLL)
        add_custom_command(TARGET proyecto2 POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${MCFGTHREAD_DLL}
                $<TARGET_FILE_DIR:proyecto2>
            COMMENT "Copying ${MCFGTHREAD_DLL}"
        )
    endif()
endif()