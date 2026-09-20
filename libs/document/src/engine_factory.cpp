#include "vestra/document/engine_factory.h"

#include "vestra/document/libreoffice_engine_adapter.h"
#include "vestra/document/pdfium_engine_adapter.h"
#include "vestra/document/qt_pdf_engine_adapter.h"

#include <memory>

namespace vestra::document {

std::unique_ptr<DocumentEngine> createDocumentEngine() {
    return std::make_unique<LibreOfficeEngineAdapter>();
}

std::unique_ptr<PdfEngine> createPdfEngine() {
#ifdef VESTRA_ENABLE_QT_PDF
    return std::make_unique<QtPdfEngineAdapter>();
#else
    return std::make_unique<PdfiumEngineAdapter>();
#endif
}

} // namespace vestra::document

