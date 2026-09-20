#include "pdf_window.h"

#include "vestra/document/engine_factory.h"
#include "vestra/sandbox/worker_session.h"
#include "vestra/ui/translation_manager.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QTabWidget>
#include <QVBoxLayout>

#include <algorithm>

using vestra::ui::t;

PdfWindow::PdfWindow(QWidget* parent)
    : QMainWindow(parent), engine_(vestra::document::createPdfEngine()) {
    worker_ = new vestra::sandbox::WorkerSession(this);
    buildUi();
    retranslate();
    connect(&vestra::ui::TranslationManager::instance(),
            &vestra::ui::TranslationManager::languageChanged, this, &PdfWindow::retranslate);
    connect(worker_, &vestra::sandbox::WorkerSession::accepted, this,
            [this](const QString& path, const bool protectedView, const bool macros,
                   const QStringList&) {
                if (macros) {
                    QMessageBox::warning(this, t("macro.title"), t("macro.message"));
                }
                openTrusted(path, protectedViewHint_ || protectedView || macros);
            });
    connect(worker_, &vestra::sandbox::WorkerSession::rejected, this,
            [this](const QString&, const QString& code, const QStringList& findings) {
                stateMessage_->setText(t("launcher.blocked"));
                QMessageBox::critical(this, t("common.error"),
                                      t("launcher.blocked") + QStringLiteral("\n\n") + code +
                                          QStringLiteral("\n") + findings.join(QLatin1Char('\n')));
            });
    connect(worker_, &vestra::sandbox::WorkerSession::failed, this,
            [this](const QString&, const QString& detail) {
                stateMessage_->setText(t("launcher.worker_failed"));
                QMessageBox::critical(this, t("common.error"), detail);
            });
}

void PdfWindow::buildUi() {
    resize(1000, 760);
    setMinimumSize(700, 500);
    auto* central = new QWidget;
    auto* root = new QVBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto* titleBar = new QFrame;
    titleBar->setObjectName(QStringLiteral("TitleBar"));
    titleBar->setFixedHeight(40);
    auto* titleRow = new QHBoxLayout(titleBar);
    titleRow->setContentsMargins(14, 5, 14, 5);
    auto* badge = new QLabel(QStringLiteral("PDF"));
    badge->setAlignment(Qt::AlignCenter);
    badge->setFixedSize(42, 28);
    badge->setStyleSheet(QStringLiteral(
        "background:#b33c48;color:white;font-weight:700;border-radius:5px;font-size:12px;"));
    auto* product = new QLabel(QStringLiteral("Vestra PDF"));
    product->setStyleSheet(QStringLiteral("font-weight:600;font-size:14px;"));
    titleRow->addWidget(badge);
    titleRow->addWidget(product);
    titleRow->addStretch();
    root->addWidget(titleBar);

    auto* ribbon = new QTabWidget;
    ribbon->setObjectName(QStringLiteral("OfficeRibbon"));
    ribbon->setDocumentMode(true);
    ribbon->setMinimumHeight(102);
    ribbon->setMaximumHeight(118);
    auto* homePage = new QWidget;
    homePage->setObjectName(QStringLiteral("RibbonPage"));
    auto* toolbar = new QHBoxLayout;
    toolbar->setContentsMargins(12, 10, 12, 10);
    openButton_ = new QPushButton;
    previousButton_ = new QPushButton;
    nextButton_ = new QPushButton;
    zoomOutButton_ = new QPushButton(QStringLiteral("−"));
    zoomInButton_ = new QPushButton(QStringLiteral("+"));
    pageStatus_ = new QLabel;
    toolbar->addWidget(openButton_);
    toolbar->addSpacing(12);
    toolbar->addWidget(previousButton_);
    toolbar->addWidget(pageStatus_);
    toolbar->addWidget(nextButton_);
    toolbar->addStretch();
    toolbar->addWidget(zoomOutButton_);
    toolbar->addWidget(zoomInButton_);
    homePage->setLayout(toolbar);
    ribbon->addTab(homePage, t("ribbon.home"));
    auto* reviewPage = new QWidget;
    reviewPage->setObjectName(QStringLiteral("RibbonPage"));
    auto* reviewLayout = new QHBoxLayout(reviewPage);
    auto* reviewNote = new QLabel(t("pdf.annotations_later"));
    reviewLayout->addWidget(reviewNote);
    reviewLayout->addStretch();
    ribbon->addTab(reviewPage, t("ribbon.review"));
    root->addWidget(ribbon);

    protectedBanner_ = new QLabel;
    protectedBanner_->setStyleSheet(
        QStringLiteral("background:#f5c84c;color:#211b05;font-weight:600;padding:10px 16px;"));
    protectedBanner_->hide();
    root->addWidget(protectedBanner_);
    stateMessage_ = new QLabel;
    stateMessage_->setAlignment(Qt::AlignCenter);
    stateMessage_->setWordWrap(true);
    stateMessage_->setStyleSheet(QStringLiteral("font-size:16px;padding:40px;"));
    root->addWidget(stateMessage_);
    pageImage_ = new QLabel;
    pageImage_->setAlignment(Qt::AlignCenter);
    pageImage_->setStyleSheet(QStringLiteral("background:#74777c;padding:24px;"));
    scrollArea_ = new QScrollArea;
    scrollArea_->setWidget(pageImage_);
    scrollArea_->setWidgetResizable(true);
    scrollArea_->hide();
    root->addWidget(scrollArea_, 1);
    setCentralWidget(central);

    connect(openButton_, &QPushButton::clicked, this, [this] {
        const QString path =
            QFileDialog::getOpenFileName(this, t("pdf.open"), QString(), t("launcher.pdf_filter"));
        if (!path.isEmpty()) {
            requestOpen(path);
        }
    });
    connect(previousButton_, &QPushButton::clicked, this, [this] { changePage(-1); });
    connect(nextButton_, &QPushButton::clicked, this, [this] { changePage(1); });
    connect(zoomOutButton_, &QPushButton::clicked, this, [this] { changeZoom(0.8); });
    connect(zoomInButton_, &QPushButton::clicked, this, [this] { changeZoom(1.25); });
}

void PdfWindow::retranslate() {
    setWindowTitle(
        currentPath_.isEmpty()
            ? t("pdf.title")
            : QStringLiteral("%1 — %2").arg(QFileInfo(currentPath_).fileName(), t("pdf.title")));
    openButton_->setText(t("pdf.open"));
    previousButton_->setText(t("pdf.previous"));
    nextButton_->setText(t("pdf.next"));
    zoomOutButton_->setToolTip(t("writer.zoom_out"));
    zoomInButton_->setToolTip(t("writer.zoom_in"));
    protectedBanner_->setText(t("protected.title") + QStringLiteral(" — ") +
                              t("protected.message"));
    if (engine_->pageCount() == 0) {
        stateMessage_->setText(engine_->info().runtimeReady ? QString()
                                                            : t("pdf.backend_unavailable"));
        pageStatus_->clear();
    } else {
        pageStatus_->setText(t("pdf.page").arg(currentPage_ + 1).arg(engine_->pageCount()));
    }
    previousButton_->setEnabled(engine_->pageCount() > 0 && currentPage_ > 0);
    nextButton_->setEnabled(engine_->pageCount() > 0 && currentPage_ + 1 < engine_->pageCount());
    zoomOutButton_->setEnabled(engine_->pageCount() > 0);
    zoomInButton_->setEnabled(engine_->pageCount() > 0);
}

void PdfWindow::requestOpen(const QString& path, const bool protectedViewHint) {
    protectedViewHint_ = protectedViewHint;
    stateMessage_->setText(t("writer.worker_check"));
    QString error;
    if (!worker_->inspect(path, &error)) {
        stateMessage_->setText(t("launcher.worker_failed"));
        QMessageBox::critical(this, t("common.error"), error);
    }
}

void PdfWindow::openTrusted(const QString& path, const bool protectedView) {
    const vestra::document::OperationResult result = engine_->open(path);
    if (!result.ok()) {
        stateMessage_->setText(result.error == vestra::document::EngineError::BackendUnavailable
                                   ? t("pdf.backend_unavailable")
                                   : t("pdf.open_failed") + QStringLiteral("\n") + result.detail);
        scrollArea_->hide();
        stateMessage_->show();
        retranslate();
        return;
    }
    currentPath_ = QFileInfo(path).absoluteFilePath();
    currentPage_ = 0;
    zoom_ = 1.0;
    protectedBanner_->setVisible(protectedView);
    stateMessage_->hide();
    scrollArea_->show();
    renderCurrentPage();
    retranslate();
}

void PdfWindow::renderCurrentPage() {
    QSizeF points = engine_->pageSizePoints(currentPage_);
    if (points.isEmpty()) {
        points = QSizeF(612, 792);
    }
    const int baseWidth = std::max(320, scrollArea_->viewport()->width() - 70);
    const int width = static_cast<int>(baseWidth * zoom_);
    const int height = static_cast<int>(width * points.height() / points.width());
    QImage image;
    const vestra::document::OperationResult result =
        engine_->renderPage(currentPage_, QSize(width, height), &image);
    if (!result.ok()) {
        stateMessage_->setText(t("pdf.open_failed") + QStringLiteral("\n") + result.detail);
        stateMessage_->show();
        scrollArea_->hide();
        return;
    }
    pageImage_->setPixmap(QPixmap::fromImage(image));
    pageImage_->resize(image.size());
}

void PdfWindow::changePage(const int delta) {
    const int next = std::clamp(currentPage_ + delta, 0, std::max(0, engine_->pageCount() - 1));
    if (next != currentPage_) {
        currentPage_ = next;
        renderCurrentPage();
        retranslate();
    }
}

void PdfWindow::changeZoom(const double multiplier) {
    zoom_ = std::clamp(zoom_ * multiplier, 0.25, 4.0);
    renderCurrentPage();
}

