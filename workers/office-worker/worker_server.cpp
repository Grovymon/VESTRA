#include "worker_server.h"

#include "vestra/ipc/messages.h"
#include "vestra/security/document_scanner.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QLocalSocket>
#include <QTimer>

#include <utility>

WorkerServer::WorkerServer(QString serverName, QString token, QString inputRoot, QObject* parent)
    : QObject(parent), serverName_(std::move(serverName)), token_(std::move(token)),
      inputRoot_(QDir(std::move(inputRoot)).canonicalPath()) {
    server_.setParent(this);
    server_.setSocketOptions(QLocalServer::UserAccessOption);
    connect(&server_, &QLocalServer::newConnection, this, &WorkerServer::acceptConnection);
}

bool WorkerServer::start(QString* error) {
    if (serverName_.isEmpty() || token_.size() != 64 || inputRoot_.isEmpty()) {
        if (error != nullptr) {
            *error = QStringLiteral("Invalid worker startup arguments");
        }
        return false;
    }
    QLocalServer::removeServer(serverName_);
    if (!server_.listen(serverName_)) {
        if (error != nullptr) {
            *error = server_.errorString();
        }
        return false;
    }
    return true;
}

void WorkerServer::acceptConnection() {
    if (socket_ != nullptr) {
        if (QLocalSocket* extra = server_.nextPendingConnection(); extra != nullptr) {
            extra->disconnectFromServer();
            extra->deleteLater();
        }
        return;
    }
    socket_ = server_.nextPendingConnection();
    server_.close();
    connect(socket_, &QLocalSocket::readyRead, this, &WorkerServer::readRequest);
    connect(socket_, &QLocalSocket::disconnected, qApp, &QCoreApplication::quit);
}

void WorkerServer::readRequest() {
    const vestra::ipc::DecodeResult decoded = decoder_.append(socket_->readAll());
    if (decoded.fatal) {
        rejectMalformed(QStringLiteral("invalid"), QStringLiteral("invalid_frame"), decoded.error);
        return;
    }
    for (const QByteArray& frame : decoded.frames) {
        vestra::ipc::OpenDocumentRequest request;
        QString error;
        if (!vestra::ipc::parseOpenDocumentRequest(frame, &request, &error)) {
            rejectMalformed(QStringLiteral("invalid"), QStringLiteral("invalid_request"), error);
            return;
        }
        if (request.token != token_) {
            rejectMalformed(request.requestId, QStringLiteral("authentication_failed"),
                            QStringLiteral("ipc.authentication_failed"));
            return;
        }
        if (!request.readOnly || !isAllowedPath(request.path)) {
            rejectMalformed(request.requestId, QStringLiteral("path_not_allowed"),
                            QStringLiteral("ipc.path_not_allowed"));
            return;
        }

        const vestra::security::ScanResult scan =
            vestra::security::DocumentScanner().scan(request.path, false);
        vestra::ipc::WorkerResponse response;
        response.requestId = request.requestId;
        response.accepted = scan.disposition != vestra::security::Disposition::Block;
        response.protectedViewRequired =
            scan.disposition == vestra::security::Disposition::ProtectedView;
        response.macrosDetected = scan.macrosDetected;
        response.code = scan.code;
        response.findings = scan.findings;
        socket_->write(vestra::ipc::encodeFrame(vestra::ipc::serialize(response)));
        socket_->flush();
        QTimer::singleShot(100, qApp, &QCoreApplication::quit);
        return;
    }
}

void WorkerServer::rejectMalformed(const QString& requestId, const QString& code,
                                   const QString& finding) {
    vestra::ipc::WorkerResponse response;
    response.requestId = requestId;
    response.code = code;
    response.findings = {finding};
    socket_->write(vestra::ipc::encodeFrame(vestra::ipc::serialize(response)));
    socket_->flush();
    QTimer::singleShot(100, qApp, &QCoreApplication::quit);
}

bool WorkerServer::isAllowedPath(const QString& path) const {
    const QString canonicalPath = QFileInfo(path).canonicalFilePath();
    if (canonicalPath.isEmpty()) {
        return false;
    }
    const QString relative = QDir(inputRoot_).relativeFilePath(canonicalPath);
    return !QDir::isAbsolutePath(relative) && relative != QStringLiteral("..") &&
           !relative.startsWith(QStringLiteral("../")) &&
           !relative.startsWith(QStringLiteral("..\\")) && !relative.contains(QLatin1Char('/')) &&
           !relative.contains(QLatin1Char('\\')) && relative.startsWith(QStringLiteral("input"));
}

