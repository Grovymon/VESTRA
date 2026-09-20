#pragma once

#include "vestra/document/pdf_engine.h"

#include <QMainWindow>

#include <memory>

class QLabel;
class QPushButton;
class QScrollArea;

namespace vestra::sandbox {
class WorkerSession;
}

class PdfWindow final : public QMainWindow {
    Q_OBJECT

  public:
    explicit PdfWindow(QWidget* parent = nullptr);
    void requestOpen(const QString& path, bool protectedViewHint = false);

  private:
    void buildUi();
    void retranslate();
    void openTrusted(const QString& path, bool protectedView);
    void renderCurrentPage();
    void changePage(int delta);
    void changeZoom(double multiplier);

    std::unique_ptr<vestra::document::PdfEngine> engine_;
    vestra::sandbox::WorkerSession* worker_{nullptr};
    QLabel* pageImage_{nullptr};
    QLabel* stateMessage_{nullptr};
    QLabel* pageStatus_{nullptr};
    QLabel* protectedBanner_{nullptr};
    QPushButton* openButton_{nullptr};
    QPushButton* previousButton_{nullptr};
    QPushButton* nextButton_{nullptr};
    QPushButton* zoomOutButton_{nullptr};
    QPushButton* zoomInButton_{nullptr};
    QScrollArea* scrollArea_{nullptr};
    QString currentPath_;
    int currentPage_{0};
    double zoom_{1.0};
    bool protectedViewHint_{false};
};

