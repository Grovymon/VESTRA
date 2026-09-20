#pragma once

#include "vestra/document/engine_types.h"

#include <QImage>
#include <QSize>
#include <QSizeF>
#include <QString>

namespace vestra::document {

class PdfEngine {
  public:
    virtual ~PdfEngine() = default;
    [[nodiscard]] virtual EngineInfo info() const = 0;
    virtual OperationResult open(const QString& path) = 0;
    [[nodiscard]] virtual int pageCount() const = 0;
    [[nodiscard]] virtual QSizeF pageSizePoints(int pageIndex) const = 0;
    virtual OperationResult renderPage(int pageIndex, const QSize& targetSize, QImage* output) = 0;
    virtual void close() = 0;
};

} // namespace vestra::document

