#include "vestra/security/document_scanner.h"

#include <QFile>
#include <QTemporaryDir>
#include <QTest>

class DocumentScannerTest final : public QObject {
    Q_OBJECT

  private slots:
    void allowsPlainText();
    void rejectsUnsupportedType();
    void rejectsInvalidOoxmlZip();
    void enforcesFileSizeLimit();
};

void DocumentScannerTest::allowsPlainText() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QFile file(directory.filePath(QStringLiteral("input.txt")));
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("safe text");
    file.close();
    const auto result = vestra::security::DocumentScanner().scan(file.fileName(), false);
    QCOMPARE(result.disposition, vestra::security::Disposition::Allow);
}

void DocumentScannerTest::rejectsUnsupportedType() {
    QTemporaryDir directory;
    QFile file(directory.filePath(QStringLiteral("input.exe")));
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("MZ");
    file.close();
    const auto result = vestra::security::DocumentScanner().scan(file.fileName(), false);
    QCOMPARE(result.disposition, vestra::security::Disposition::Block);
    QCOMPARE(result.code, QStringLiteral("unsupported_type"));
}

void DocumentScannerTest::rejectsInvalidOoxmlZip() {
    QTemporaryDir directory;
    QFile file(directory.filePath(QStringLiteral("input.docx")));
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("not-a-zip");
    file.close();
    const auto result = vestra::security::DocumentScanner().scan(file.fileName(), false);
    QCOMPARE(result.disposition, vestra::security::Disposition::Block);
    QCOMPARE(result.code, QStringLiteral("invalid_ooxml_zip"));
}

void DocumentScannerTest::enforcesFileSizeLimit() {
    QTemporaryDir directory;
    QFile file(directory.filePath(QStringLiteral("input.txt")));
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("123456789");
    file.close();
    vestra::security::ScanLimits limits;
    limits.maximumFileBytes = 4;
    const auto result = vestra::security::DocumentScanner(limits).scan(file.fileName(), false);
    QCOMPARE(result.code, QStringLiteral("file_too_large"));
}

QTEST_APPLESS_MAIN(DocumentScannerTest)
#include "document_scanner_test.moc"

