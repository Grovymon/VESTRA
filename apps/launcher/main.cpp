#include "launcher_window.h"

#include "vestra/core/application_paths.h"
#include "vestra/logging/logger.h"
#include "vestra/settings/app_settings.h"
#include "vestra/ui/theme_manager.h"
#include "vestra/ui/translation_manager.h"

#include <QApplication>
#include <QMessageBox>

int main(int argc, char* argv[]) {
    QApplication application(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("Vestra"));
    QCoreApplication::setApplicationName(QStringLiteral("Vestra"));
    QCoreApplication::setApplicationVersion(QStringLiteral(VESTRA_VERSION));

    QString pathError;
    if (!vestra::core::ApplicationPaths::ensureCreated(&pathError)) {
        QMessageBox::critical(nullptr, QStringLiteral("Vestra"), pathError);
        return 1;
    }
    vestra::logging::Logger::install(QStringLiteral("vestra"));
    vestra::settings::AppSettings settings;
    vestra::ui::TranslationManager::instance().setLanguage(settings.language());
    vestra::ui::ThemeManager::apply(settings.theme());

    LauncherWindow window;
    window.show();
    return application.exec();
}

