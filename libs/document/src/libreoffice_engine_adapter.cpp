#include "vestra/document/libreoffice_engine_adapter.h"

namespace vestra::document {
namespace {
OperationResult unavailable() {
#ifdef VESTRA_ENABLE_LIBREOFFICE
    return OperationResult::failure(
        EngineError::Unsupported, QStringLiteral("LibreOfficeKit SDK is linked, but tiled document "
                                                 "integration is not implemented in Vestra 0.1."));
#else
    return OperationResult::failure(
        EngineError::BackendUnavailable,
        QStringLiteral("LibreOfficeKit support was disabled at build time."));
#endif
}
} // namespace

EngineInfo LibreOfficeEngineAdapter::info() const {
#ifdef VESTRA_ENABLE_LIBREOFFICE
    return {QStringLiteral("LibreOfficeKit"), true, false,
            QStringLiteral("SDK linked; tiled rendering bridge is not implemented yet")};
#else
    return {QStringLiteral("LibreOfficeKit"), false, false,
            QStringLiteral("Configure with VESTRA_ENABLE_LIBREOFFICE=ON")};
#endif
}

OperationResult LibreOfficeEngineAdapter::open(const QString&, const bool) { return unavailable(); }

OperationResult LibreOfficeEngineAdapter::saveAs(const QString&, const QString&) {
    return unavailable();
}

OperationResult LibreOfficeEngineAdapter::exportPdf(const QString&) { return unavailable(); }

void LibreOfficeEngineAdapter::close() {}

} // namespace vestra::document

