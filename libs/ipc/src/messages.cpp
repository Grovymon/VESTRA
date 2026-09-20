#include "vestra/ipc/messages.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace vestra::ipc {
namespace {
bool parseObject(const QByteArray& json, QJsonObject* object, QString* error) {
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (error != nullptr) {
            *error = QStringLiteral("Invalid JSON object: %1").arg(parseError.errorString());
        }
        return false;
    }
    *object = document.object();
    return true;
}

bool requiredString(const QJsonObject& object, const QString& key, QString* value, QString* error) {
    const QJsonValue item = object.value(key);
    if (!item.isString() || item.toString().isEmpty() || item.toString().size() > 32768) {
        if (error != nullptr) {
            *error = QStringLiteral("Missing or invalid field: %1").arg(key);
        }
        return false;
    }
    *value = item.toString();
    return true;
}
} // namespace

QByteArray serialize(const OpenDocumentRequest& request) {
    QJsonObject object{{QStringLiteral("type"), QStringLiteral("open_document")},
                       {QStringLiteral("requestId"), request.requestId},
                       {QStringLiteral("token"), request.token},
                       {QStringLiteral("path"), request.path},
                       {QStringLiteral("readOnly"), request.readOnly}};
    return QJsonDocument(object).toJson(QJsonDocument::Compact);
}

QByteArray serialize(const WorkerResponse& response) {
    QJsonArray findings;
    for (const QString& finding : response.findings) {
        findings.append(finding);
    }
    QJsonObject object{{QStringLiteral("type"), QStringLiteral("open_result")},
                       {QStringLiteral("requestId"), response.requestId},
                       {QStringLiteral("accepted"), response.accepted},
                       {QStringLiteral("protectedViewRequired"), response.protectedViewRequired},
                       {QStringLiteral("macrosDetected"), response.macrosDetected},
                       {QStringLiteral("code"), response.code},
                       {QStringLiteral("findings"), findings}};
    return QJsonDocument(object).toJson(QJsonDocument::Compact);
}

bool parseOpenDocumentRequest(const QByteArray& json, OpenDocumentRequest* request,
                              QString* error) {
    if (request == nullptr) {
        return false;
    }
    QJsonObject object;
    if (!parseObject(json, &object, error) ||
        object.value(QStringLiteral("type")) != QStringLiteral("open_document")) {
        if (error != nullptr && error->isEmpty()) {
            *error = QStringLiteral("Unexpected IPC message type");
        }
        return false;
    }
    if (!requiredString(object, QStringLiteral("requestId"), &request->requestId, error) ||
        !requiredString(object, QStringLiteral("token"), &request->token, error) ||
        !requiredString(object, QStringLiteral("path"), &request->path, error)) {
        return false;
    }
    const QJsonValue readOnly = object.value(QStringLiteral("readOnly"));
    if (!readOnly.isBool()) {
        if (error != nullptr) {
            *error = QStringLiteral("Missing or invalid field: readOnly");
        }
        return false;
    }
    request->readOnly = readOnly.toBool();
    return true;
}

bool parseWorkerResponse(const QByteArray& json, WorkerResponse* response, QString* error) {
    if (response == nullptr) {
        return false;
    }
    QJsonObject object;
    if (!parseObject(json, &object, error) ||
        object.value(QStringLiteral("type")) != QStringLiteral("open_result")) {
        if (error != nullptr && error->isEmpty()) {
            *error = QStringLiteral("Unexpected IPC response type");
        }
        return false;
    }
    if (!requiredString(object, QStringLiteral("requestId"), &response->requestId, error) ||
        !requiredString(object, QStringLiteral("code"), &response->code, error)) {
        return false;
    }
    if (!object.value(QStringLiteral("accepted")).isBool() ||
        !object.value(QStringLiteral("protectedViewRequired")).isBool() ||
        !object.value(QStringLiteral("macrosDetected")).isBool() ||
        !object.value(QStringLiteral("findings")).isArray()) {
        if (error != nullptr) {
            *error = QStringLiteral("Invalid worker response fields");
        }
        return false;
    }
    response->accepted = object.value(QStringLiteral("accepted")).toBool();
    response->protectedViewRequired =
        object.value(QStringLiteral("protectedViewRequired")).toBool();
    response->macrosDetected = object.value(QStringLiteral("macrosDetected")).toBool();
    response->findings.clear();
    for (const QJsonValue& value : object.value(QStringLiteral("findings")).toArray()) {
        if (value.isString()) {
            response->findings.append(value.toString());
        }
    }
    return true;
}

} // namespace vestra::ipc

