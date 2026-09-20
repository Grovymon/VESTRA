#include "vestra/settings/app_settings.h"
#include "vestra/ui/translation_manager.h"

#include <QFile>
#include <QTemporaryDir>
#include <QTest>

class AppSettingsTest final : public QObject {
    Q_OBJECT

  private slots:
    void persistsPrivacySafeSettings();
    void loadsBothTranslationCatalogs();
};

void AppSettingsTest::persistsPrivacySafeSettings() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    qputenv("LOCALAPPDATA", directory.path().toLocal8Bit());

    const QString recentPath = directory.filePath(QStringLiteral("recent.txt"));
    QFile recent(recentPath);
    QVERIFY(recent.open(QIODevice::WriteOnly));
    recent.write("recent");
    recent.close();

    {
        vestra::settings::AppSettings settings;
        QVERIFY(!settings.telemetryEnabled());
        QVERIFY(!settings.macrosEnabled());
        QVERIFY(settings.protectedViewEnabled());
        QVERIFY(settings.autoRecoveryEnabled());
        QVERIFY(!settings.updateChecksAllowed());
        QCOMPARE(settings.externalLinksPolicy(), QStringLiteral("ask"));
        settings.setLanguage(QStringLiteral("ru"));
        settings.setTheme(vestra::settings::Theme::Dark);
        settings.addRecentFile(recentPath);
        settings.sync();
    }
    {
        vestra::settings::AppSettings settings;
        QCOMPARE(settings.language(), QStringLiteral("ru"));
        QCOMPARE(settings.theme(), vestra::settings::Theme::Dark);
        QCOMPARE(settings.recentFiles(), QStringList{recentPath});
    }
}

void AppSettingsTest::loadsBothTranslationCatalogs() {
    auto& translations = vestra::ui::TranslationManager::instance();
    QString error;
    QVERIFY2(translations.setLanguage(QStringLiteral("en"), &error), qPrintable(error));
    QCOMPARE(translations.text(QStringLiteral("launcher.new_document")),
             QStringLiteral("New document"));
    QVERIFY2(translations.setLanguage(QStringLiteral("ru"), &error), qPrintable(error));
    QCOMPARE(translations.text(QStringLiteral("launcher.new_document")),
             QStringLiteral("Новый документ"));
}

QTEST_GUILESS_MAIN(AppSettingsTest)
#include "app_settings_test.moc"

