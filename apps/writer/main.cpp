#include "writer_window.h"

#include "vestra/core/application_paths.h"
#include "vestra/logging/logger.h"
#include "vestra/settings/app_settings.h"
#include "vestra/ui/theme_manager.h"
#include "vestra/ui/translation_manager.h"

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QMessageBox>

int main(int argc, char* argv[]) {
    QApplication application(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("Vestra"));
    QCoreApplication::setApplicationName(QStringLiteral("Vestra Writer"));
    QCoreApplication::setApplicationVersion(QStringLiteral(VESTRA_VERSION));

    QString pathError;
    if (!vestra::core::ApplicationPaths::ensureCreated(&pathError)) {
        QMessageBox::critical(nullptr, QStringLiteral("Vestra"), pathError);
        return 1;
    }
    vestra::logging::Logger::install(QStringLiteral("writer"));
    vestra::settings::AppSettings settings;
    vestra::ui::TranslationManager::instance().setLanguage(settings.language());
    vestra::ui::ThemeManager::apply(settings.theme());

    QCommandLineParser parser;
    parser.addHelpOption();
    const QCommandLineOption fileOption(QStringLiteral("file"), QStringLiteral("Document path"),
                                        QStringLiteral("path"));
    const QCommandLineOption protectedOption(QStringLiteral("protected-view"),
                                             QStringLiteral("Open read-only"));
    parser.addOptions({fileOption, protectedOption});
    parser.process(application);

    WriterWindow window;
    window.showMaximized();
    if (parser.isSet(fileOption)) {
        window.requestOpen(parser.value(fileOption), parser.isSet(protectedOption));
    }
    return application.exec();
}

