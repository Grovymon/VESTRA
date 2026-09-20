#include "vestra/logging/logger.h"

#include "vestra/core/application_paths.h"

#include <QDate>
#include <QDateTime>
#include <QFile>
#include <QMutex>
#include <QRegularExpression>
#include <QTextStream>

namespace vestra::logging {
namespace {
QMutex logMutex;
QString logPath;
QtMessageHandler previousHandler = nullptr;

QString levelName(const QtMsgType type) {
    switch (type) {
    case QtDebugMsg:
        return QStringLiteral("DEBUG");
    case QtInfoMsg:
        return QStringLiteral("INFO");
    case QtWarningMsg:
        return QStringLiteral("WARN");
    case QtCriticalMsg:
        return QStringLiteral("ERROR");
    case QtFatalMsg:
        return QStringLiteral("FATAL");
    }
    return QStringLiteral("UNKNOWN");
}

void messageHandler(const QtMsgType type, const QMessageLogContext& context,
                    const QString& message) {
    const QMutexLocker locker(&logMutex);
    QFile file(logPath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream stream(&file);
        stream << QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs) << ' '
               << levelName(type) << ' ' << message;
        if (context.file != nullptr) {
            stream << " (" << context.file << ':' << context.line << ')';
        }
        stream << '\n';
    }
    if (previousHandler != nullptr) {
        previousHandler(type, context, message);
    }
}
} // namespace

bool Logger::install(const QString& processName, QString* error) {
    if (!core::ApplicationPaths::ensureCreated(error)) {
        return false;
    }
    const QString safeName =
        QString(processName)
            .replace(QRegularExpression(QStringLiteral("[^a-zA-Z0-9_-]")), QStringLiteral("_"));
    logPath = core::ApplicationPaths::logs() + QLatin1Char('/') + safeName + QLatin1Char('-') +
              QDate::currentDate().toString(Qt::ISODate) + QStringLiteral(".log");
    previousHandler = qInstallMessageHandler(messageHandler);
    qInfo().noquote() << "Logging started. Logs remain local:" << logPath;
    return true;
}

void Logger::uninstall() {
    qInstallMessageHandler(previousHandler);
    previousHandler = nullptr;
}

QString Logger::currentLogFile() { return logPath; }

} // namespace vestra::logging

