#pragma once

#include "vestra/ipc/frame_codec.h"

#include <QLocalServer>
#include <QObject>

class QLocalSocket;
class QTimer;

class WorkerServer final : public QObject {
    Q_OBJECT

  public:
    WorkerServer(QString serverName, QString token, QString inputRoot, QObject* parent = nullptr);
    bool start(QString* error);

  private:
    void acceptConnection();
    void readRequest();
    void rejectMalformed(const QString& requestId, const QString& code, const QString& finding);
    [[nodiscard]] bool isAllowedPath(const QString& path) const;

    QString serverName_;
    QString token_;
    QString inputRoot_;
    QLocalServer server_;
    QLocalSocket* socket_{nullptr};
    vestra::ipc::FrameDecoder decoder_;
};

