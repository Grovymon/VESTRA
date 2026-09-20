#pragma once

#include <QString>

namespace vestra::core {

enum class FileKind { Unknown, Writer, Sheets, Slides, Pdf, Text };

struct FileType final {
    FileKind kind{FileKind::Unknown};
    QString extension;
    QString displayName;
    bool mayContainMacros{false};
};

[[nodiscard]] FileType detectFileType(const QString& path);
[[nodiscard]] QString fileKindName(FileKind kind);

} // namespace vestra::core

