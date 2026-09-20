#include "page_editor.h"

#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include <QClipboard>
#include <QFocusEvent>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QKeyEvent>
#include <QMimeData>
#include <QPainter>
#include <QScrollBar>
#include <QTextBlock>
#include <QTextDocumentFragment>
#include <QTextFrame>
#include <QTextLayout>
#include <QTimer>
#include <QWheelEvent>
#include <cmath>

namespace {
class LocalDocument final : public QTextDocument {
  public:
    using QTextDocument::QTextDocument;
  protected:
    QVariant loadResource(int, const QUrl&) override {
        // Only explicitly registered resources can be rendered. HTML cannot read a
        // local path or fetch a URL, including when it comes from the clipboard.
        return {};
    }
};
}

class WriterTextItem final : public QGraphicsTextItem {
  public:
    explicit WriterTextItem(PageEditor* editor) : owner_(editor) {
        setDocument(new LocalDocument(this));
        setTextInteractionFlags(Qt::TextEditorInteraction);
        setDefaultTextColor(QColor(28, 30, 34));
    }
  protected:
    bool sceneEvent(QEvent* event) override {
        const auto type = event->type();
        // Keep clipboard input under the same local-resource policy as toolbar paste.
        if (type == QEvent::KeyPress) {
            auto* key = static_cast<QKeyEvent*>(event);
            if (key->matches(QKeySequence::Paste)) {
                owner_->paste();
                return true;
            }
        }
        const bool handled = QGraphicsTextItem::sceneEvent(event);
        if (type == QEvent::KeyPress || type == QEvent::GraphicsSceneMouseRelease ||
            type == QEvent::GraphicsSceneMouseMove || type == QEvent::InputMethod) {
            QTimer::singleShot(0, owner_, &PageEditor::refreshCursor);
        }
        return handled;
    }
  private:
    PageEditor* owner_;
};

PageEditor::PageEditor(QWidget* parent) : QGraphicsView(parent) {
    setObjectName(QStringLiteral("WriterCanvas"));
    setFrameShape(QFrame::NoFrame);
    setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
    setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    setTransformationAnchor(QGraphicsView::NoAnchor);
    setResizeAnchor(QGraphicsView::NoAnchor);
    setAcceptDrops(false);
    auto* canvas = new QGraphicsScene(this);
    setScene(canvas);
    textItem_ = new WriterTextItem(this);
    scene()->addItem(textItem_);
    document()->setDefaultFont(QFont(QStringLiteral("Calibri"), 11));
    setPageGeometry(paper_, margins_);
    document()->setModified(false);
    connect(document(), &QTextDocument::contentsChanged, this, [this] {
        relayout();
        emit textChanged();
    });
    connect(document()->documentLayout(), &QAbstractTextDocumentLayout::documentSizeChanged,
            this, [this] { relayout(); });
    connect(verticalScrollBar(), &QScrollBar::valueChanged, this, [this] {
        viewport()->update();
    });
}

QTextDocument* PageEditor::document() const { return textItem_->document(); }
QTextCursor PageEditor::textCursor() const { return textItem_->textCursor(); }
void PageEditor::setTextCursor(const QTextCursor& cursor) {
    textItem_->setTextCursor(cursor);
    refreshCursor();
}
QString PageEditor::toPlainText() const { return document()->toPlainText(); }
void PageEditor::setPlainText(const QString& text) {
    document()->setPlainText(text);
    setPageGeometry(paper_, margins_);
    setTextCursor(QTextCursor(document()));
}
void PageEditor::setHtml(const QString& html) {
    document()->setHtml(html);
    setPageGeometry(paper_, margins_);
    setTextCursor(QTextCursor(document()));
}
void PageEditor::mergeFormat(const QTextCharFormat& format) {
    if (readOnly_) return;
    auto cursor = textCursor();
    cursor.mergeCharFormat(format);
    setTextCursor(cursor);
}
void PageEditor::setReadOnly(bool enabled) {
    readOnly_ = enabled;
    textItem_->setTextInteractionFlags(enabled ? Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard
                                               : Qt::TextEditorInteraction);
}
bool PageEditor::isReadOnly() const { return readOnly_; }
void PageEditor::setPageGeometry(const QSizeF& size, const QMarginsF& margins) {
    if (size.width() < 200 || size.height() < 200 || size.width() > 2000 || size.height() > 2000 ||
        margins.left() < 20 || margins.right() < 20 || margins.top() < 20 || margins.bottom() < 20 ||
        margins.left() + margins.right() > size.width() - 100 ||
        margins.top() + margins.bottom() > size.height() - 100) return;
    paper_ = size;
    margins_ = margins;
    auto frame = document()->rootFrame()->frameFormat();
    frame.setLeftMargin(margins.left());
    frame.setRightMargin(margins.right());
    frame.setTopMargin(margins.top());
    frame.setBottomMargin(margins.bottom());
    document()->rootFrame()->setFrameFormat(frame);
    document()->setPageSize(paper_);
    relayout();
    emit pageGeometryChanged();
}
QSizeF PageEditor::pageSize() const { return paper_; }
QMarginsF PageEditor::pageMargins() const { return margins_; }
int PageEditor::pageCount() const { return qMax(1, document()->pageCount()); }
int PageEditor::currentPage() const {
    return qBound(1, int(cursorRectangle().top() / paper_.height()) + 1, pageCount());
}
QRectF PageEditor::cursorRectangle() const {
    const auto cursor = textCursor();
    const auto block = cursor.block();
    const QRectF blockRect = document()->documentLayout()->blockBoundingRect(block);
    const QTextLayout* layout = block.layout();
    const QTextLine line = layout->lineForTextPosition(cursor.positionInBlock());
    if (!line.isValid()) return blockRect;
    return QRectF(blockRect.left() + line.cursorToX(cursor.positionInBlock()),
                  blockRect.top() + line.y(), 2, line.height());
}
int PageEditor::zoomPercent() const { return zoom_; }
void PageEditor::setZoomPercent(int percent) {
    zoom_ = qBound(50, percent, 200);
    setTransform(QTransform::fromScale(zoom_ / 100.0, zoom_ / 100.0));
    viewport()->update();
    emit zoomChanged(zoom_);
}
void PageEditor::fitWidth() { setZoomPercent(qRound((viewport()->width() - 100) * 100.0 / paper_.width())); }
void PageEditor::fitPage() {
    setZoomPercent(qRound(qMin((viewport()->width() - 100) / paper_.width(),
                              (viewport()->height() - 50) / paper_.height()) * 100));
}
void PageEditor::setRulersVisible(bool visible) { rulers_ = visible; viewport()->update(); }
void PageEditor::setFormattingMarks(bool visible) {
    auto option = document()->defaultTextOption();
    option.setFlags(visible ? QTextOption::ShowTabsAndSpaces | QTextOption::ShowLineAndParagraphSeparators
                            : QTextOption::Flags{});
    document()->setDefaultTextOption(option);
}
void PageEditor::copy() {
    const auto cursor = textCursor();
    if (!cursor.hasSelection()) return;
    auto* mime = new QMimeData;
    mime->setHtml(QTextDocumentFragment(cursor).toHtml());
    mime->setText(cursor.selectedText().replace(QChar::ParagraphSeparator, QLatin1Char('\n')));
    QApplication::clipboard()->setMimeData(mime);
}
void PageEditor::cut() {
    if (readOnly_) return;
    copy();
    auto cursor = textCursor();
    cursor.removeSelectedText();
    setTextCursor(cursor);
}
void PageEditor::paste() {
    if (readOnly_) return;
    const auto* mime = QApplication::clipboard()->mimeData();
    auto cursor = textCursor();
    if (mime->hasHtml() && mime->html().size() <= 2 * 1024 * 1024) {
        cursor.insertFragment(QTextDocumentFragment::fromHtml(mime->html(), document()));
    } else if (mime->hasText() && mime->text().size() <= 2 * 1024 * 1024) {
        cursor.insertText(mime->text());
    }
    setTextCursor(cursor);
}
void PageEditor::undo() { if (!readOnly_) { auto c = textCursor(); document()->undo(&c); setTextCursor(c); } }
void PageEditor::redo() { if (!readOnly_) { auto c = textCursor(); document()->redo(&c); setTextCursor(c); } }
void PageEditor::selectAll() { auto c = textCursor(); c.select(QTextCursor::Document); setTextCursor(c); }
bool PageEditor::find(const QString& query, QTextDocument::FindFlags flags) {
    auto cursor = document()->find(query, textCursor(), flags);
    if (cursor.isNull()) cursor = document()->find(query, flags.testFlag(QTextDocument::FindBackward)
                                                            ? document()->characterCount() - 1 : 0, flags);
    if (cursor.isNull()) return false;
    setTextCursor(cursor);
    return true;
}
void PageEditor::print(QPagedPaintDevice* device) const { document()->print(device); }
void PageEditor::refreshCursor() {
    ensureVisible(cursorRectangle(), 24, 32);
    emit cursorPositionChanged();
}
void PageEditor::relayout() {
    setSceneRect(-42, -30, paper_.width() + 84, pageCount() * paper_.height() + 60);
    viewport()->update();
}
void PageEditor::drawBackground(QPainter* painter, const QRectF& exposed) {
    const bool dark = palette().color(QPalette::Window).lightness() < 100;
    painter->fillRect(exposed, dark ? QColor("#22252b") : QColor("#e6e7e9"));
    const int first = qMax(0, int(std::floor(exposed.top() / paper_.height())));
    const int last = qMin(pageCount() - 1, int(exposed.bottom() / paper_.height()));
    for (int page = first; page <= last; ++page) {
        const QRectF sheet(0, page * paper_.height(), paper_.width(), paper_.height());
        painter->fillRect(sheet.translated(3, 4), QColor(0, 0, 0, 24));
        painter->setPen(QColor("#bfc3ca"));
        painter->setBrush(Qt::white);
        painter->drawRect(sheet);
    }
}
void PageEditor::drawForeground(QPainter* painter, const QRectF& exposed) {
    const bool dark = palette().color(QPalette::Window).lightness() < 100;
    const QColor desk = dark ? QColor("#22252b") : QColor("#e6e7e9");
    // Separators live entirely in the reserved page margins. Document coordinates
    // stay unchanged, so selection, tables, IME and exported page breaks agree.
    for (int page = qMax(1, int(exposed.top() / paper_.height()));
         page < pageCount() && page * paper_.height() < exposed.bottom() + 12; ++page) {
        const qreal y = page * paper_.height();
        painter->fillRect(QRectF(-1, y - 10, paper_.width() + 6, 20), desk);
        painter->setPen(QColor("#bfc3ca"));
        painter->drawLine(QPointF(0, y - 10), QPointF(paper_.width(), y - 10));
        painter->drawLine(QPointF(0, y + 10), QPointF(paper_.width(), y + 10));
    }
    if (!rulers_) return;
    painter->save();
    painter->resetTransform();
    const qreal scale = zoom_ / 100.0;
    const qreal x0 = mapFromScene(0, 0).x();
    const qreal y0 = mapFromScene(0, 0).y();
    const qreal pageWidth = paper_.width() * scale;
    painter->setFont(QFont(QStringLiteral("Segoe UI"), 7));
    const QColor ruleBackground = dark ? QColor("#333840") : QColor("#fafafa");
    const QColor ink = dark ? QColor("#e4e8ef") : QColor("#4b5058");
    painter->fillRect(QRectF(x0, 0, pageWidth, 20), ruleBackground);
    painter->fillRect(QRectF(0, 20, 20, viewport()->height()), ruleBackground);
    painter->setPen(ink);
    const qreal cm = 96.0 / 2.54 * scale;
    for (int tick = 0; tick * cm / 10 < pageWidth; ++tick) {
        const qreal x = x0 + tick * cm / 10;
        const int length = tick % 10 == 0 ? 7 : (tick % 5 == 0 ? 5 : 3);
        painter->drawLine(QPointF(x, 20), QPointF(x, 20 - length));
        if (tick > 0 && tick % 10 == 0) painter->drawText(QRectF(x - 10, 0, 20, 13), Qt::AlignCenter, QString::number(tick / 10));
    }
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor("#3971b9"));
    for (qreal marginX : {x0 + margins_.left() * scale, x0 + pageWidth - margins_.right() * scale}) {
        painter->drawPolygon(QPolygonF{QPointF(marginX - 4, 12), QPointF(marginX + 4, 12), QPointF(marginX, 19)});
    }
    painter->setPen(ink);
    const qreal pageHeight = paper_.height() * scale;
    for (int tick = qMax(0, int((-y0) / (cm / 10))); tick * cm / 10 + y0 < viewport()->height(); ++tick) {
        const qreal y = y0 + tick * cm / 10;
        if (y < 22) continue;
        const int length = tick % 10 == 0 ? 7 : 3;
        painter->drawLine(QPointF(20, y), QPointF(20 - length, y));
        if (tick % 10 == 0) {
            const int value = qRound(std::fmod(tick * cm / 10, pageHeight) / cm);
            painter->drawText(QRectF(0, y - 12, 15, 13), Qt::AlignCenter, QString::number(value));
        }
    }
    painter->restore();
}
void PageEditor::resizeEvent(QResizeEvent* event) { QGraphicsView::resizeEvent(event); viewport()->update(); }
void PageEditor::wheelEvent(QWheelEvent* event) {
    if (event->modifiers().testFlag(Qt::ControlModifier)) {
        setZoomPercent(zoom_ + (event->angleDelta().y() > 0 ? 10 : -10));
        event->accept();
    } else QGraphicsView::wheelEvent(event);
}
void PageEditor::focusInEvent(QFocusEvent* event) {
    QGraphicsView::focusInEvent(event);
    textItem_->setFocus(event->reason());
}

