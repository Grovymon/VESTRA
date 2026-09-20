#pragma once

#include "vestra/document/pdf_engine.h"

#include <memory>

class QPdfDocument;

namespace vestra::document {

class QtPdfEngineAdapter final : public PdfEngine {
  public:
    QtPdfEngineAdapter();
    ~QtPdfEngineAdapter() override;

    [[nodiscard]] EngineInfo info() const override;
    OperationResult open(const QString& path) override;
    [[nodiscard]] int pageCount() const override;
    [[nodiscard]] QSizeF pageSizePoints(int pageIndex) const override;
    OperationResult renderPage(int pageIndex, const QSize& targetSize, QImage* output) override;
    void close() override;

  private:
#ifdef VESTRA_ENABLE_QT_PDF
    std::unique_ptr<QPdfDocument> document_;
#endif
};

} // namespace vestra::document

