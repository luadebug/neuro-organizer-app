cmake_minimum_required(VERSION 3.24)
project(project_notes_database
        VERSION 6.0.0 # CI_PROJECT_VERSION
)

set(AUI_VERSION v8.0.0-rc.7)
set(UTF8PROC_VERSION 72918b954b4a85c00106842b073a4e5178209f95)

option(BUILD_SHARED_LIBS OFF "")


file(
        DOWNLOAD
        https://raw.githubusercontent.com/aui-framework/aui/${AUI_VERSION}/aui.boot.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/aui.boot.cmake)
include(${CMAKE_CURRENT_BINARY_DIR}/aui.boot.cmake)

auib_import(aui https://github.com/aui-framework/aui
        COMPONENTS core crypt curl image views json updater
        VERSION ${AUI_VERSION})

auib_import(utf8proc https://github.com/JuliaStrings/utf8proc
        VERSION ${UTF8PROC_VERSION})

aui_executable(notes_database)
aui_compile_assets(notes_database)

aui_link(notes_database PRIVATE
        aui::core aui::crypt aui::curl aui::image aui::views aui::json aui::updater
        utf8proc::utf8proc
)

aui_enable_tests(notes_database)

aui_app(TARGET notes_database
        NAME "AUI Neuro Organizer"
        VENDOR "AUI Project"
        ICON "assets/img/icon.svg"
)

#target_precompile_headers(notes_database
#    PRIVATE
#        "${CMAKE_CURRENT_SOURCE_DIR}/include/pch.h"
#)
#
#target_include_directories(notes_database
#    PUBLIC
#        "${CMAKE_CURRENT_SOURCE_DIR}/include"
#)
