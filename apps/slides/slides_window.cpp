#include "slides_window.h"

#include "vestra/sandbox/worker_session.h"
#include "vestra/ui/office_ribbon.h"
#include "vestra/ui/translation_manager.h"

#include <QBuffer>
#include <QColorDialog>
#include <QDialog>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QGraphicsItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPainter>
#include <QPdfWriter>
#include <QPushButton>
#include <QSaveFile>
#include <QStatusBar>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>

using vestra::ui::makeRibbonButton;
using vestra::ui::t;

namespace {

constexpr qreal kSlideWidth = 960.0;
constexpr qreal kSlideHeight = 540.0;

QPixmap renderScene(QGraphicsScene* scene, const QSize& size) {
    QPixmap image(size);
    image.fill(Qt::white);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    scene->render(&painter, QRectF(QPointF(0, 0), QSizeF(size)), scene->sceneRect());
    return image;
}

class SlideShowDialog final : public QDialog {
  public:
    SlideShowDialog(const QList<QGraphicsScene*>& scenes, int startIndex, QWidget* parent)
        : QDialog(parent), scenes_(scenes), index_(std::max(0, startIndex)) {
        setWindowFlag(Qt::FramelessWindowHint);
        setStyleSheet(QStringLiteral("background:black;"));
        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        image_ = new QLabel;
        image_->setAlignment(Qt::AlignCenter);
        layout->addWidget(image_);
        updateSlide();
    }

  protected:
    void keyPressEvent(QKeyEvent* event) override {
        if (event->key() == Qt::Key_Escape) {
            close();
        } else if (event->key() == Qt::Key_Right || event->key() == Qt::Key_Space ||
                   event->key() == Qt::Key_PageDown) {
            index_ = std::min(index_ + 1, static_cast<int>(scenes_.size()) - 1);
            updateSlide();
        } else if (event->key() == Qt::Key_Left || event->key() == Qt::Key_PageUp) {
            index_ = std::max(0, index_ - 1);
            updateSlide();
        }
    }

    void resizeEvent(QResizeEvent* event) override {
        QDialog::resizeEvent(event);
        updateSlide();
    }

  private:
    void updateSlide() {
        if (scenes_.isEmpty() || size().isEmpty()) {
            return;
        }
        const QSize available = size().isEmpty() ? QSize(1280, 720) : size();
        image_->setPixmap(renderScene(scenes_[index_], available));
    }

    QList<QGraphicsScene*> scenes_;
    QLabel* image_{nullptr};
    int index_{0};
};

QJsonObject serializeItem(QGraphicsItem* item) {
    QJsonObject object;
    object.insert(QStringLiteral("x"), item->pos().x());
    object.insert(QStringLiteral("y"), item->pos().y());
    if (auto* text = qgraphicsitem_cast<QGraphicsTextItem*>(item)) {
        object.insert(QStringLiteral("type"), QStringLiteral("text"));
        object.insert(QStringLiteral("text"), text->toPlainText());
        object.insert(QStringLiteral("html"), text->toHtml());
    } else if (auto* rectangle = qgraphicsitem_cast<QGraphicsRectItem*>(item)) {
        object.insert(QStringLiteral("type"), QStringLiteral("rect"));
        object.insert(QStringLiteral("width"), rectangle->rect().width());
        object.insert(QStringLiteral("height"), rectangle->rect().height());
        object.insert(QStringLiteral("color"), rectangle->brush().color().name(QColor::HexArgb));
    } else if (auto* pixmap = qgraphicsitem_cast<QGraphicsPixmapItem*>(item)) {
        object.insert(QStringLiteral("type"), QStringLiteral("image"));
        QByteArray bytes;
        QBuffer buffer(&bytes);
        buffer.open(QIODevice::WriteOnly);
        pixmap->pixmap().save(&buffer, "PNG");
        object.insert(QStringLiteral("data"), QString::fromLatin1(bytes.toBase64()));
    }
    return object;
}

} // namespace

SlidesWindow::SlidesWindow(QWidget* parent) : QMainWindow(parent) {
    worker_ = new vestra::sandbox::WorkerSession(this);
    buildUi();
    addSlide(true);
    retranslate();

    connect(&vestra::ui::TranslationManager::instance(),
            &vestra::ui::TranslationManager::languageChanged, this, &SlidesWindow::retranslate);
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
                QMessageBox::critical(this, t("common.error"),
                                      t("launcher.blocked") + QStringLiteral("\n\n") + code +
                                          QStringLiteral("\n") + findings.join(u'\n'));
            });
    connect(worker_, &vestra::sandbox::WorkerSession::failed, this,
            [this](const QString&, const QString& detail) {
                QMessageBox::critical(this, t("common.error"), detail);
            });
}

void SlidesWindow::buildUi() {
    resize(1240, 820);
    setMinimumSize(850, 580);
    auto* central = new QWidget;
    auto* root = new QVBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto* titleBar = new QFrame;
    titleBar->setObjectName(QStringLiteral("TitleBar"));
    titleBar->setFixedHeight(40);
    auto* titleRow = new QHBoxLayout(titleBar);
    titleRow->setContentsMargins(14, 5, 14, 5);
    auto* badge = new QLabel(QStringLiteral("P"));
    badge->setAlignment(Qt::AlignCenter);
    badge->setFixedSize(28, 28);
    badge->setStyleSheet(QStringLiteral(
        "background:#d45124;color:white;font-weight:700;border-radius:5px;font-size:16px;"));
    auto* product = new QLabel(QStringLiteral("Vestra Slides"));
    product->setStyleSheet(QStringLiteral("font-weight:600;font-size:14px;"));
    titleRow->addWidget(badge);
    titleRow->addWidget(product);
    titleRow->addStretch();
    root->addWidget(titleBar);

    buildRibbon();
    root->addWidget(ribbon_);

    protectedBanner_ = new QWidget;
    protectedBanner_->setStyleSheet(QStringLiteral("background:#f6cc54;color:#211b05;"));
    auto* protectedRow = new QHBoxLayout(protectedBanner_);
    protectedMessage_ = new QLabel;
    protectedMessage_->setStyleSheet(QStringLiteral("font-weight:600;"));
    enableEditing_ = new QPushButton;
    protectedRow->addWidget(protectedMessage_);
    protectedRow->addStretch();
    protectedRow->addWidget(enableEditing_);
    protectedBanner_->hide();
    root->addWidget(protectedBanner_);

    auto* workspace = new QWidget;
    workspace->setObjectName(QStringLiteral("Workspace"));
    auto* workspaceRow = new QHBoxLayout(workspace);
    workspaceRow->setContentsMargins(10, 10, 10, 10);
    workspaceRow->setSpacing(12);
    thumbnails_ = new QListWidget;
    thumbnails_->setIconSize(QSize(160, 90));
    thumbnails_->setFixedWidth(205);
    thumbnails_->setSpacing(6);
    canvas_ = new QGraphicsView;
    canvas_->setRenderHint(QPainter::Antialiasing);
    canvas_->setAlignment(Qt::AlignCenter);
    canvas_->setDragMode(QGraphicsView::RubberBandDrag);
    canvas_->setStyleSheet(QStringLiteral("QGraphicsView{background:#cfd2d8;border:0;}"));
    workspaceRow->addWidget(thumbnails_);
    workspaceRow->addWidget(canvas_, 1);
    root->addWidget(workspace, 1);
    setCentralWidget(central);

    status_ = new QLabel;
    statusBar()->addWidget(status_, 1);
    connect(thumbnails_, &QListWidget::currentRowChanged, this, &SlidesWindow::selectSlide);
    connect(enableEditing_, &QPushButton::clicked, this, [this] { setProtectedView(false); });
}

void SlidesWindow::buildRibbon() {
    ribbon_ = new vestra::ui::OfficeRibbon;
    openButton_ = makeRibbonButton();
    saveButton_ = makeRibbonButton();
    newSlideButton_ = makeRibbonButton();
    duplicateButton_ = makeRibbonButton();
    deleteButton_ = makeRibbonButton();
    moveUpButton_ = makeRibbonButton();
    moveDownButton_ = makeRibbonButton();
    textButton_ = makeRibbonButton();
    imageButton_ = makeRibbonButton();
    shapeButton_ = makeRibbonButton();
    backgroundButton_ = makeRibbonButton();
    slideShowButton_ = makeRibbonButton();
    exportPdfButton_ = makeRibbonButton();

    ribbon_->addRibbonTab(
        QStringLiteral("ribbon.file"),
        {{QStringLiteral("group.file"), {openButton_, saveButton_, exportPdfButton_}}});
    ribbon_->addRibbonTab(
        QStringLiteral("ribbon.home"),
        {{QStringLiteral("group.slides"), {newSlideButton_, duplicateButton_, deleteButton_}},
         {QStringLiteral("group.arrange"), {moveUpButton_, moveDownButton_}}});
    ribbon_->addRibbonTab(
        QStringLiteral("ribbon.insert"),
        {{QStringLiteral("group.insert"), {textButton_, imageButton_, shapeButton_}}});
    ribbon_->addRibbonTab(QStringLiteral("ribbon.layout"),
                          {{QStringLiteral("group.background"), {backgroundButton_}}});
    ribbon_->addRibbonTab(QStringLiteral("ribbon.review"), {});
    ribbon_->addRibbonTab(QStringLiteral("ribbon.view"),
                          {{QStringLiteral("group.presentation"), {slideShowButton_}}});
    ribbon_->setCurrentIndex(1);

    connect(openButton_, &QToolButton::clicked, this, &SlidesWindow::openFile);
    connect(saveButton_, &QToolButton::clicked, this, &SlidesWindow::saveFile);
    connect(exportPdfButton_, &QToolButton::clicked, this, &SlidesWindow::exportPdf);
    connect(newSlideButton_, &QToolButton::clicked, this, [this] { addSlide(); });
    connect(duplicateButton_, &QToolButton::clicked, this, &SlidesWindow::duplicateSlide);
    connect(deleteButton_, &QToolButton::clicked, this, &SlidesWindow::deleteSlide);
    connect(moveUpButton_, &QToolButton::clicked, this, [this] { moveSlide(-1); });
    connect(moveDownButton_, &QToolButton::clicked, this, [this] { moveSlide(1); });
    connect(textButton_, &QToolButton::clicked, this, &SlidesWindow::insertText);
    connect(imageButton_, &QToolButton::clicked, this, &SlidesWindow::insertImage);
    connect(shapeButton_, &QToolButton::clicked, this, &SlidesWindow::insertShape);
    connect(backgroundButton_, &QToolButton::clicked, this, &SlidesWindow::chooseBackground);
    connect(slideShowButton_, &QToolButton::clicked, this, &SlidesWindow::startSlideShow);
}

void SlidesWindow::retranslate() {
    ribbon_->retranslate();
    openButton_->setText(t("common.open"));
    saveButton_->setText(t("common.save"));
    newSlideButton_->setText(t("slides.new_slide"));
    duplicateButton_->setText(t("slides.duplicate"));
    deleteButton_->setText(t("slides.delete"));
    moveUpButton_->setText(t("slides.move_up"));
    moveDownButton_->setText(t("slides.move_down"));
    textButton_->setText(t("slides.text"));
    imageButton_->setText(t("writer.image"));
    shapeButton_->setText(t("slides.shape"));
    backgroundButton_->setText(t("slides.background"));
    slideShowButton_->setText(t("slides.slide_show"));
    exportPdfButton_->setText(t("writer.export_pdf"));
    protectedMessage_->setText(t("protected.title") + QStringLiteral(" — ") +
                               t("protected.message"));
    enableEditing_->setText(t("protected.enable_editing"));
    status_->setText(t("slides.status").arg(scenes_.size()));
    updateTitle();
}

void SlidesWindow::addSlide(const bool titleLayout) {
    auto* scene = new QGraphicsScene(this);
    scene->setSceneRect(0, 0, kSlideWidth, kSlideHeight);
    scene->setBackgroundBrush(Qt::white);
    if (titleLayout) {
        auto* title =
            scene->addText(t("slides.title_placeholder"), QFont(QString(), 28, QFont::Bold));
        title->setDefaultTextColor(QColor(QStringLiteral("#202124")));
        title->setTextInteractionFlags(Qt::TextEditorInteraction);
        title->setTextWidth(760);
        title->setPos(100, 135);
        title->setFlag(QGraphicsItem::ItemIsMovable);
        title->setFlag(QGraphicsItem::ItemIsSelectable);
        auto* subtitle = scene->addText(t("slides.subtitle_placeholder"), QFont(QString(), 17));
        subtitle->setDefaultTextColor(QColor(QStringLiteral("#3c4043")));
        subtitle->setTextInteractionFlags(Qt::TextEditorInteraction);
        subtitle->setTextWidth(760);
        subtitle->setPos(100, 250);
        subtitle->setFlag(QGraphicsItem::ItemIsMovable);
        subtitle->setFlag(QGraphicsItem::ItemIsSelectable);
    }
    const int index =
        thumbnails_->currentRow() < 0 ? scenes_.size() : thumbnails_->currentRow() + 1;
    scenes_.insert(index, scene);
    thumbnails_->insertItem(index, new QListWidgetItem);
    updateThumbnails();
    thumbnails_->setCurrentRow(index);
    status_->setText(t("slides.status").arg(scenes_.size()));
}

void SlidesWindow::duplicateSlide() {
    const int index = thumbnails_->currentRow();
    if (index < 0) {
        return;
    }
    QGraphicsScene* source = scenes_[index];
    auto* duplicate = new QGraphicsScene(this);
    duplicate->setSceneRect(source->sceneRect());
    duplicate->setBackgroundBrush(source->backgroundBrush());
    for (QGraphicsItem* item : source->items(Qt::AscendingOrder)) {
        const QJsonObject data = serializeItem(item);
        const QString type = data.value(QStringLiteral("type")).toString();
        QGraphicsItem* copy = nullptr;
        if (type == QStringLiteral("text")) {
            auto* text = new QGraphicsTextItem;
            text->setHtml(data.value(QStringLiteral("html")).toString());
            text->setTextInteractionFlags(Qt::TextEditorInteraction);
            copy = text;
        } else if (type == QStringLiteral("rect")) {
            copy = new QGraphicsRectItem(0, 0, data.value(QStringLiteral("width")).toDouble(),
                                         data.value(QStringLiteral("height")).toDouble());
            static_cast<QGraphicsRectItem*>(copy)->setBrush(
                QColor(data.value(QStringLiteral("color")).toString()));
        } else if (type == QStringLiteral("image")) {
            QPixmap pixmap;
            pixmap.loadFromData(
                QByteArray::fromBase64(data.value(QStringLiteral("data")).toString().toLatin1()));
            copy = new QGraphicsPixmapItem(pixmap);
        }
        if (copy != nullptr) {
            copy->setPos(data.value(QStringLiteral("x")).toDouble(),
                         data.value(QStringLiteral("y")).toDouble());
            copy->setFlag(QGraphicsItem::ItemIsMovable);
            copy->setFlag(QGraphicsItem::ItemIsSelectable);
            duplicate->addItem(copy);
        }
    }
    scenes_.insert(index + 1, duplicate);
    thumbnails_->insertItem(index + 1, new QListWidgetItem);
    updateThumbnails();
    thumbnails_->setCurrentRow(index + 1);
}

void SlidesWindow::deleteSlide() {
    const int index = thumbnails_->currentRow();
    if (index < 0 || scenes_.size() <= 1) {
        return;
    }
    QGraphicsScene* scene = scenes_.takeAt(index);
    delete thumbnails_->takeItem(index);
    scene->deleteLater();
    thumbnails_->setCurrentRow(std::min(index, static_cast<int>(scenes_.size()) - 1));
    updateThumbnails();
}

void SlidesWindow::moveSlide(const int delta) {
    const int from = thumbnails_->currentRow();
    const int to = std::clamp(from + delta, 0, static_cast<int>(scenes_.size()) - 1);
    if (from < 0 || from == to) {
        return;
    }
    scenes_.move(from, to);
    updateThumbnails();
    thumbnails_->setCurrentRow(to);
}

void SlidesWindow::selectSlide(const int index) {
    if (index >= 0 && index < scenes_.size()) {
        canvas_->setScene(scenes_[index]);
        canvas_->fitInView(scenes_[index]->sceneRect(), Qt::KeepAspectRatio);
    }
}

void SlidesWindow::updateThumbnails() {
    while (thumbnails_->count() < scenes_.size()) {
        thumbnails_->addItem(new QListWidgetItem);
    }
    while (thumbnails_->count() > scenes_.size()) {
        delete thumbnails_->takeItem(thumbnails_->count() - 1);
    }
    for (int index = 0; index < scenes_.size(); ++index) {
        QListWidgetItem* item = thumbnails_->item(index);
        item->setText(QString::number(index + 1));
        item->setIcon(QIcon(renderScene(scenes_[index], QSize(160, 90))));
    }
}

void SlidesWindow::insertText() {
    if (protectedBanner_->isVisible() || canvas_->scene() == nullptr) {
        return;
    }
    auto* text = canvas_->scene()->addText(t("slides.text_placeholder"), QFont(QString(), 22));
    text->setDefaultTextColor(QColor(QStringLiteral("#202124")));
    text->setTextInteractionFlags(Qt::TextEditorInteraction);
    text->setTextWidth(420);
    text->setPos(260, 220);
    text->setFlag(QGraphicsItem::ItemIsMovable);
    text->setFlag(QGraphicsItem::ItemIsSelectable);
    text->setFocus();
    updateThumbnails();
}

void SlidesWindow::insertImage() {
    const QString path = QFileDialog::getOpenFileName(this, t("dialog.select_image"), QString(),
                                                      t("dialog.image_filter"));
    QPixmap image(path);
    if (!path.isEmpty() && !image.isNull() && canvas_->scene() != nullptr) {
        image = image.scaled(440, 300, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        auto* item = canvas_->scene()->addPixmap(image);
        item->setPos(260, 120);
        item->setFlag(QGraphicsItem::ItemIsMovable);
        item->setFlag(QGraphicsItem::ItemIsSelectable);
        updateThumbnails();
    }
}

void SlidesWindow::insertShape() {
    if (protectedBanner_->isVisible() || canvas_->scene() == nullptr) {
        return;
    }
    auto* shape = canvas_->scene()->addRect(0, 0, 230, 130, QPen(QColor("#2f5fbf"), 3),
                                            QBrush(QColor("#dbe7ff")));
    shape->setPos(365, 205);
    shape->setFlag(QGraphicsItem::ItemIsMovable);
    shape->setFlag(QGraphicsItem::ItemIsSelectable);
    updateThumbnails();
}

void SlidesWindow::chooseBackground() {
    if (canvas_->scene() == nullptr) {
        return;
    }
    const QColor color = QColorDialog::getColor(canvas_->scene()->backgroundBrush().color(), this,
                                                t("slides.background"));
    if (color.isValid()) {
        canvas_->scene()->setBackgroundBrush(color);
        updateThumbnails();
    }
}

void SlidesWindow::startSlideShow() {
    if (scenes_.isEmpty()) {
        return;
    }
    SlideShowDialog dialog(scenes_, thumbnails_->currentRow(), this);
    dialog.showFullScreen();
    dialog.exec();
}

void SlidesWindow::exportPdf() {
    const QString path = QFileDialog::getSaveFileName(
        this, t("dialog.export_pdf"), QStringLiteral("Presentation.pdf"), t("pdf.save_filter"));
    if (path.isEmpty()) {
        return;
    }
    QPdfWriter writer(path);
    writer.setResolution(96);
    writer.setPageSize(QPageSize(QSizeF(10, 5.625), QPageSize::Inch));
    writer.setPageMargins(QMarginsF());
    QPainter painter(&writer);
    for (int index = 0; index < scenes_.size(); ++index) {
        if (index > 0) {
            writer.newPage();
        }
        scenes_[index]->render(&painter, QRectF(0, 0, writer.width(), writer.height()),
                               scenes_[index]->sceneRect());
    }
    status_->setText(t("writer.exported"));
}

void SlidesWindow::openFile() {
    const QString path =
        QFileDialog::getOpenFileName(this, t("common.open"), QString(), t("slides.open_filter"));
    if (!path.isEmpty()) {
        requestOpen(path);
    }
}

void SlidesWindow::saveFile() {
    if (protectedBanner_->isVisible()) {
        return;
    }
    QString path = currentPath_;
    if (path.isEmpty()) {
        path = QFileDialog::getSaveFileName(this, t("common.save"),
                                            QStringLiteral("Presentation.vestra-slides"),
                                            t("slides.save_filter"));
    }
    if (!path.isEmpty() && writePresentation(path)) {
        currentPath_ = QFileInfo(path).absoluteFilePath();
        status_->setText(t("slides.saved"));
        updateTitle();
    }
}

bool SlidesWindow::writePresentation(const QString& path) {
    QJsonArray slides;
    for (QGraphicsScene* scene : scenes_) {
        QJsonObject slide;
        slide.insert(QStringLiteral("background"),
                     scene->backgroundBrush().color().name(QColor::HexArgb));
        QJsonArray items;
        for (QGraphicsItem* item : scene->items(Qt::AscendingOrder)) {
            const QJsonObject serialized = serializeItem(item);
            if (serialized.contains(QStringLiteral("type"))) {
                items.append(serialized);
            }
        }
        slide.insert(QStringLiteral("items"), items);
        slides.append(slide);
    }
    QJsonObject root{{QStringLiteral("format"), QStringLiteral("vestra-slides-1")},
                     {QStringLiteral("slides"), slides}};
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) ||
        file.write(QJsonDocument(root).toJson(QJsonDocument::Indented)) < 0 || !file.commit()) {
        QMessageBox::critical(this, t("common.error"), file.errorString());
        return false;
    }
    return true;
}

bool SlidesWindow::readPresentation(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    const QJsonObject root = document.object();
    if (root.value(QStringLiteral("format")).toString() != QStringLiteral("vestra-slides-1")) {
        return false;
    }
    for (QGraphicsScene* scene : scenes_) {
        scene->deleteLater();
    }
    scenes_.clear();
    thumbnails_->clear();
    const QJsonArray slides = root.value(QStringLiteral("slides")).toArray();
    for (const QJsonValue& slideValue : slides) {
        const QJsonObject slide = slideValue.toObject();
        auto* scene = new QGraphicsScene(this);
        scene->setSceneRect(0, 0, kSlideWidth, kSlideHeight);
        scene->setBackgroundBrush(QColor(slide.value(QStringLiteral("background")).toString()));
        for (const QJsonValue& itemValue : slide.value(QStringLiteral("items")).toArray()) {
            const QJsonObject data = itemValue.toObject();
            const QString type = data.value(QStringLiteral("type")).toString();
            QGraphicsItem* item = nullptr;
            if (type == QStringLiteral("text")) {
                auto* text = new QGraphicsTextItem;
                text->setHtml(data.value(QStringLiteral("html")).toString());
                text->setTextInteractionFlags(Qt::TextEditorInteraction);
                item = text;
            } else if (type == QStringLiteral("rect")) {
                auto* rectangle =
                    new QGraphicsRectItem(0, 0, data.value(QStringLiteral("width")).toDouble(),
                                          data.value(QStringLiteral("height")).toDouble());
                rectangle->setBrush(QColor(data.value(QStringLiteral("color")).toString()));
                item = rectangle;
            } else if (type == QStringLiteral("image")) {
                QPixmap pixmap;
                pixmap.loadFromData(QByteArray::fromBase64(
                    data.value(QStringLiteral("data")).toString().toLatin1()));
                item = new QGraphicsPixmapItem(pixmap);
            }
            if (item != nullptr) {
                item->setPos(data.value(QStringLiteral("x")).toDouble(),
                             data.value(QStringLiteral("y")).toDouble());
                item->setFlag(QGraphicsItem::ItemIsMovable);
                item->setFlag(QGraphicsItem::ItemIsSelectable);
                scene->addItem(item);
            }
        }
        scenes_.append(scene);
    }
    if (scenes_.isEmpty()) {
        addSlide(true);
    }
    updateThumbnails();
    thumbnails_->setCurrentRow(0);
    return true;
}

void SlidesWindow::requestOpen(const QString& path, const bool protectedViewHint) {
    protectedViewHint_ = protectedViewHint;
    QString error;
    if (!worker_->inspect(path, &error)) {
        QMessageBox::critical(this, t("common.error"), error);
    }
}

void SlidesWindow::openTrusted(const QString& path, const bool protectedView) {
    if (!path.endsWith(QStringLiteral(".vestra-slides"), Qt::CaseInsensitive)) {
        QMessageBox::information(this, t("slides.title"), t("slides.backend_unavailable"));
        return;
    }
    if (!readPresentation(path)) {
        QMessageBox::critical(this, t("common.error"), t("slides.open_failed"));
        return;
    }
    currentPath_ = QFileInfo(path).absoluteFilePath();
    setProtectedView(protectedView);
    updateTitle();
}

void SlidesWindow::setProtectedView(const bool enabled) {
    protectedBanner_->setVisible(enabled);
    saveButton_->setEnabled(!enabled);
    newSlideButton_->setEnabled(!enabled);
    duplicateButton_->setEnabled(!enabled);
    deleteButton_->setEnabled(!enabled);
    textButton_->setEnabled(!enabled);
    imageButton_->setEnabled(!enabled);
    shapeButton_->setEnabled(!enabled);
}

void SlidesWindow::updateTitle() {
    const QString name =
        currentPath_.isEmpty() ? t("slides.presentation") : QFileInfo(currentPath_).fileName();
    setWindowTitle(QStringLiteral("%1 — %2").arg(name, t("slides.title")));
}

