#include "launcher_window.h"

#include "vestra/core/file_type.h"
#include "vestra/sandbox/worker_session.h"
#include "vestra/ui/settings_dialog.h"
#include "vestra/ui/translation_manager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QStandardPaths>
#include <QVBoxLayout>

using vestra::ui::t;

LauncherWindow::LauncherWindow(QWidget* parent) : QMainWindow(parent) {
    worker_ = new vestra::sandbox::WorkerSession(this);
    buildUi();
    retranslate();
    refreshRecentFiles();

    connect(&vestra::ui::TranslationManager::instance(),
            &vestra::ui::TranslationManager::languageChanged, this, &LauncherWindow::retranslate);
    connect(worker_, &vestra::sandbox::WorkerSession::accepted, this,
            [this](const QString& path, const bool protectedView, const bool macros,
                   const QStringList& findings) {
                setBusy(false);
                openAfterInspection(path, protectedView, macros, findings);
            });
    connect(worker_, &vestra::sandbox::WorkerSession::rejected, this,
            [this](const QString&, const QString& code, const QStringList& findings) {
                setBusy(false);
                QMessageBox::critical(this, t("common.error"),
                                      t("launcher.blocked") + QStringLiteral("\n\n") + code +
                                          QStringLiteral("\n") + findings.join(QLatin1Char('\n')));
            });
    connect(worker_, &vestra::sandbox::WorkerSession::failed, this,
            [this](const QString&, const QString& detail) {
                setBusy(false);
                QMessageBox::critical(this, t("common.error"),
                                      t("launcher.worker_failed") + QStringLiteral("\n\n") +
                                          detail);
            });
}

QPushButton* LauncherWindow::createActionButton(const QString& objectName) {
    auto* button = new QPushButton;
    button->setObjectName(objectName);
    button->setProperty("moduleCard", true);
    button->setMinimumSize(235, 112);
    button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    button->setCursor(Qt::PointingHandCursor);
    button->setStyleSheet(QStringLiteral(
        "QPushButton{font-size:16px;font-weight:600;text-align:left;padding:18px 20px;"
        "border-radius:12px;}"));
    return button;
}

void LauncherWindow::buildUi() {
    resize(1120, 720);
    setMinimumSize(820, 580);
    auto* central = new QWidget;
    auto* root = new QHBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto* sidebar = new QFrame;
    sidebar->setFixedWidth(232);
    sidebar->setStyleSheet(
        QStringLiteral("QFrame{background:#173a72;color:white;}"
                       "QPushButton{color:white;background:transparent;border:0;text-align:left;"
                       "padding:11px 14px;}"
                       "QPushButton:hover{background:#28508c;} QLabel{color:white;}"));
    auto* side = new QVBoxLayout(sidebar);
    side->setContentsMargins(20, 24, 20, 18);
    side->setSpacing(10);
    auto* brand = new QLabel(QStringLiteral("V"));
    brand->setAlignment(Qt::AlignCenter);
    brand->setFixedSize(44, 44);
    brand->setStyleSheet(QStringLiteral(
        "background:white;color:#173a72;font-size:25px;font-weight:800;border-radius:9px;"));
    auto* brandName = new QLabel(QStringLiteral("Vestra"));
    brandName->setStyleSheet(QStringLiteral("font-size:22px;font-weight:700;"));
    side->addWidget(brand);
    side->addWidget(brandName);
    side->addSpacing(22);
    openFile_ = new QPushButton;
    settingsButton_ = new QPushButton;
    side->addWidget(openFile_);
    side->addWidget(settingsButton_);
    side->addStretch();
    status_ = new QLabel;
    status_->setWordWrap(true);
    status_->setStyleSheet(QStringLiteral("font-size:11px;color:#d6e3fb;"));
    about_ = new QLabel;
    about_->setWordWrap(true);
    about_->setStyleSheet(QStringLiteral("font-size:11px;color:#a9bfdf;"));
    side->addWidget(status_);
    side->addSpacing(8);
    side->addWidget(about_);
    root->addWidget(sidebar);

    auto* content = new QWidget;
    auto* page = new QVBoxLayout(content);
    page->setContentsMargins(38, 30, 38, 28);
    page->setSpacing(15);
    title_ = new QLabel;
    title_->setObjectName(QStringLiteral("LauncherTitle"));
    title_->setStyleSheet(QStringLiteral("font-size:32px;font-weight:700;"));
    tagline_ = new QLabel;
    tagline_->setObjectName(QStringLiteral("LauncherTagline"));
    tagline_->setStyleSheet(QStringLiteral("font-size:14px;"));
    page->addWidget(title_);
    page->addWidget(tagline_);
    page->addSpacing(8);

    auto* actions = new QGridLayout;
    actions->setSpacing(14);
    newDocument_ = createActionButton(QStringLiteral("newDocument"));
    newSheet_ = createActionButton(QStringLiteral("newSheet"));
    newPresentation_ = createActionButton(QStringLiteral("newPresentation"));
    openPdf_ = createActionButton(QStringLiteral("openPdf"));
    newDocument_->setStyleSheet(newDocument_->styleSheet() +
                                QStringLiteral("QPushButton{border-left:6px solid #2b62b5;}"));
    newSheet_->setStyleSheet(newSheet_->styleSheet() +
                             QStringLiteral("QPushButton{border-left:6px solid #16834b;}"));
    newPresentation_->setStyleSheet(newPresentation_->styleSheet() +
                                    QStringLiteral("QPushButton{border-left:6px solid #d45124;}"));
    openPdf_->setStyleSheet(openPdf_->styleSheet() +
                            QStringLiteral("QPushButton{border-left:6px solid #b33c48;}"));
    actions->addWidget(newDocument_, 0, 0);
    actions->addWidget(newSheet_, 0, 1);
    actions->addWidget(newPresentation_, 1, 0);
    actions->addWidget(openPdf_, 1, 1);
    actions->setColumnStretch(0, 1);
    actions->setColumnStretch(1, 1);
    page->addLayout(actions);

    recentTitle_ = new QLabel;
    recentTitle_->setStyleSheet(QStringLiteral("font-size:18px;font-weight:600;margin-top:10px;"));
    page->addWidget(recentTitle_);
    recentFiles_ = new QListWidget;
    recentFiles_->setAlternatingRowColors(true);
    recentFiles_->setMinimumHeight(130);
    page->addWidget(recentFiles_, 1);
    root->addWidget(content, 1);
    setCentralWidget(central);

    connect(newDocument_, &QPushButton::clicked, this,
            [this] { launchApplication(QStringLiteral("vestra-writer")); });
    connect(newSheet_, &QPushButton::clicked, this,
            [this] { launchApplication(QStringLiteral("vestra-sheets")); });
    connect(newPresentation_, &QPushButton::clicked, this,
            [this] { launchApplication(QStringLiteral("vestra-slides")); });
    connect(openPdf_, &QPushButton::clicked, this, [this] { chooseFile(true); });
    connect(openFile_, &QPushButton::clicked, this, [this] { chooseFile(false); });
    connect(recentFiles_, &QListWidget::itemActivated, this, [this](QListWidgetItem* item) {
        const QString path = item->data(Qt::UserRole).toString();
        if (!path.isEmpty()) {
            inspectAndOpen(path);
        }
    });
    connect(settingsButton_, &QPushButton::clicked, this, [this] {
        vestra::ui::SettingsDialog dialog(settings_, this);
        connect(&dialog, &vestra::ui::SettingsDialog::recentFilesCleared, this,
                &LauncherWindow::refreshRecentFiles);
        connect(&dialog, &vestra::ui::SettingsDialog::settingsApplied, this,
                &LauncherWindow::refreshRecentFiles);
        dialog.exec();
    });
}

void LauncherWindow::retranslate() {
    setWindowTitle(t("app.name"));
    title_->setText(t("app.name"));
    tagline_->setText(t("app.tagline"));
    about_->setText(t("launcher.about"));
    recentTitle_->setText(t("launcher.recent"));
    status_->setText(t("launcher.ready"));
    settingsButton_->setText(t("launcher.settings"));
    newDocument_->setText(t("launcher.new_document"));
    newSheet_->setText(t("launcher.new_sheet"));
    newPresentation_->setText(t("launcher.new_presentation"));
    openPdf_->setText(t("launcher.open_pdf"));
    openFile_->setText(t("launcher.open_file"));
    refreshRecentFiles();
}

void LauncherWindow::refreshRecentFiles() {
    recentFiles_->clear();
    const QStringList files = settings_.recentFiles();
    if (files.isEmpty()) {
        auto* item = new QListWidgetItem(t("launcher.no_recent"));
        item->setFlags(Qt::NoItemFlags);
        recentFiles_->addItem(item);
        return;
    }
    for (const QString& path : files) {
        auto* item = new QListWidgetItem(QFileInfo(path).fileName());
        item->setToolTip(path);
        item->setData(Qt::UserRole, path);
        recentFiles_->addItem(item);
    }
}

void LauncherWindow::chooseFile(const bool pdfOnly) {
    const QString path = QFileDialog::getOpenFileName(
        this, pdfOnly ? t("launcher.open_pdf") : t("launcher.open_file"), QString(),
        pdfOnly ? t("launcher.pdf_filter") : t("launcher.file_filter"));
    if (!path.isEmpty()) {
        inspectAndOpen(path);
    }
}

void LauncherWindow::inspectAndOpen(const QString& path) {
    if (vestra::core::detectFileType(path).kind == vestra::core::FileKind::Unknown) {
        QMessageBox::warning(this, t("common.error"), t("launcher.unsupported"));
        return;
    }
    QString error;
    if (!worker_->inspect(path, &error)) {
        QMessageBox::critical(this, t("common.error"),
                              t("launcher.worker_failed") + QStringLiteral("\n\n") + error);
        return;
    }
    setBusy(true);
}

void LauncherWindow::openAfterInspection(const QString& path, const bool workerProtectedView,
                                         const bool macros, const QStringList&) {
    const vestra::core::FileType type = vestra::core::detectFileType(path);
    if (macros) {
        QMessageBox::warning(this, t("macro.title"), t("macro.message"));
    }
    const bool protectedView =
        macros || workerProtectedView ||
        (settings_.protectedViewEnabled() && sourceRequiresProtectedView(path));
    settings_.addRecentFile(path);
    settings_.sync();
    refreshRecentFiles();

    QStringList arguments{QStringLiteral("--file"), QFileInfo(path).absoluteFilePath()};
    if (protectedView) {
        arguments.append(QStringLiteral("--protected-view"));
    }
    switch (type.kind) {
    case vestra::core::FileKind::Writer:
    case vestra::core::FileKind::Text:
        launchApplication(QStringLiteral("vestra-writer"), arguments);
        break;
    case vestra::core::FileKind::Pdf:
        launchApplication(QStringLiteral("vestra-pdf"), arguments);
        break;
    case vestra::core::FileKind::Sheets:
        launchApplication(QStringLiteral("vestra-sheets"), arguments);
        break;
    case vestra::core::FileKind::Slides:
        launchApplication(QStringLiteral("vestra-slides"), arguments);
        break;
    case vestra::core::FileKind::Unknown:
        QMessageBox::warning(this, t("common.error"), t("launcher.unsupported"));
        break;
    }
}

void LauncherWindow::launchApplication(const QString& executableName,
                                       const QStringList& arguments) {
    QString executable = executableName;
#ifdef Q_OS_WIN
    executable += QStringLiteral(".exe");
#endif
    const QString program = QDir(QCoreApplication::applicationDirPath()).filePath(executable);
    if (!QProcess::startDetached(program, arguments)) {
        QMessageBox::critical(this, t("common.error"),
                              t("common.error") + QStringLiteral(": ") + program);
    }
}

bool LauncherWindow::sourceRequiresProtectedView(const QString& path) const {
    const QString absolute = QFileInfo(path).absoluteFilePath();
    for (const QStandardPaths::StandardLocation location :
         {QStandardPaths::DownloadLocation, QStandardPaths::TempLocation}) {
        const QString root = QStandardPaths::writableLocation(location);
        if (!root.isEmpty() && absolute.startsWith(QDir(root).absolutePath() + QDir::separator(),
                                                   Qt::CaseInsensitive)) {
            return true;
        }
    }
#ifdef Q_OS_WIN
    return QFileInfo::exists(absolute + QStringLiteral(":Zone.Identifier"));
#else
    return false;
#endif
}

void LauncherWindow::setBusy(const bool busy) {
    newDocument_->setEnabled(!busy);
    newSheet_->setEnabled(!busy);
    newPresentation_->setEnabled(!busy);
    openPdf_->setEnabled(!busy);
    openFile_->setEnabled(!busy);
    recentFiles_->setEnabled(!busy);
    settingsButton_->setEnabled(!busy);
    status_->setText(busy ? t("launcher.inspecting") : t("launcher.ready"));
}

