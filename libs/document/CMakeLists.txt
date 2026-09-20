add_library(vestra_document STATIC
  src/engine_factory.cpp
  src/libreoffice_engine_adapter.cpp
  src/pdfium_engine_adapter.cpp
  src/qt_pdf_engine_adapter.cpp
)
target_include_directories(vestra_document PUBLIC include)
target_link_libraries(vestra_document PUBLIC Qt6::Core Qt6::Gui)

if(VESTRA_ENABLE_QT_PDF)
  target_link_libraries(vestra_document PRIVATE Qt6::Pdf)
  target_compile_definitions(vestra_document PRIVATE VESTRA_ENABLE_QT_PDF=1)
endif()

if(VESTRA_ENABLE_LIBREOFFICE)
  find_path(VESTRA_LOK_INCLUDE_DIR
    NAMES LibreOfficeKit/LibreOfficeKit.h LibreOfficeKit/LibreOfficeKit.hxx
    HINTS "${VESTRA_LIBREOFFICE_ROOT}/include" "${VESTRA_LIBREOFFICE_ROOT}/sdk/include"
    REQUIRED)
  find_library(VESTRA_LOK_LIBRARY
    NAMES LibreOfficeKit libreofficekit mergedlo
    HINTS "${VESTRA_LIBREOFFICE_ROOT}" "${VESTRA_LIBREOFFICE_ROOT}/program" "${VESTRA_LIBREOFFICE_ROOT}/lib"
    REQUIRED)
  target_include_directories(vestra_document PRIVATE "${VESTRA_LOK_INCLUDE_DIR}")
  target_link_libraries(vestra_document PRIVATE "${VESTRA_LOK_LIBRARY}")
  target_compile_definitions(vestra_document PRIVATE VESTRA_ENABLE_LIBREOFFICE=1)
endif()

if(VESTRA_ENABLE_PDFIUM)
  find_path(VESTRA_PDFIUM_INCLUDE_DIR
    NAMES fpdfview.h
    HINTS "${VESTRA_PDFIUM_ROOT}/include"
    REQUIRED)
  find_library(VESTRA_PDFIUM_LIBRARY
    NAMES pdfium
    HINTS "${VESTRA_PDFIUM_ROOT}/lib" "${VESTRA_PDFIUM_ROOT}/bin"
    REQUIRED)
  target_include_directories(vestra_document PRIVATE "${VESTRA_PDFIUM_INCLUDE_DIR}")
  target_link_libraries(vestra_document PRIVATE "${VESTRA_PDFIUM_LIBRARY}")
  target_compile_definitions(vestra_document PRIVATE VESTRA_ENABLE_PDFIUM=1)
endif()

