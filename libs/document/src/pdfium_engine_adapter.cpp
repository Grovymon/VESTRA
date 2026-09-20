#include "vestra/document/pdfium_engine_adapter.h"

#ifdef VESTRA_ENABLE_PDFIUM
#include <fpdfview.h>
#endif

namespace vestra::document {

PdfiumEngineAdapter::PdfiumEngineAdapter() {
#ifdef VESTRA_ENABLE_PDFIUM
    FPDF_LIBRARY_CONFIG config{};
    config.version = 2;
    FPDF_InitLibraryWithConfig(&config);
#endif
}

PdfiumEngineAdapter::~PdfiumEngineAdapter() {
    close();
#ifdef VESTRA_ENABLE_PDFIUM
    FPDF_DestroyLibrary();
#endif
}

EngineInfo PdfiumEngineAdapter::info() const {
#ifdef VESTRA_ENABLE_PDFIUM
    return {QStringLiteral("PDFium"), true, true, QStringLiteral("PDFium rendering enabled")};
#else
    return {QStringLiteral("PDFium"), false, false,
            QStringLiteral("Configure with VESTRA_ENABLE_PDFIUM=ON")};
#endif
}

OperationResult PdfiumEngineAdapter::open(const QString& path) {
    close();
#ifdef VESTRA_ENABLE_PDFIUM
    document_ = FPDF_LoadDocument(path.toUtf8().constData(), nullptr);
    if (document_ == nullptr) {
        return OperationResult::failure(EngineError::InvalidDocument,
                                        QStringLiteral("PDFium could not open the document"));
    }
    return OperationResult::success();
#else
    Q_UNUSED(path)
    return OperationResult::failure(EngineError::BackendUnavailable,
                                    QStringLiteral("PDFium support was disabled at build time."));
#endif
}

int PdfiumEngineAdapter::pageCount() const {
#ifdef VESTRA_ENABLE_PDFIUM
    return document_ != nullptr ? FPDF_GetPageCount(static_cast<FPDF_DOCUMENT>(document_)) : 0;
#else
    return 0;
#endif
}

QSizeF PdfiumEngineAdapter::pageSizePoints(const int pageIndex) const {
#ifdef VESTRA_ENABLE_PDFIUM
    if (document_ == nullptr || pageIndex < 0 || pageIndex >= pageCount()) {
        return {};
    }
    FS_SIZEF size{};
    if (!FPDF_GetPageSizeByIndexF(static_cast<FPDF_DOCUMENT>(document_), pageIndex, &size)) {
        return {};
    }
    return {size.width, size.height};
#else
    Q_UNUSED(pageIndex)
    return {};
#endif
}

OperationResult PdfiumEngineAdapter::renderPage(const int pageIndex, const QSize& targetSize,
                                                QImage* output) {
#ifdef VESTRA_ENABLE_PDFIUM
    if (document_ == nullptr || output == nullptr || targetSize.isEmpty() || pageIndex < 0 ||
        pageIndex >= pageCount()) {
        return OperationResult::failure(EngineError::InvalidDocument,
                                        QStringLiteral("Invalid PDF render request"));
    }
    FPDF_PAGE page = FPDF_LoadPage(static_cast<FPDF_DOCUMENT>(document_), pageIndex);
    if (page == nullptr) {
        return OperationResult::failure(EngineError::InvalidDocument,
                                        QStringLiteral("PDFium could not load the requested page"));
    }
    QImage image(targetSize, QImage::Format_ARGB32);
    image.fill(Qt::white);
    FPDF_BITMAP bitmap = FPDFBitmap_CreateEx(image.width(), image.height(), FPDFBitmap_BGRA,
                                             image.bits(), image.bytesPerLine());
    if (bitmap == nullptr) {
        FPDF_ClosePage(page);
        return OperationResult::failure(EngineError::InternalError,
                                        QStringLiteral("PDFium bitmap allocation failed"));
    }
    FPDF_RenderPageBitmap(bitmap, page, 0, 0, image.width(), image.height(), 0, FPDF_ANNOT);
    FPDFBitmap_Destroy(bitmap);
    FPDF_ClosePage(page);
    *output = std::move(image);
    return OperationResult::success();
#else
    Q_UNUSED(pageIndex)
    Q_UNUSED(targetSize)
    Q_UNUSED(output)
    return OperationResult::failure(EngineError::BackendUnavailable,
                                    QStringLiteral("PDFium support was disabled at build time."));
#endif
}

void PdfiumEngineAdapter::close() {
#ifdef VESTRA_ENABLE_PDFIUM
    if (document_ != nullptr) {
        FPDF_CloseDocument(static_cast<FPDF_DOCUMENT>(document_));
        document_ = nullptr;
    }
#else
    document_ = nullptr;
#endif
}

} // namespace vestra::document

