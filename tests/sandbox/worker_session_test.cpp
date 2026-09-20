#include "vestra/sandbox/worker_session.h"

#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

class WorkerSessionTest final : public QObject {
    Q_OBJECT

  private slots:
    void inspectsPlainTextOutOfProcess();
};

void WorkerSessionTest::inspectsPlainTextOutOfProcess() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("sample.txt"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QCOMPARE(file.write("Vestra worker integration test"), 30);
    file.close();

    vestra::sandbox::WorkerSession session;
    QSignalSpy accepted(&session, &vestra::sandbox::WorkerSession::accepted);
    QSignalSpy rejected(&session, &vestra::sandbox::WorkerSession::rejected);
    QSignalSpy failed(&session, &vestra::sandbox::WorkerSession::failed);
    QString error;
    QVERIFY2(session.inspect(path, &error), qPrintable(error));
    QTRY_VERIFY_WITH_TIMEOUT(!accepted.isEmpty() || !rejected.isEmpty() || !failed.isEmpty(),
                             10000);
    const QString rejectionDetail =
        rejected.isEmpty() ? QString()
                           : rejected.first().at(1).toString() + QStringLiteral(": ") +
                                 rejected.first().at(2).toStringList().join(QLatin1Char(','));
    const QString failureDetail = failed.isEmpty() ? QString() : failed.first().at(1).toString();
    QVERIFY2(rejected.isEmpty(), qPrintable(rejectionDetail));
    QVERIFY2(failed.isEmpty(), qPrintable(failureDetail));
    QCOMPARE(accepted.size(), 1);
    QCOMPARE(accepted.first().at(0).toString(), path);
}

QTEST_GUILESS_MAIN(WorkerSessionTest)
#include "worker_session_test.moc"

