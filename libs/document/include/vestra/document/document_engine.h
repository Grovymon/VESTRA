#pragma once

#include "vestra/document/engine_types.h"

#include <QString>

namespace vestra::document {

class DocumentEngine {
  public:
    virtual ~DocumentEngine() = default;
    [[nodiscard]] virtual EngineInfo info() const = 0;
    virtual OperationResult open(const QString& path, bool readOnly) = 0;
    virtual OperationResult saveAs(const QString& path, const QString& format) = 0;
    virtual OperationResult exportPdf(const QString& path) = 0;
    virtual void close() = 0;
};

} // namespace vestra::document

