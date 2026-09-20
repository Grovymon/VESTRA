#pragma once

#include "vestra/document/pdf_engine.h"

namespace vestra::document {

class PdfiumEngineAdapter final : public PdfEngine {
  public:
    PdfiumEngineAdapter();
    ~PdfiumEngineAdapter() override;
    PdfiumEngineAdapter(const PdfiumEngineAdapter&) = delete;
    PdfiumEngineAdapter& operator=(const PdfiumEngineAdapter&) = delete;

    EngineInfo info() const override;
    OperationResult open(const QString& path) override;
    [[nodiscard]] int pageCount() const override;
    [[nodiscard]] QSizeF pageSizePoints(int pageIndex) const override;
    OperationResult renderPage(int pageIndex, const QSize& targetSize, QImage* output) override;
    void close() override;

  private:
    void* document_{nullptr};
};

} // namespace vestra::document

