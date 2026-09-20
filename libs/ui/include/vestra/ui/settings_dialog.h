#pragma once

#include <QDialog>

class QCheckBox;
class QComboBox;
class QListWidget;
class QPushButton;
class QStackedWidget;

namespace vestra::settings {
class AppSettings;
}

namespace vestra::ui {

class SettingsDialog final : public QDialog {
    Q_OBJECT

  public:
    explicit SettingsDialog(settings::AppSettings& settings, QWidget* parent = nullptr);

  signals:
    void settingsApplied();
    void recentFilesCleared();

  private:
    QWidget* createGeneralPage();
    QWidget* createAppearancePage();
    QWidget* createDocumentsPage();
    QWidget* createSecurityPage();
    QWidget* createUpdatesPage();
    QWidget* createAdvancedPage();
    void retranslate();
    void save();

    settings::AppSettings& settings_;
    QListWidget* categories_{nullptr};
    QStackedWidget* pages_{nullptr};
    QComboBox* language_{nullptr};
    QComboBox* theme_{nullptr};
    QCheckBox* recovery_{nullptr};
    QCheckBox* protectedView_{nullptr};
    QComboBox* externalLinks_{nullptr};
    QCheckBox* updates_{nullptr};
    QCheckBox* telemetry_{nullptr};
    QCheckBox* macros_{nullptr};
    QPushButton* clearRecent_{nullptr};
    QPushButton* openLogs_{nullptr};
    QPushButton* saveButton_{nullptr};
    QPushButton* cancelButton_{nullptr};
};

} // namespace vestra::ui

