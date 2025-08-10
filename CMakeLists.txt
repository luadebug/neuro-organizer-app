cmake_minimum_required(VERSION 3.30)
project(project_notes_database)

set(AUI_VERSION v7.1.2)

file(
        DOWNLOAD
        https://raw.githubusercontent.com/aui-framework/aui/${AUI_VERSION}/aui.boot.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/aui.boot.cmake)
include(${CMAKE_CURRENT_BINARY_DIR}/aui.boot.cmake)

auib_import(aui https://github.com/aui-framework/aui
        COMPONENTS core image views json
        VERSION ${AUI_VERSION})

auib_import(utf8proc https://github.com/JuliaStrings/utf8proc
        VERSION master)

aui_executable(notes_database)
aui_compile_assets(notes_database)

aui_link(notes_database PRIVATE
        aui::core aui::image aui::views aui::json
        utf8proc::utf8proc
)

target_precompile_headers(notes_database
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include/pch.h"
)

target_include_directories(notes_database
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
