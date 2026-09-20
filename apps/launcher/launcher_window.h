#pragma once

#include "vestra/settings/app_settings.h"

#include <QMainWindow>

class QLabel;
class QListWidget;
class QPushButton;

namespace vestra::sandbox {
class WorkerSession;
}

class LauncherWindow final : public QMainWindow {
    Q_OBJECT

  public:
    explicit LauncherWindow(QWidget* parent = nullptr);

  private:
    QPushButton* createActionButton(const QString& objectName);
    void buildUi();
    void retranslate();
    void refreshRecentFiles();
    void chooseFile(bool pdfOnly);
    void inspectAndOpen(const QString& path);
    void openAfterInspection(const QString& path, bool protectedView, bool macros,
                             const QStringList& findings);
    void launchApplication(const QString& executableName, const QStringList& arguments = {});
    [[nodiscard]] bool sourceRequiresProtectedView(const QString& path) const;
    void setBusy(bool busy);

    vestra::settings::AppSettings settings_;
    vestra::sandbox::WorkerSession* worker_{nullptr};
    QLabel* title_{nullptr};
    QLabel* tagline_{nullptr};
    QLabel* about_{nullptr};
    QLabel* recentTitle_{nullptr};
    QLabel* status_{nullptr};
    QListWidget* recentFiles_{nullptr};
    QPushButton* newDocument_{nullptr};
    QPushButton* newSheet_{nullptr};
    QPushButton* newPresentation_{nullptr};
    QPushButton* openPdf_{nullptr};
    QPushButton* openFile_{nullptr};
    QPushButton* settingsButton_{nullptr};
};

