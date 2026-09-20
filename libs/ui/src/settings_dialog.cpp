#include "vestra/ui/settings_dialog.h"

#include "vestra/core/application_paths.h"
#include "vestra/settings/app_settings.h"
#include "vestra/ui/theme_manager.h"
#include "vestra/ui/translation_manager.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QStackedWidget>
#include <QUrl>
#include <QVBoxLayout>

namespace vestra::ui {

SettingsDialog::SettingsDialog(settings::AppSettings& settings, QWidget* parent)
    : QDialog(parent), settings_(settings) {
    resize(720, 470);
    auto* root = new QVBoxLayout(this);
    auto* content = new QHBoxLayout;
    categories_ = new QListWidget;
    categories_->setFixedWidth(180);
    pages_ = new QStackedWidget;
    pages_->addWidget(createGeneralPage());
    pages_->addWidget(createAppearancePage());
    pages_->addWidget(createDocumentsPage());
    pages_->addWidget(createSecurityPage());
    pages_->addWidget(createUpdatesPage());
    pages_->addWidget(createAdvancedPage());
    content->addWidget(categories_);
    content->addWidget(pages_, 1);
    root->addLayout(content, 1);

    auto* buttons = new QHBoxLayout;
    buttons->addStretch();
    cancelButton_ = new QPushButton;
    saveButton_ = new QPushButton;
    saveButton_->setDefault(true);
    buttons->addWidget(cancelButton_);
    buttons->addWidget(saveButton_);
    root->addLayout(buttons);

    connect(categories_, &QListWidget::currentRowChanged, pages_, &QStackedWidget::setCurrentIndex);
    connect(cancelButton_, &QPushButton::clicked, this, &QDialog::reject);
    connect(saveButton_, &QPushButton::clicked, this, &SettingsDialog::save);
    connect(clearRecent_, &QPushButton::clicked, this, [this] {
        settings_.clearRecentFiles();
        emit recentFilesCleared();
    });
    connect(openLogs_, &QPushButton::clicked, this,
            [] { QDesktopServices::openUrl(QUrl::fromLocalFile(core::ApplicationPaths::logs())); });

    categories_->setCurrentRow(0);
    retranslate();
}

QWidget* SettingsDialog::createGeneralPage() {
    auto* page = new QWidget;
    auto* layout = new QFormLayout(page);
    language_ = new QComboBox;
    language_->addItem(QStringLiteral("English"), QStringLiteral("en"));
    language_->addItem(QStringLiteral("Русский"), QStringLiteral("ru"));
    const int index = language_->findData(settings_.language());
    language_->setCurrentIndex(index >= 0 ? index : 0);
    layout->addRow(t("settings.language"), language_);
    return page;
}

QWidget* SettingsDialog::createAppearancePage() {
    auto* page = new QWidget;
    auto* layout = new QFormLayout(page);
    theme_ = new QComboBox;
    theme_->addItem(t("theme.system"), QStringLiteral("system"));
    theme_->addItem(t("theme.light"), QStringLiteral("light"));
    theme_->addItem(t("theme.dark"), QStringLiteral("dark"));
    theme_->setCurrentIndex(static_cast<int>(settings_.theme()));
    layout->addRow(t("settings.theme"), theme_);
    return page;
}

QWidget* SettingsDialog::createDocumentsPage() {
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    recovery_ = new QCheckBox(t("settings.auto_recovery"));
    recovery_->setChecked(settings_.autoRecoveryEnabled());
    clearRecent_ = new QPushButton(t("settings.clear_recent"));
    layout->addWidget(recovery_);
    layout->addWidget(clearRecent_, 0, Qt::AlignLeft);
    layout->addStretch();
    return page;
}

QWidget* SettingsDialog::createSecurityPage() {
    auto* page = new QWidget;
    auto* layout = new QFormLayout(page);
    protectedView_ = new QCheckBox(t("settings.protected_view"));
    protectedView_->setChecked(settings_.protectedViewEnabled());
    macros_ = new QCheckBox(t("settings.macros"));
    macros_->setChecked(false);
    macros_->setEnabled(false);
    telemetry_ = new QCheckBox(t("settings.telemetry"));
    telemetry_->setChecked(false);
    telemetry_->setEnabled(false);
    externalLinks_ = new QComboBox;
    externalLinks_->addItem(t("settings.ask"), QStringLiteral("ask"));
    externalLinks_->addItem(t("settings.block"), QStringLiteral("block"));
    externalLinks_->setCurrentIndex(settings_.externalLinksPolicy() == QStringLiteral("block") ? 1
                                                                                               : 0);
    layout->addRow(protectedView_);
    layout->addRow(macros_);
    layout->addRow(telemetry_);
    layout->addRow(t("settings.external_links"), externalLinks_);
    return page;
}

QWidget* SettingsDialog::createUpdatesPage() {
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    updates_ = new QCheckBox(t("settings.update_consent"));
    updates_->setChecked(settings_.updateChecksAllowed());
    auto* note = new QLabel(t("settings.updates_note"));
    note->setWordWrap(true);
    layout->addWidget(updates_);
    layout->addWidget(note);
    layout->addStretch();
    return page;
}

QWidget* SettingsDialog::createAdvancedPage() {
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    auto* note = new QLabel(t("settings.local_data_note"));
    note->setWordWrap(true);
    openLogs_ = new QPushButton(t("settings.open_logs"));
    layout->addWidget(note);
    layout->addWidget(openLogs_, 0, Qt::AlignLeft);
    layout->addStretch();
    return page;
}

void SettingsDialog::retranslate() {
    setWindowTitle(t("settings.title"));
    const QStringList names{t("settings.general"),   t("settings.appearance"),
                            t("settings.documents"), t("settings.security"),
                            t("settings.updates"),   t("settings.advanced")};
    categories_->clear();
    categories_->addItems(names);
    saveButton_->setText(t("common.save"));
    cancelButton_->setText(t("common.cancel"));
}

void SettingsDialog::save() {
    settings_.setLanguage(language_->currentData().toString());
    const QString selectedTheme = theme_->currentData().toString();
    settings_.setTheme(selectedTheme == QStringLiteral("dark")    ? settings::Theme::Dark
                       : selectedTheme == QStringLiteral("light") ? settings::Theme::Light
                                                                  : settings::Theme::System);
    settings_.setAutoRecoveryEnabled(recovery_->isChecked());
    settings_.setProtectedViewEnabled(protectedView_->isChecked());
    settings_.setExternalLinksPolicy(externalLinks_->currentData().toString());
    settings_.setUpdateChecksAllowed(updates_->isChecked());
    settings_.sync();
    TranslationManager::instance().setLanguage(settings_.language());
    ThemeManager::apply(settings_.theme());
    emit settingsApplied();
    accept();
}

} // namespace vestra::ui

