#include "vestra/ipc/frame_codec.h"

#include <QtEndian>

#include <cstring>

namespace vestra::ipc {

QByteArray encodeFrame(const QByteArray& payload, QString* error) {
    if (payload.size() > static_cast<qsizetype>(kMaximumFrameBytes)) {
        if (error != nullptr) {
            *error = QStringLiteral("IPC payload exceeds the maximum frame size");
        }
        return {};
    }

    QByteArray frame(sizeof(quint32), Qt::Uninitialized);
    qToLittleEndian(static_cast<quint32>(payload.size()), frame.data());
    frame.append(payload);
    return frame;
}

DecodeResult FrameDecoder::append(const QByteArray& bytes) {
    DecodeResult result;
    buffer_.append(bytes);
    if (buffer_.size() > static_cast<qsizetype>(kMaximumFrameBytes + sizeof(quint32))) {
        result.error = QStringLiteral("Buffered IPC data exceeds the protocol limit");
        result.fatal = true;
        buffer_.clear();
        return result;
    }

    while (buffer_.size() >= static_cast<qsizetype>(sizeof(quint32))) {
        quint32 length = 0;
        std::memcpy(&length, buffer_.constData(), sizeof(length));
        length = qFromLittleEndian(length);
        if (length > kMaximumFrameBytes) {
            result.error = QStringLiteral("Declared IPC frame exceeds the protocol limit");
            result.fatal = true;
            buffer_.clear();
            return result;
        }

        const qsizetype fullFrameSize = static_cast<qsizetype>(sizeof(quint32)) + length;
        if (buffer_.size() < fullFrameSize) {
            break;
        }
        result.frames.append(buffer_.mid(sizeof(quint32), length));
        buffer_.remove(0, fullFrameSize);
    }
    return result;
}

void FrameDecoder::reset() { buffer_.clear(); }

} // namespace vestra::ipc

