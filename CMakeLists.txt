cmake_minimum_required(VERSION 3.24)

project(Vestra VERSION 0.1.0 LANGUAGES CXX)

option(VESTRA_ENABLE_LIBREOFFICE "Build the LibreOfficeKit adapter" OFF)
option(VESTRA_ENABLE_PDFIUM "Build the PDFium adapter" OFF)
option(VESTRA_ENABLE_QT_PDF "Use Qt PDF (PDFium-based) as the bundled PDF renderer" ON)
option(VESTRA_ENABLE_CLANG_TIDY "Run clang-tidy while compiling" OFF)

set(VESTRA_LIBREOFFICE_ROOT "" CACHE PATH "LibreOffice/LibreOfficeKit SDK root")
set(VESTRA_PDFIUM_ROOT "" CACHE PATH "PDFium distribution root")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

if(MSVC)
  add_compile_options(/W4 /permissive- /utf-8 /EHsc)
  add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

if(VESTRA_ENABLE_CLANG_TIDY)
  find_program(CLANG_TIDY_EXECUTABLE NAMES clang-tidy REQUIRED)
  set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXECUTABLE}")
endif()

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets Network PrintSupport Svg Test)
if(VESTRA_ENABLE_QT_PDF)
  find_package(Qt6 6.5 REQUIRED COMPONENTS Pdf)
endif()

include(CTest)

add_subdirectory(libs/core)
add_subdirectory(libs/logging)
add_subdirectory(libs/settings)
add_subdirectory(libs/ipc)
add_subdirectory(libs/security)
add_subdirectory(libs/document)
add_subdirectory(libs/sandbox)
add_subdirectory(libs/ui)

add_subdirectory(apps/launcher)
add_subdirectory(apps/writer)
add_subdirectory(apps/pdf)
add_subdirectory(apps/sheets)
add_subdirectory(apps/slides)

add_subdirectory(workers/office-worker)
add_subdirectory(workers/pdf-worker)

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()

