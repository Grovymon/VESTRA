#include "worker_server.h"

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QTimer>

int main(int argc, char* argv[]) {
    QCoreApplication application(argc, argv);
    application.setApplicationName(QStringLiteral("Vestra Document Worker"));

    QCommandLineParser parser;
    parser.addHelpOption();
    const QCommandLineOption serverOption(
        QStringLiteral("server"), QStringLiteral("IPC server name"), QStringLiteral("name"));
    const QCommandLineOption tokenOption(QStringLiteral("token"), QStringLiteral("Session token"),
                                         QStringLiteral("token"));
    const QCommandLineOption rootOption(
        QStringLiteral("input-root"), QStringLiteral("Allowed input root"), QStringLiteral("path"));
    parser.addOptions({serverOption, tokenOption, rootOption});
    parser.process(application);

    WorkerServer server(parser.value(serverOption), parser.value(tokenOption),
                        parser.value(rootOption));
    QString error;
    if (!server.start(&error)) {
        qCritical().noquote() << error;
        return 2;
    }
    QTimer::singleShot(20000, &application, &QCoreApplication::quit);
    return application.exec();
}

