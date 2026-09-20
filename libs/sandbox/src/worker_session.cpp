#include "vestra/sandbox/worker_session.h"

#include "vestra/core/application_paths.h"
#include "vestra/ipc/messages.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLocalSocket>
#include <QProcess>
#include <QProcessEnvironment>
#include <QRandomGenerator>
#include <QTimer>
#include <QUuid>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace vestra::sandbox {
namespace {
QString randomToken() {
    QByteArray bytes(32, Qt::Uninitialized);
    for (char& byte : bytes) {
        byte = static_cast<char>(QRandomGenerator::system()->generate() & 0xffU);
    }
    return QString::fromLatin1(bytes.toHex());
}

QString workerExecutable() {
    QString name = QStringLiteral("vestra-document-worker");
#ifdef Q_OS_WIN
    name += QStringLiteral(".exe");
#endif
    return QDir(QCoreApplication::applicationDirPath()).filePath(name);
}
} // namespace

WorkerSession::WorkerSession(QObject* parent)
    : QObject(parent), process_(std::make_unique<QProcess>()),
      socket_(std::make_unique<QLocalSocket>()), connectTimer_(std::make_unique<QTimer>()),
      timeoutTimer_(std::make_unique<QTimer>()) {
    process_->setParent(this);
    socket_->setParent(this);
    connectTimer_->setParent(this);
    timeoutTimer_->setParent(this);
    connectTimer_->setInterval(60);
    timeoutTimer_->setSingleShot(true);

    connect(connectTimer_.get(), &QTimer::timeout, this, &WorkerSession::tryConnect);
    connect(timeoutTimer_.get(), &QTimer::timeout, this,
            [this] { fail(QStringLiteral("Worker operation timed out")); });
    connect(socket_.get(), &QLocalSocket::connected, this, &WorkerSession::sendRequest);
    connect(socket_.get(), &QLocalSocket::readyRead, this, &WorkerSession::handleReadyRead);
    connect(process_.get(), &QProcess::started, this, [this] {
        applyProcessLimits();
        connectTimer_->start();
        tryConnect();
    });
    connect(process_.get(), &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart || error == QProcess::Crashed) {
            fail(process_->errorString());
        }
    });
}

WorkerSession::~WorkerSession() {
    finish();
#ifdef Q_OS_WIN
    if (jobHandle_ != nullptr) {
        CloseHandle(static_cast<HANDLE>(jobHandle_));
    }
#endif
}

bool WorkerSession::inspect(const QString& sourcePath, QString* error) {
    if (isRunning()) {
        if (error != nullptr) {
            *error = QStringLiteral("A document is already being inspected");
        }
        return false;
    }
    const QFileInfo info(sourcePath);
    if (!info.exists() || !info.isFile()) {
        if (error != nullptr) {
            *error = QStringLiteral("The selected file does not exist");
        }
        return false;
    }
    if (!QFileInfo::exists(workerExecutable())) {
        if (error != nullptr) {
            *error = QStringLiteral("Document worker executable was not found: %1")
                         .arg(workerExecutable());
        }
        return false;
    }
    if (!createJobDirectory(sourcePath, error)) {
        return false;
    }

    sourcePath_ = info.absoluteFilePath();
    serverName_ =
        QStringLiteral("vestra-office-%1").arg(QUuid::createUuid().toString(QUuid::Id128));
    token_ = randomToken();
    requestId_ = QUuid::createUuid().toString(QUuid::WithoutBraces);
    connectAttempts_ = 0;
    decoder_.reset();

    QProcessEnvironment environment;
    const QProcessEnvironment system = QProcessEnvironment::systemEnvironment();
    for (const QString& key : {QStringLiteral("SystemRoot"), QStringLiteral("WINDIR")}) {
        if (system.contains(key)) {
            environment.insert(key, system.value(key));
        }
    }
    environment.insert(QStringLiteral("TEMP"), jobDirectory_);
    environment.insert(QStringLiteral("TMP"), jobDirectory_);
    environment.insert(QStringLiteral("VESTRA_WORKER"), QStringLiteral("1"));
    process_->setProcessEnvironment(environment);
    process_->setProgram(workerExecutable());
    process_->setArguments({QStringLiteral("--server"), serverName_, QStringLiteral("--token"),
                            token_, QStringLiteral("--input-root"), jobDirectory_});
    process_->setProcessChannelMode(QProcess::ForwardedErrorChannel);
    process_->start();
    timeoutTimer_->start(15000);
    return true;
}

bool WorkerSession::isRunning() const {
    return process_->state() != QProcess::NotRunning || timeoutTimer_->isActive();
}

bool WorkerSession::createJobDirectory(const QString& sourcePath, QString* error) {
    core::ApplicationPaths::ensureCreated(error);
    jobDirectory_ =
        QDir(core::ApplicationPaths::temp())
            .filePath(QStringLiteral("job-%1").arg(QUuid::createUuid().toString(QUuid::Id128)));
    if (!QDir().mkpath(jobDirectory_)) {
        if (error != nullptr) {
            *error = QStringLiteral("Could not create a private worker directory");
        }
        return false;
    }
    const QString suffix = QFileInfo(sourcePath).suffix();
    copiedPath_ = QDir(jobDirectory_)
                      .filePath(suffix.isEmpty() ? QStringLiteral("input")
                                                 : QStringLiteral("input.%1").arg(suffix));
    if (!QFile::copy(sourcePath, copiedPath_)) {
        QDir(jobDirectory_).removeRecursively();
        if (error != nullptr) {
            *error = QStringLiteral("Could not copy the document into the worker directory");
        }
        return false;
    }
    QFile::setPermissions(copiedPath_, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    return true;
}

void WorkerSession::tryConnect() {
    if (socket_->state() == QLocalSocket::ConnectedState ||
        socket_->state() == QLocalSocket::ConnectingState) {
        return;
    }
    if (++connectAttempts_ > 100) {
        fail(QStringLiteral("Could not connect to the document worker"));
        return;
    }
    socket_->abort();
    socket_->connectToServer(serverName_, QIODevice::ReadWrite);
}

void WorkerSession::sendRequest() {
    connectTimer_->stop();
    const ipc::OpenDocumentRequest request{requestId_, token_, copiedPath_, true};
    QString error;
    const QByteArray frame = ipc::encodeFrame(ipc::serialize(request), &error);
    if (frame.isEmpty() && !error.isEmpty()) {
        fail(error);
        return;
    }
    if (socket_->write(frame) != frame.size()) {
        fail(QStringLiteral("Could not write the IPC request"));
    }
}

void WorkerSession::handleReadyRead() {
    const ipc::DecodeResult decoded = decoder_.append(socket_->readAll());
    if (decoded.fatal) {
        fail(decoded.error);
        return;
    }
    for (const QByteArray& frame : decoded.frames) {
        ipc::WorkerResponse response;
        QString error;
        if (!ipc::parseWorkerResponse(frame, &response, &error) ||
            response.requestId != requestId_) {
            fail(error.isEmpty() ? QStringLiteral("Mismatched worker response") : error);
            return;
        }
        const QString path = sourcePath_;
        if (response.accepted) {
            emit accepted(path, response.protectedViewRequired, response.macrosDetected,
                          response.findings);
        } else {
            emit rejected(path, response.code, response.findings);
        }
        finish();
        return;
    }
}

void WorkerSession::fail(const QString& detail) {
    if (finishing_ || (sourcePath_.isEmpty() && !timeoutTimer_->isActive())) {
        return;
    }
    const QString path = sourcePath_;
    finish();
    emit failed(path, detail);
}

void WorkerSession::finish() {
    if (finishing_) {
        return;
    }
    finishing_ = true;
    connectTimer_->stop();
    timeoutTimer_->stop();
    socket_->abort();
    if (process_->state() != QProcess::NotRunning) {
        process_->terminate();
        if (!process_->waitForFinished(750)) {
            process_->kill();
            process_->waitForFinished(750);
        }
    }
#ifdef Q_OS_WIN
    if (jobHandle_ != nullptr) {
        CloseHandle(static_cast<HANDLE>(jobHandle_));
        jobHandle_ = nullptr;
    }
#endif
    if (!jobDirectory_.isEmpty()) {
        QDir(jobDirectory_).removeRecursively();
    }
    sourcePath_.clear();
    copiedPath_.clear();
    jobDirectory_.clear();
    serverName_.clear();
    token_.clear();
    requestId_.clear();
    finishing_ = false;
}

void WorkerSession::applyProcessLimits() {
#ifdef Q_OS_WIN
    HANDLE job = CreateJobObjectW(nullptr, nullptr);
    if (job == nullptr) {
        qWarning() << "Could not create worker Job Object" << GetLastError();
        return;
    }
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
    limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE |
                                              JOB_OBJECT_LIMIT_ACTIVE_PROCESS |
                                              JOB_OBJECT_LIMIT_PROCESS_MEMORY;
    limits.BasicLimitInformation.ActiveProcessLimit = 1;
    limits.ProcessMemoryLimit = static_cast<SIZE_T>(512ULL * 1024ULL * 1024ULL);
    if (!SetInformationJobObject(job, JobObjectExtendedLimitInformation, &limits, sizeof(limits))) {
        qWarning() << "Could not configure worker Job Object" << GetLastError();
        CloseHandle(job);
        return;
    }
    HANDLE process =
        OpenProcess(PROCESS_SET_QUOTA | PROCESS_TERMINATE | PROCESS_QUERY_LIMITED_INFORMATION,
                    FALSE, static_cast<DWORD>(process_->processId()));
    if (process == nullptr || !AssignProcessToJobObject(job, process)) {
        qWarning() << "Could not assign worker to Job Object" << GetLastError();
        if (process != nullptr) {
            CloseHandle(process);
        }
        CloseHandle(job);
        return;
    }
    CloseHandle(process);
    jobHandle_ = job;
#endif
}

} // namespace vestra::sandbox

