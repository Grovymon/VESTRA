#pragma once

#include <QString>
#include <QStringList>

namespace vestra::security {

enum class Disposition { Allow, ProtectedView, Block };

struct ScanLimits final {
    qint64 maximumFileBytes{256LL * 1024LL * 1024LL};
    quint64 maximumUncompressedBytes{1024ULL * 1024ULL * 1024ULL};
    quint64 maximumEntryBytes{256ULL * 1024ULL * 1024ULL};
    quint64 maximumXmlBytes{64ULL * 1024ULL * 1024ULL};
    quint64 maximumImageBytes{128ULL * 1024ULL * 1024ULL};
    quint32 maximumEntries{10000U};
    quint32 maximumPathDepth{32U};
    double maximumCompressionRatio{200.0};
};

struct ScanResult final {
    Disposition disposition{Disposition::Allow};
    QString code{QStringLiteral("ok")};
    QStringList findings;
    bool macrosDetected{false};
    bool externalRelationshipsDetected{false};
};

class DocumentScanner final {
  public:
    explicit DocumentScanner(ScanLimits limits = {});
    [[nodiscard]] ScanResult scan(const QString& path, bool considerSourceLocation = true) const;

  private:
    ScanLimits limits_;
};

} // namespace vestra::security

