#pragma once

#include <QByteArray>
#include <QString>
#include <QStringList>

namespace vestra::ipc {

struct OpenDocumentRequest final {
    QString requestId;
    QString token;
    QString path;
    bool readOnly{true};
};

struct WorkerResponse final {
    QString requestId;
    bool accepted{false};
    bool protectedViewRequired{false};
    bool macrosDetected{false};
    QString code;
    QStringList findings;
};

[[nodiscard]] QByteArray serialize(const OpenDocumentRequest& request);
[[nodiscard]] QByteArray serialize(const WorkerResponse& response);
[[nodiscard]] bool parseOpenDocumentRequest(const QByteArray& json, OpenDocumentRequest* request,
                                            QString* error);
[[nodiscard]] bool parseWorkerResponse(const QByteArray& json, WorkerResponse* response,
                                       QString* error);

} // namespace vestra::ipc

