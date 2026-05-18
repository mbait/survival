cmake_minimum_required(VERSION 3.20)

project(survival
    VERSION 0.1.0
    DESCRIPTION "2006 2D shooter, ported from Win32/DirectX 9 to SDL2"
    LANGUAGES CXX)

# ---------------------------------------------------------------------------
# Standard / warnings
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---------------------------------------------------------------------------
# Dependencies — system SDL2 stack
# ---------------------------------------------------------------------------
find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)
find_package(SDL2_ttf REQUIRED)

# SDL2_gfx has no upstream CMake config — fall back to pkg-config, then plain find.
find_package(PkgConfig)
if(PkgConfig_FOUND)
    pkg_check_modules(SDL2_GFX IMPORTED_TARGET SDL2_gfx)
endif()
if(NOT TARGET PkgConfig::SDL2_GFX)
    find_library(SDL2_GFX_LIBRARY NAMES SDL2_gfx REQUIRED)
    find_path(SDL2_GFX_INCLUDE_DIR NAMES SDL2_gfxPrimitives.h PATH_SUFFIXES SDL2 REQUIRED)
    add_library(sdl2_gfx_compat INTERFACE)
    target_include_directories(sdl2_gfx_compat INTERFACE ${SDL2_GFX_INCLUDE_DIR})
    target_link_libraries(sdl2_gfx_compat INTERFACE ${SDL2_GFX_LIBRARY})
    add_library(PkgConfig::SDL2_GFX ALIAS sdl2_gfx_compat)
endif()

# ---------------------------------------------------------------------------
# Game target
# ---------------------------------------------------------------------------
# Source list grows as Phase 1 progresses — each step adds the file(s) it
# ports. The intermediate builds are kept clean (linker-clean, runnable)
# at every commit; sources still tied to Win32/DX live on disk but are
# not yet compiled.
add_executable(game
    platform/main_sdl.cpp        # Phase 1e
    platform/read_ini.cpp        # Phase 1f
    platform/font_cache.cpp      # Phase 1k.2
    gamecode.cpp                 # Phase 1g/1h/1k/1l
    object.cpp                   # Phase 1k
    graphics/sprite2.cpp         # Phase 1i
    graphics/model.cpp           # Phase 1j
    graphics/particles.cpp       # Phase 1j
    physics/math2D.cpp           # Phase 1j (compiles unchanged, just gated)
    physics/physics2D.cpp        # Phase 1j
)

target_include_directories(game PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/compat
)

target_link_libraries(game PRIVATE
    SDL2::SDL2
    SDL2::SDL2main
    SDL2_image::SDL2_image
    SDL2_ttf::SDL2_ttf
    PkgConfig::SDL2_GFX
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(game PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()

# ---------------------------------------------------------------------------
# Working directory: exe expects ./settings.ini and ./data/ as siblings.
# Stage them next to the binary so `./game` works from the build dir.
# ---------------------------------------------------------------------------
add_custom_command(TARGET game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_SOURCE_DIR}/data $<TARGET_FILE_DIR:game>/data
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_SOURCE_DIR}/settings.ini $<TARGET_FILE_DIR:game>/settings.ini
    VERBATIM
)
