#include "vestra/core/file_type.h"

#include <QTest>

class FileTypeTest final : public QObject {
    Q_OBJECT

  private slots:
    void detectsSupportedTypes_data();
    void detectsSupportedTypes();
    void marksMacroCapableTypes();
};

void FileTypeTest::detectsSupportedTypes_data() {
    QTest::addColumn<QString>("path");
    QTest::addColumn<int>("kind");
    QTest::newRow("docx") << QStringLiteral("report.DOCX")
                          << static_cast<int>(vestra::core::FileKind::Writer);
    QTest::newRow("doc") << QStringLiteral("legacy.doc")
                         << static_cast<int>(vestra::core::FileKind::Writer);
    QTest::newRow("xlsx") << QStringLiteral("table.xlsx")
                          << static_cast<int>(vestra::core::FileKind::Sheets);
    QTest::newRow("vestra-sheets")
        << QStringLiteral("book.vestra-sheets") << static_cast<int>(vestra::core::FileKind::Sheets);
    QTest::newRow("pptx") << QStringLiteral("deck.pptx")
                          << static_cast<int>(vestra::core::FileKind::Slides);
    QTest::newRow("vestra-slides")
        << QStringLiteral("deck.vestra-slides") << static_cast<int>(vestra::core::FileKind::Slides);
    QTest::newRow("pdf") << QStringLiteral("scan.pdf")
                         << static_cast<int>(vestra::core::FileKind::Pdf);
    QTest::newRow("txt") << QStringLiteral("notes.txt")
                         << static_cast<int>(vestra::core::FileKind::Text);
    QTest::newRow("unknown") << QStringLiteral("program.exe")
                             << static_cast<int>(vestra::core::FileKind::Unknown);
}

void FileTypeTest::detectsSupportedTypes() {
    QFETCH(QString, path);
    QFETCH(int, kind);
    QCOMPARE(static_cast<int>(vestra::core::detectFileType(path).kind), kind);
}

void FileTypeTest::marksMacroCapableTypes() {
    QVERIFY(vestra::core::detectFileType(QStringLiteral("unsafe.docm")).mayContainMacros);
    QVERIFY(vestra::core::detectFileType(QStringLiteral("legacy.xls")).mayContainMacros);
    QVERIFY(!vestra::core::detectFileType(QStringLiteral("safe.docx")).mayContainMacros);
}

QTEST_APPLESS_MAIN(FileTypeTest)
#include "file_type_test.moc"

