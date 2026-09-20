#include "vestra/security/document_scanner.h"

#include "vestra/core/file_type.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QtEndian>

#include <algorithm>
#include <cstring>
#include <utility>

namespace vestra::security {
namespace {
constexpr quint32 kEndOfCentralDirectorySignature = 0x06054b50U;
constexpr quint32 kCentralDirectorySignature = 0x02014b50U;

quint16 read16(const QByteArray& data, const qsizetype offset) {
    quint16 value = 0;
    std::memcpy(&value, data.constData() + offset, sizeof(value));
    return qFromLittleEndian(value);
}

quint32 read32(const QByteArray& data, const qsizetype offset) {
    quint32 value = 0;
    std::memcpy(&value, data.constData() + offset, sizeof(value));
    return qFromLittleEndian(value);
}

bool isOfficeZip(const QString& extension) {
    static const QStringList extensions{
        QStringLiteral("docx"), QStringLiteral("docm"), QStringLiteral("xlsx"),
        QStringLiteral("xlsm"), QStringLiteral("pptx"), QStringLiteral("pptm"),
        QStringLiteral("odt"),  QStringLiteral("ods"),  QStringLiteral("odp")};
    return extensions.contains(extension);
}

bool isOoxml(const QString& extension) {
    static const QStringList extensions{QStringLiteral("docx"), QStringLiteral("docm"),
                                        QStringLiteral("xlsx"), QStringLiteral("xlsm"),
                                        QStringLiteral("pptx"), QStringLiteral("pptm")};
    return extensions.contains(extension);
}

bool isUntrustedLocation(const QString& path) {
    const QString canonical = QFileInfo(path).absoluteFilePath();
    const QString downloads = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    const QString temporary = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    const auto inside = [&canonical](const QString& root) {
        if (root.isEmpty()) {
            return false;
        }
        const QString prefix = QDir(root).absolutePath() + QDir::separator();
        return canonical.startsWith(prefix, Qt::CaseInsensitive);
    };
    if (inside(downloads) || inside(temporary)) {
        return true;
    }
#ifdef Q_OS_WIN
    QFile zone(canonical + QStringLiteral(":Zone.Identifier"));
    return zone.exists();
#else
    return false;
#endif
}

void requireProtectedView(ScanResult* result, const QString& finding) {
    if (result->disposition == Disposition::Allow) {
        result->disposition = Disposition::ProtectedView;
        result->code = QStringLiteral("protected_view_required");
    }
    result->findings.append(finding);
}

ScanResult block(QString code, QString finding) {
    ScanResult result;
    result.disposition = Disposition::Block;
    result.code = std::move(code);
    result.findings.append(std::move(finding));
    return result;
}

ScanResult inspectZip(QFile* file, const ScanLimits& limits, const bool requireContentTypes) {
    ScanResult result;
    constexpr qint64 maximumTailBytes = 65557;
    const qint64 tailSize = std::min(file->size(), maximumTailBytes);
    if (!file->seek(file->size() - tailSize)) {
        return block(QStringLiteral("zip_read_failed"), QStringLiteral("zip.read_failed"));
    }
    const QByteArray tail = file->read(tailSize);

    qsizetype eocd = -1;
    for (qsizetype index = tail.size() - 22; index >= 0; --index) {
        if (read32(tail, index) == kEndOfCentralDirectorySignature) {
            eocd = index;
            break;
        }
    }
    if (eocd < 0 || eocd + 22 > tail.size()) {
        return block(QStringLiteral("invalid_ooxml_zip"),
                     QStringLiteral("zip.central_directory_missing"));
    }

    const quint16 diskNumber = read16(tail, eocd + 4);
    const quint16 centralDisk = read16(tail, eocd + 6);
    const quint16 entries = read16(tail, eocd + 10);
    const quint32 centralSize = read32(tail, eocd + 12);
    const quint32 centralOffset = read32(tail, eocd + 16);
    if (diskNumber != 0 || centralDisk != 0 || entries == 0xffffU || centralSize == 0xffffffffU ||
        centralOffset == 0xffffffffU) {
        return block(QStringLiteral("unsupported_zip"), QStringLiteral("zip.multi_disk_or_zip64"));
    }
    if (entries > limits.maximumEntries) {
        return block(QStringLiteral("too_many_entries"), QStringLiteral("zip.too_many_entries"));
    }
    if (static_cast<quint64>(centralOffset) + centralSize > static_cast<quint64>(file->size()) ||
        !file->seek(centralOffset)) {
        return block(QStringLiteral("invalid_ooxml_zip"),
                     QStringLiteral("zip.invalid_directory_bounds"));
    }
    const QByteArray central = file->read(centralSize);
    if (central.size() != static_cast<qsizetype>(centralSize)) {
        return block(QStringLiteral("zip_read_failed"), QStringLiteral("zip.read_failed"));
    }

    quint64 totalUncompressed = 0;
    qsizetype offset = 0;
    bool hasContentTypes = false;
    for (quint32 index = 0; index < entries; ++index) {
        if (offset + 46 > central.size() || read32(central, offset) != kCentralDirectorySignature) {
            return block(QStringLiteral("invalid_ooxml_zip"), QStringLiteral("zip.invalid_entry"));
        }
        const quint32 compressed = read32(central, offset + 20);
        const quint32 uncompressed = read32(central, offset + 24);
        const quint16 nameLength = read16(central, offset + 28);
        const quint16 extraLength = read16(central, offset + 30);
        const quint16 commentLength = read16(central, offset + 32);
        const qsizetype next = offset + 46 + nameLength + extraLength + commentLength;
        if (nameLength == 0 || nameLength > 4096 || next > central.size()) {
            return block(QStringLiteral("invalid_ooxml_zip"), QStringLiteral("zip.invalid_name"));
        }

        const QString name = QString::fromUtf8(central.mid(offset + 46, nameLength));
        const QString normalized =
            QDir::cleanPath(name).replace(QLatin1Char('\\'), QLatin1Char('/'));
        if (name.startsWith(QLatin1Char('/')) || name.startsWith(QLatin1Char('\\')) ||
            (normalized.size() > 1 && normalized.at(1) == QLatin1Char(':')) ||
            normalized == QStringLiteral("..") || normalized.startsWith(QStringLiteral("../")) ||
            name.contains(QStringLiteral("/../")) || name.contains(QStringLiteral("\\..\\"))) {
            return block(QStringLiteral("path_traversal"), QStringLiteral("zip.path_traversal"));
        }
        if (uncompressed > limits.maximumEntryBytes) {
            return block(QStringLiteral("entry_too_large"), QStringLiteral("zip.entry_too_large"));
        }
        if (normalized.count(QLatin1Char('/')) + 1 > static_cast<int>(limits.maximumPathDepth)) {
            return block(QStringLiteral("nesting_too_deep"),
                         QStringLiteral("zip.path_depth_limit"));
        }
        totalUncompressed += uncompressed;
        if (totalUncompressed > limits.maximumUncompressedBytes) {
            return block(QStringLiteral("decompression_bomb"),
                         QStringLiteral("zip.expanded_size_limit"));
        }
        if (compressed == 0U && uncompressed > 0U) {
            return block(QStringLiteral("decompression_bomb"),
                         QStringLiteral("zip.invalid_compression_ratio"));
        }
        if (compressed > 0U && static_cast<double>(uncompressed) / static_cast<double>(compressed) >
                                   limits.maximumCompressionRatio) {
            return block(QStringLiteral("decompression_bomb"),
                         QStringLiteral("zip.compression_ratio_limit"));
        }

        const QString lower = normalized.toLower();
        hasContentTypes = hasContentTypes || lower == QStringLiteral("[content_types].xml");
        if ((lower.endsWith(QStringLiteral(".xml")) || lower.endsWith(QStringLiteral(".rels"))) &&
            uncompressed > limits.maximumXmlBytes) {
            return block(QStringLiteral("xml_too_large"),
                         QStringLiteral("document.xml_size_limit"));
        }
        static const QStringList imageSuffixes{
            QStringLiteral(".png"),  QStringLiteral(".jpg"), QStringLiteral(".jpeg"),
            QStringLiteral(".gif"),  QStringLiteral(".bmp"), QStringLiteral(".tif"),
            QStringLiteral(".tiff"), QStringLiteral(".wmf"), QStringLiteral(".emf")};
        for (const QString& suffix : imageSuffixes) {
            if (lower.endsWith(suffix) && uncompressed > limits.maximumImageBytes) {
                return block(QStringLiteral("image_too_large"),
                             QStringLiteral("document.image_size_limit"));
            }
        }
        if (lower.endsWith(QStringLiteral("vbaproject.bin"))) {
            result.macrosDetected = true;
            requireProtectedView(&result, QStringLiteral("document.macros_detected"));
        }
        if (lower.contains(QStringLiteral("externallinks/"))) {
            result.externalRelationshipsDetected = true;
        }
        if (lower.contains(QStringLiteral("embeddings/")) ||
            lower.contains(QStringLiteral("oleobject"))) {
            requireProtectedView(&result, QStringLiteral("document.embedded_object"));
        }
        static const QStringList executableSuffixes{
            QStringLiteral(".exe"), QStringLiteral(".dll"), QStringLiteral(".com"),
            QStringLiteral(".bat"), QStringLiteral(".cmd"), QStringLiteral(".ps1"),
            QStringLiteral(".js"),  QStringLiteral(".vbs"), QStringLiteral(".msi")};
        for (const QString& suffix : executableSuffixes) {
            if (lower.endsWith(suffix)) {
                return block(QStringLiteral("embedded_executable"),
                             QStringLiteral("document.embedded_executable"));
            }
        }
        offset = next;
    }

    if (requireContentTypes && !hasContentTypes) {
        return block(QStringLiteral("invalid_ooxml_zip"),
                     QStringLiteral("zip.content_types_missing"));
    }

    if (result.externalRelationshipsDetected) {
        requireProtectedView(&result, QStringLiteral("document.external_relationships"));
    }
    return result;
}
} // namespace

DocumentScanner::DocumentScanner(ScanLimits limits) : limits_(limits) {}

ScanResult DocumentScanner::scan(const QString& path, const bool considerSourceLocation) const {
    const QFileInfo info(path);
    if (!info.exists() || !info.isFile()) {
        return block(QStringLiteral("file_not_found"), QStringLiteral("file.not_found"));
    }
    if (info.size() < 0 || info.size() > limits_.maximumFileBytes) {
        return block(QStringLiteral("file_too_large"), QStringLiteral("file.size_limit"));
    }

    const core::FileType type = core::detectFileType(path);
    if (type.kind == core::FileKind::Unknown) {
        return block(QStringLiteral("unsupported_type"), QStringLiteral("file.unsupported_type"));
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return block(QStringLiteral("file_unreadable"), QStringLiteral("file.unreadable"));
    }

    ScanResult result;
    if (type.kind == core::FileKind::Pdf) {
        if (file.read(5) != QByteArrayLiteral("%PDF-")) {
            return block(QStringLiteral("invalid_pdf"), QStringLiteral("pdf.invalid_signature"));
        }
        file.seek(0);
    }
    if (isOfficeZip(type.extension)) {
        if (file.read(4) != QByteArray::fromHex("504b0304")) {
            return block(QStringLiteral("invalid_ooxml_zip"),
                         QStringLiteral("zip.invalid_signature"));
        }
        result = inspectZip(&file, limits_, isOoxml(type.extension));
        if (result.disposition == Disposition::Block) {
            return result;
        }
    }

    if (type.mayContainMacros && !result.macrosDetected) {
        result.macrosDetected = true;
        requireProtectedView(&result, QStringLiteral("document.macro_capable_format"));
    }
    if (considerSourceLocation && isUntrustedLocation(path)) {
        requireProtectedView(&result, QStringLiteral("file.untrusted_location"));
    }
    return result;
}

} // namespace vestra::security

