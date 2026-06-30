cmake_minimum_required(VERSION 3.22) # requis par SFML 3
project(BobCan LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Les dependances ne sont pas supposees installees sur la machine de build :
# on les recupere et on les compile via FetchContent.
include(FetchContent)

# --- SFML 3 -----------------------------------------------------------------
# On ne compile que ce dont le jeu a besoin (graphics/window/system).
set(SFML_BUILD_AUDIO   OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)
# Liaison statique : un seul binaire, pas de .so a trimballer a cote.
set(BUILD_SHARED_LIBS  OFF CACHE BOOL "" FORCE)

FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG        3.0.2
    GIT_SHALLOW    ON
)

# --- spdlog -----------------------------------------------------------------
# spdlog embarque sa propre copie de {fmt}, donc rien d'autre a installer.
FetchContent_Declare(spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.15.1
    GIT_SHALLOW    ON
)

FetchContent_MakeAvailable(SFML spdlog)

# --- Jeu --------------------------------------------------------------------
add_executable(BobCan BobCan/Main.cpp)

target_link_libraries(BobCan PRIVATE
    SFML::Graphics
    SFML::Window
    SFML::System
    spdlog::spdlog
)

# Le jeu charge ses assets via des chemins relatifs ("Resources/...").
# On copie donc le dossier Resources a cote du binaire pour qu'il tourne
# directement depuis le repertoire de build.
add_custom_command(TARGET BobCan POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/Resources
            $<TARGET_FILE_DIR:BobCan>/Resources
    COMMENT "Copie des Resources a cote du binaire"
)
