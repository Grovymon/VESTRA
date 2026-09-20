#include "vestra/document/qt_pdf_engine_adapter.h"

#ifdef VESTRA_ENABLE_QT_PDF
#include <QPdfDocument>
#include <QPdfDocumentRenderOptions>
#endif

namespace vestra::document {

QtPdfEngineAdapter::QtPdfEngineAdapter() {
#ifdef VESTRA_ENABLE_QT_PDF
    document_ = std::make_unique<QPdfDocument>();
#endif
}

QtPdfEngineAdapter::~QtPdfEngineAdapter() = default;

EngineInfo QtPdfEngineAdapter::info() const {
#ifdef VESTRA_ENABLE_QT_PDF
    return {QStringLiteral("Qt PDF / PDFium"), true, true,
            QStringLiteral("Bundled PDF rendering enabled")};
#else
    return {QStringLiteral("Qt PDF / PDFium"), false, false,
            QStringLiteral("Configure with VESTRA_ENABLE_QT_PDF=ON")};
#endif
}

OperationResult QtPdfEngineAdapter::open(const QString& path) {
    close();
#ifdef VESTRA_ENABLE_QT_PDF
    document_ = std::make_unique<QPdfDocument>();
    const QPdfDocument::Error error = document_->load(path);
    if (error != QPdfDocument::Error::None) {
        return OperationResult::failure(
            EngineError::InvalidDocument,
            QStringLiteral("Qt PDF could not open the document (%1)").arg(static_cast<int>(error)));
    }
    return OperationResult::success();
#else
    Q_UNUSED(path)
    return OperationResult::failure(EngineError::BackendUnavailable,
                                    QStringLiteral("Qt PDF support was disabled at build time."));
#endif
}

int QtPdfEngineAdapter::pageCount() const {
#ifdef VESTRA_ENABLE_QT_PDF
    return document_ != nullptr ? document_->pageCount() : 0;
#else
    return 0;
#endif
}

QSizeF QtPdfEngineAdapter::pageSizePoints(const int pageIndex) const {
#ifdef VESTRA_ENABLE_QT_PDF
    if (document_ == nullptr || pageIndex < 0 || pageIndex >= document_->pageCount()) {
        return {};
    }
    return document_->pagePointSize(pageIndex);
#else
    Q_UNUSED(pageIndex)
    return {};
#endif
}

OperationResult QtPdfEngineAdapter::renderPage(const int pageIndex, const QSize& targetSize,
                                               QImage* output) {
#ifdef VESTRA_ENABLE_QT_PDF
    if (document_ == nullptr || output == nullptr || targetSize.isEmpty() || pageIndex < 0 ||
        pageIndex >= document_->pageCount()) {
        return OperationResult::failure(EngineError::InvalidDocument,
                                        QStringLiteral("Invalid PDF render request"));
    }
    *output = document_->render(pageIndex, targetSize, QPdfDocumentRenderOptions());
    if (output->isNull()) {
        return OperationResult::failure(EngineError::InvalidDocument,
                                        QStringLiteral("Qt PDF could not render the page"));
    }
    return OperationResult::success();
#else
    Q_UNUSED(pageIndex)
    Q_UNUSED(targetSize)
    Q_UNUSED(output)
    return OperationResult::failure(EngineError::BackendUnavailable,
                                    QStringLiteral("Qt PDF support was disabled at build time."));
#endif
}

void QtPdfEngineAdapter::close() {
#ifdef VESTRA_ENABLE_QT_PDF
    if (document_ != nullptr) {
        document_->close();
    }
#endif
}

} // namespace vestra::document

