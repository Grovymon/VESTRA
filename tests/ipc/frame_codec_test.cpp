#include "vestra/ipc/frame_codec.h"
#include "vestra/ipc/messages.h"

#include <QTest>
#include <QtEndian>

class FrameCodecTest final : public QObject {
    Q_OBJECT

  private slots:
    void roundTripsMessage();
    void acceptsPartialFrames();
    void rejectsOversizedFrame();
    void validatesMessageFields();
};

void FrameCodecTest::roundTripsMessage() {
    const vestra::ipc::OpenDocumentRequest request{QStringLiteral("request-1"),
                                                   QStringLiteral("token"),
                                                   QStringLiteral("C:/Temp/input.docx"), true};
    const QByteArray frame = vestra::ipc::encodeFrame(vestra::ipc::serialize(request));
    vestra::ipc::FrameDecoder decoder;
    const vestra::ipc::DecodeResult result = decoder.append(frame);
    QCOMPARE(result.frames.size(), 1);
    vestra::ipc::OpenDocumentRequest parsed;
    QString error;
    QVERIFY(vestra::ipc::parseOpenDocumentRequest(result.frames.first(), &parsed, &error));
    QCOMPARE(parsed.requestId, request.requestId);
    QCOMPARE(parsed.path, request.path);
    QVERIFY(parsed.readOnly);
}

void FrameCodecTest::acceptsPartialFrames() {
    const QByteArray frame = vestra::ipc::encodeFrame(QByteArrayLiteral("hello"));
    vestra::ipc::FrameDecoder decoder;
    QCOMPARE(decoder.append(frame.left(3)).frames.size(), 0);
    const vestra::ipc::DecodeResult result = decoder.append(frame.mid(3));
    QCOMPARE(result.frames, QList<QByteArray>{QByteArrayLiteral("hello")});
}

void FrameCodecTest::rejectsOversizedFrame() {
    QByteArray header(sizeof(quint32), '\0');
    qToLittleEndian(vestra::ipc::kMaximumFrameBytes + 1U, header.data());
    vestra::ipc::FrameDecoder decoder;
    const vestra::ipc::DecodeResult result = decoder.append(header);
    QVERIFY(result.fatal);
    QVERIFY(!result.error.isEmpty());
}

void FrameCodecTest::validatesMessageFields() {
    vestra::ipc::OpenDocumentRequest request;
    QString error;
    QVERIFY(!vestra::ipc::parseOpenDocumentRequest(
        QByteArrayLiteral("{\"type\":\"open_document\"}"), &request, &error));
    QVERIFY(!error.isEmpty());
}

QTEST_APPLESS_MAIN(FrameCodecTest)
#include "frame_codec_test.moc"

