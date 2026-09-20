#pragma once

#include "vestra/ipc/frame_codec.h"

#include <QObject>
#include <QStringList>

#include <memory>

class QLocalSocket;
class QProcess;
class QTimer;

namespace vestra::sandbox {

class WorkerSession final : public QObject {
    Q_OBJECT

  public:
    explicit WorkerSession(QObject* parent = nullptr);
    ~WorkerSession() override;
    WorkerSession(const WorkerSession&) = delete;
    WorkerSession& operator=(const WorkerSession&) = delete;

    bool inspect(const QString& sourcePath, QString* error = nullptr);
    [[nodiscard]] bool isRunning() const;

  signals:
    void accepted(const QString& sourcePath, bool protectedViewRequired, bool macrosDetected,
                  const QStringList& findings);
    void rejected(const QString& sourcePath, const QString& code, const QStringList& findings);
    void failed(const QString& sourcePath, const QString& detail);

  private:
    void tryConnect();
    void sendRequest();
    void handleReadyRead();
    void finish();
    void fail(const QString& detail);
    bool createJobDirectory(const QString& sourcePath, QString* error);
    void applyProcessLimits();

    std::unique_ptr<QProcess> process_;
    std::unique_ptr<QLocalSocket> socket_;
    std::unique_ptr<QTimer> connectTimer_;
    std::unique_ptr<QTimer> timeoutTimer_;
    ipc::FrameDecoder decoder_;
    QString sourcePath_;
    QString copiedPath_;
    QString jobDirectory_;
    QString serverName_;
    QString token_;
    QString requestId_;
    int connectAttempts_{0};
    bool finishing_{false};
    void* jobHandle_{nullptr};
};

} // namespace vestra::sandbox

