#pragma once

#include "vestra/document/document_engine.h"

namespace vestra::document {

class LibreOfficeEngineAdapter final : public DocumentEngine {
  public:
    EngineInfo info() const override;
    OperationResult open(const QString& path, bool readOnly) override;
    OperationResult saveAs(const QString& path, const QString& format) override;
    OperationResult exportPdf(const QString& path) override;
    void close() override;
};

} // namespace vestra::document

