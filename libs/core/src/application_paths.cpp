#include "vestra/core/application_paths.h"

#include <QDir>
#include <QStandardPaths>

namespace vestra::core {

namespace {
QString childPath(const QString& name) { return QDir(ApplicationPaths::root()).filePath(name); }
} // namespace

QString ApplicationPaths::root() {
#ifdef Q_OS_WIN
    QString localData = qEnvironmentVariable("LOCALAPPDATA");
    if (localData.isEmpty()) {
        localData = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    }
#else
    const QString localData =
        QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
#endif
    return QDir(localData).filePath(QStringLiteral("Vestra"));
}

QString ApplicationPaths::config() { return childPath(QStringLiteral("Config")); }

QString ApplicationPaths::cache() { return childPath(QStringLiteral("Cache")); }

QString ApplicationPaths::recovery() { return childPath(QStringLiteral("Recovery")); }

QString ApplicationPaths::logs() { return childPath(QStringLiteral("Logs")); }

QString ApplicationPaths::temp() { return childPath(QStringLiteral("Temp")); }

bool ApplicationPaths::ensureCreated(QString* error) {
    const QStringList paths{root(), config(), cache(), recovery(), logs(), temp()};
    for (const QString& path : paths) {
        if (!QDir().mkpath(path)) {
            if (error != nullptr) {
                *error = QStringLiteral("Could not create application directory: %1").arg(path);
            }
            return false;
        }
    }
    return true;
}

} // namespace vestra::core

