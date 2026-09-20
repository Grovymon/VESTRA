#pragma once

#include <QString>

#include <utility>

namespace vestra::document {

enum class EngineError {
    None,
    BackendUnavailable,
    Unsupported,
    InvalidDocument,
    IoError,
    InternalError
};

struct EngineInfo final {
    QString name;
    bool compiledIn{false};
    bool runtimeReady{false};
    QString detail;
};

struct OperationResult final {
    EngineError error{EngineError::None};
    QString detail;

    [[nodiscard]] bool ok() const { return error == EngineError::None; }
    static OperationResult success() { return {}; }
    static OperationResult failure(EngineError error, QString detail) {
        return {error, std::move(detail)};
    }
};

} // namespace vestra::document

