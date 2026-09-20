#pragma once

#include <QList>
#include <QMainWindow>

class QGraphicsScene;
class QGraphicsView;
class QLabel;
class QListWidget;
class QPushButton;
class QToolButton;

namespace vestra::sandbox {
class WorkerSession;
}

namespace vestra::ui {
class OfficeRibbon;
}

class SlidesWindow final : public QMainWindow {
    Q_OBJECT

  public:
    explicit SlidesWindow(QWidget* parent = nullptr);
    void requestOpen(const QString& path, bool protectedViewHint = false);

  private:
    void buildUi();
    void buildRibbon();
    void retranslate();
    void addSlide(bool titleLayout = false);
    void duplicateSlide();
    void deleteSlide();
    void moveSlide(int delta);
    void selectSlide(int index);
    void updateThumbnails();
    void insertText();
    void insertImage();
    void insertShape();
    void chooseBackground();
    void startSlideShow();
    void exportPdf();
    void openFile();
    void saveFile();
    bool writePresentation(const QString& path);
    bool readPresentation(const QString& path);
    void openTrusted(const QString& path, bool protectedView);
    void setProtectedView(bool enabled);
    void updateTitle();

    vestra::sandbox::WorkerSession* worker_{nullptr};
    vestra::ui::OfficeRibbon* ribbon_{nullptr};
    QListWidget* thumbnails_{nullptr};
    QGraphicsView* canvas_{nullptr};
    QList<QGraphicsScene*> scenes_;
    QWidget* protectedBanner_{nullptr};
    QLabel* protectedMessage_{nullptr};
    QPushButton* enableEditing_{nullptr};
    QLabel* status_{nullptr};
    QToolButton* openButton_{nullptr};
    QToolButton* saveButton_{nullptr};
    QToolButton* newSlideButton_{nullptr};
    QToolButton* duplicateButton_{nullptr};
    QToolButton* deleteButton_{nullptr};
    QToolButton* moveUpButton_{nullptr};
    QToolButton* moveDownButton_{nullptr};
    QToolButton* textButton_{nullptr};
    QToolButton* imageButton_{nullptr};
    QToolButton* shapeButton_{nullptr};
    QToolButton* backgroundButton_{nullptr};
    QToolButton* slideShowButton_{nullptr};
    QToolButton* exportPdfButton_{nullptr};
    QString currentPath_;
    bool protectedViewHint_{false};
};

