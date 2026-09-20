#pragma once

#include <QString>
#include <QStringList>

#include <memory>

class QSettings;

namespace vestra::settings {

enum class Theme { System, Light, Dark };

class AppSettings final {
  public:
    AppSettings();
    ~AppSettings();
    AppSettings(const AppSettings&) = delete;
    AppSettings& operator=(const AppSettings&) = delete;

    [[nodiscard]] Theme theme() const;
    void setTheme(Theme theme);
    [[nodiscard]] QString language() const;
    void setLanguage(const QString& language);

    [[nodiscard]] bool telemetryEnabled() const;
    [[nodiscard]] bool macrosEnabled() const;
    [[nodiscard]] bool protectedViewEnabled() const;
    void setProtectedViewEnabled(bool enabled);
    [[nodiscard]] bool autoRecoveryEnabled() const;
    void setAutoRecoveryEnabled(bool enabled);
    [[nodiscard]] QString externalLinksPolicy() const;
    void setExternalLinksPolicy(const QString& policy);
    [[nodiscard]] bool updateChecksAllowed() const;
    void setUpdateChecksAllowed(bool allowed);

    [[nodiscard]] QStringList recentFiles() const;
    void addRecentFile(const QString& path);
    void clearRecentFiles();
    void sync();

  private:
    std::unique_ptr<QSettings> settings_;
};

} // namespace vestra::settings

