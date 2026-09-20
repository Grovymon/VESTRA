#pragma once

#include <QByteArray>
#include <QList>
#include <QString>

namespace vestra::ipc {

inline constexpr quint32 kMaximumFrameBytes = 1024U * 1024U;

struct DecodeResult final {
    QList<QByteArray> frames;
    QString error;
    bool fatal{false};
};

[[nodiscard]] QByteArray encodeFrame(const QByteArray& payload, QString* error = nullptr);

class FrameDecoder final {
  public:
    DecodeResult append(const QByteArray& bytes);
    void reset();

  private:
    QByteArray buffer_;
};

} // namespace vestra::ipc

