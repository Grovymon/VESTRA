#pragma once

#include <QGraphicsView>
#include <QMarginsF>
#include <QTextCursor>
#include <QTextDocument>

class WriterTextItem;
class QPrinter;
class QPagedPaintDevice;

// Qt owns rich-text layout, editing, tables and pagination. Zoom is a view transform:
// it never changes saved font sizes, line wrapping, or the undo stack.
class PageEditor final : public QGraphicsView {
    Q_OBJECT
  public:
    explicit PageEditor(QWidget* parent = nullptr);
    QTextDocument* document() const;
    QTextCursor textCursor() const;
    void setTextCursor(const QTextCursor& cursor);
    QString toPlainText() const;
    void setPlainText(const QString& text);
    void setHtml(const QString& html);
    void mergeFormat(const QTextCharFormat& format);
    void setReadOnly(bool enabled);
    bool isReadOnly() const;
    void setPageGeometry(const QSizeF& size, const QMarginsF& margins);
    QSizeF pageSize() const;
    QMarginsF pageMargins() const;
    int pageCount() const;
    int currentPage() const;
    QRectF cursorRectangle() const;
    int zoomPercent() const;
    void setZoomPercent(int percent);
    void fitWidth();
    void fitPage();
    void setRulersVisible(bool visible);
    void setFormattingMarks(bool visible);
    void copy();
    void cut();
    void paste();
    void undo();
    void redo();
    void selectAll();
    void refreshCursor();
    bool find(const QString& query, QTextDocument::FindFlags flags = {});
    void print(QPagedPaintDevice* device) const;

  signals:
    void textChanged();
    void cursorPositionChanged();
    void zoomChanged(int percent);
    void pageGeometryChanged();

  protected:
    void drawBackground(QPainter* painter, const QRectF& rect) override;
    void drawForeground(QPainter* painter, const QRectF& rect) override;
    void resizeEvent(QResizeEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void focusInEvent(QFocusEvent* event) override;

  private:
    void relayout();
    WriterTextItem* textItem_{nullptr};
    QSizeF paper_{793.7, 1122.52};
    QMarginsF margins_{94.49, 75.59, 75.59, 75.59};
    int zoom_{100};
    bool readOnly_{false};
    bool rulers_{true};
};

