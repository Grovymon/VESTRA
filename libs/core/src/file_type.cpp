#include "vestra/core/file_type.h"

#include <QFileInfo>
#include <QHash>

namespace vestra::core {

FileType detectFileType(const QString& path) {
    const QString extension = QFileInfo(path).suffix().toLower();

    static const QHash<QString, FileType> types{
        {QStringLiteral("docx"), {FileKind::Writer, extension, QStringLiteral("DOCX"), false}},
        {QStringLiteral("docm"), {FileKind::Writer, extension, QStringLiteral("DOCM"), true}},
        {QStringLiteral("doc"), {FileKind::Writer, extension, QStringLiteral("DOC"), true}},
        {QStringLiteral("odt"), {FileKind::Writer, extension, QStringLiteral("ODT"), false}},
        {QStringLiteral("rtf"), {FileKind::Writer, extension, QStringLiteral("RTF"), false}},
        {QStringLiteral("txt"), {FileKind::Text, extension, QStringLiteral("Text"), false}},
        {QStringLiteral("xlsx"), {FileKind::Sheets, extension, QStringLiteral("XLSX"), false}},
        {QStringLiteral("xlsm"), {FileKind::Sheets, extension, QStringLiteral("XLSM"), true}},
        {QStringLiteral("xls"), {FileKind::Sheets, extension, QStringLiteral("XLS"), true}},
        {QStringLiteral("ods"), {FileKind::Sheets, extension, QStringLiteral("ODS"), false}},
        {QStringLiteral("csv"), {FileKind::Sheets, extension, QStringLiteral("CSV"), false}},
        {QStringLiteral("vestra-sheets"),
         {FileKind::Sheets, extension, QStringLiteral("Vestra Sheets"), false}},
        {QStringLiteral("pptx"), {FileKind::Slides, extension, QStringLiteral("PPTX"), false}},
        {QStringLiteral("pptm"), {FileKind::Slides, extension, QStringLiteral("PPTM"), true}},
        {QStringLiteral("ppt"), {FileKind::Slides, extension, QStringLiteral("PPT"), true}},
        {QStringLiteral("odp"), {FileKind::Slides, extension, QStringLiteral("ODP"), false}},
        {QStringLiteral("vestra-slides"),
         {FileKind::Slides, extension, QStringLiteral("Vestra Slides"), false}},
        {QStringLiteral("pdf"), {FileKind::Pdf, extension, QStringLiteral("PDF"), false}},
    };

    if (const auto iterator = types.constFind(extension); iterator != types.cend()) {
        FileType result = iterator.value();
        result.extension = extension;
        return result;
    }
    return {FileKind::Unknown, extension, QStringLiteral("Unknown"), false};
}

QString fileKindName(const FileKind kind) {
    switch (kind) {
    case FileKind::Writer:
        return QStringLiteral("writer");
    case FileKind::Sheets:
        return QStringLiteral("sheets");
    case FileKind::Slides:
        return QStringLiteral("slides");
    case FileKind::Pdf:
        return QStringLiteral("pdf");
    case FileKind::Text:
        return QStringLiteral("text");
    case FileKind::Unknown:
        return QStringLiteral("unknown");
    }
    return QStringLiteral("unknown");
}

} // namespace vestra::core

