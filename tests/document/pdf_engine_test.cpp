#include "vestra/document/engine_factory.h"
#include "vestra/document/pdf_engine.h"

#include <QPainter>
#include <QPdfWriter>
#include <QTemporaryDir>
#include <QtTest>

class PdfEngineTest final : public QObject {
    Q_OBJECT

  private slots:
    void rendersGeneratedPdf() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString path = directory.filePath(QStringLiteral("sample.pdf"));
        {
            QPdfWriter writer(path);
            writer.setResolution(96);
            QPainter painter(&writer);
            QVERIFY(painter.isActive());
            painter.drawText(QPoint(80, 120), QStringLiteral("Vestra PDF engine test"));
        }

        std::unique_ptr<vestra::document::PdfEngine> engine = vestra::document::createPdfEngine();
        QVERIFY2(engine->info().runtimeReady, qPrintable(engine->info().detail));
        const vestra::document::OperationResult opened = engine->open(path);
        QVERIFY2(opened.ok(), qPrintable(opened.detail));
        QCOMPARE(engine->pageCount(), 1);
        QImage image;
        const vestra::document::OperationResult rendered =
            engine->renderPage(0, QSize(640, 900), &image);
        QVERIFY2(rendered.ok(), qPrintable(rendered.detail));
        QVERIFY(!image.isNull());
        QCOMPARE(image.size(), QSize(640, 900));
    }
};

QTEST_MAIN(PdfEngineTest)
#include "pdf_engine_test.moc"

