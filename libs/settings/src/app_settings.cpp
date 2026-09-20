#include "vestra/settings/app_settings.h"

#include "vestra/core/application_paths.h"

#include <QFileInfo>
#include <QLocale>
#include <QSettings>

namespace vestra::settings {
namespace {
QString themeName(const Theme theme) {
    switch (theme) {
    case Theme::Light:
        return QStringLiteral("light");
    case Theme::Dark:
        return QStringLiteral("dark");
    case Theme::System:
        return QStringLiteral("system");
    }
    return QStringLiteral("system");
}
} // namespace

AppSettings::AppSettings() {
    core::ApplicationPaths::ensureCreated();
    settings_ = std::make_unique<QSettings>(
        core::ApplicationPaths::config() + QStringLiteral("/vestra.ini"), QSettings::IniFormat);
}

AppSettings::~AppSettings() = default;

Theme AppSettings::theme() const {
    const QString value =
        settings_->value(QStringLiteral("appearance/theme"), QStringLiteral("system")).toString();
    if (value == QStringLiteral("light")) {
        return Theme::Light;
    }
    if (value == QStringLiteral("dark")) {
        return Theme::Dark;
    }
    return Theme::System;
}

void AppSettings::setTheme(const Theme theme) {
    settings_->setValue(QStringLiteral("appearance/theme"), themeName(theme));
}

QString AppSettings::language() const {
    const QString fallback = QLocale::system().language() == QLocale::Russian
                                 ? QStringLiteral("ru")
                                 : QStringLiteral("en");
    return settings_->value(QStringLiteral("general/language"), fallback).toString();
}

void AppSettings::setLanguage(const QString& language) {
    settings_->setValue(QStringLiteral("general/language"), language == QStringLiteral("ru")
                                                                ? QStringLiteral("ru")
                                                                : QStringLiteral("en"));
}

bool AppSettings::telemetryEnabled() const { return false; }

bool AppSettings::macrosEnabled() const { return false; }

bool AppSettings::protectedViewEnabled() const {
    return settings_->value(QStringLiteral("security/protectedView"), true).toBool();
}

void AppSettings::setProtectedViewEnabled(const bool enabled) {
    settings_->setValue(QStringLiteral("security/protectedView"), enabled);
}

bool AppSettings::autoRecoveryEnabled() const {
    return settings_->value(QStringLiteral("documents/autoRecovery"), true).toBool();
}

void AppSettings::setAutoRecoveryEnabled(const bool enabled) {
    settings_->setValue(QStringLiteral("documents/autoRecovery"), enabled);
}

QString AppSettings::externalLinksPolicy() const {
    return settings_->value(QStringLiteral("security/externalLinks"), QStringLiteral("ask"))
        .toString();
}

void AppSettings::setExternalLinksPolicy(const QString& policy) {
    settings_->setValue(QStringLiteral("security/externalLinks"), policy == QStringLiteral("block")
                                                                      ? QStringLiteral("block")
                                                                      : QStringLiteral("ask"));
}

bool AppSettings::updateChecksAllowed() const {
    return settings_->value(QStringLiteral("updates/checkAllowed"), false).toBool();
}

void AppSettings::setUpdateChecksAllowed(const bool allowed) {
    settings_->setValue(QStringLiteral("updates/checkAllowed"), allowed);
}

QStringList AppSettings::recentFiles() const {
    QStringList result;
    const QStringList stored = settings_->value(QStringLiteral("recent/files")).toStringList();
    for (const QString& path : stored) {
        if (QFileInfo::exists(path)) {
            result.append(QFileInfo(path).absoluteFilePath());
        }
    }
    return result;
}

void AppSettings::addRecentFile(const QString& path) {
    const QString absolutePath = QFileInfo(path).absoluteFilePath();
    QStringList files = recentFiles();
    files.removeAll(absolutePath);
    files.prepend(absolutePath);
    constexpr qsizetype maximumRecentFiles = 12;
    if (files.size() > maximumRecentFiles) {
        files = files.mid(0, maximumRecentFiles);
    }
    settings_->setValue(QStringLiteral("recent/files"), files);
}

void AppSettings::clearRecentFiles() { settings_->remove(QStringLiteral("recent/files")); }

void AppSettings::sync() { settings_->sync(); }

} // namespace vestra::settings

