#pragma once

#include <QColor>
#include <QFrame>
#include <QIcon>
#include <QKeySequence>
#include <QList>
#include <QString>
#include <QTabWidget>
#include <QToolButton>
#include <QWidget>

#include <functional>

class QAction;
class QBoxLayout;
class QHBoxLayout;
class QLabel;
class QSlider;

namespace vestra::ui {

struct Metrics final {
    static constexpr int titleBarHeight = 38;
    static constexpr int ribbonMinimumHeight = 126;
    static constexpr int ribbonMaximumHeight = 142;
    static constexpr int ribbonTabHeight = 34;
    static constexpr int controlHeight = 30;
    static constexpr int compactButtonSize = 30;
    static constexpr int mediumButtonWidth = 48;
    static constexpr int largeButtonWidth = 66;
    static constexpr int statusBarHeight = 28;
    static constexpr int horizontalRulerHeight = 24;
    static constexpr int verticalRulerWidth = 24;
    static constexpr int radius = 6;
    static constexpr int spacing = 4;
};

class IconProvider final {
  public:
    [[nodiscard]] static QIcon icon(const QString& name, const QColor& color = QColor());
};

enum class RibbonButtonSize { Compact, Medium, Large };

class VestraRibbonButton final : public QToolButton {
    Q_OBJECT

  public:
    VestraRibbonButton(const QString& textKey, const QString& iconName,
                       RibbonButtonSize size = RibbonButtonSize::Compact,
                       QWidget* parent = nullptr);
    void retranslate();

  private:
    QString textKey_;
    QString iconName_;
    RibbonButtonSize size_;
};

class VestraRibbonGroup final : public QFrame {
    Q_OBJECT

  public:
    explicit VestraRibbonGroup(const QString& titleKey, QWidget* parent = nullptr);
    [[nodiscard]] QHBoxLayout* contentLayout() const;
    void retranslate();

  private:
    QString titleKey_;
    QLabel* title_{nullptr};
    QHBoxLayout* content_{nullptr};
};

class VestraRibbon final : public QTabWidget {
    Q_OBJECT

  public:
    explicit VestraRibbon(QWidget* parent = nullptr);
    void addRibbonTab(const QString& titleKey, QWidget* page);
    void retranslate();

  private:
    QList<QString> titleKeys_;
};

class VestraRuler final : public QWidget {
    Q_OBJECT

  public:
    explicit VestraRuler(Qt::Orientation orientation, QWidget* parent = nullptr);
    void setZoomPercent(int percent);

  protected:
    void paintEvent(QPaintEvent* event) override;

  private:
    Qt::Orientation orientation_;
    int zoomPercent_{100};
};

class VestraStatusBar final : public QWidget {
    Q_OBJECT

  public:
    explicit VestraStatusBar(QWidget* parent = nullptr);
    void setDocumentStats(int currentPage, int pageCount, int wordCount);
    void setMessage(const QString& message);
    void clearMessage();
    void setZoomPercent(int percent);
    void retranslate();

  signals:
    void zoomRequested(int percent);
    void pageViewRequested();

  private:
    int currentPage_{1};
    int pageCount_{1};
    int wordCount_{0};
    QLabel* page_{nullptr};
    QLabel* words_{nullptr};
    QLabel* language_{nullptr};
    QLabel* message_{nullptr};
    QLabel* zoomValue_{nullptr};
    QSlider* zoomSlider_{nullptr};
    QToolButton* pageView_{nullptr};
    QToolButton* zoomOut_{nullptr};
    QToolButton* zoomIn_{nullptr};
};

class ShortcutManager final {
  public:
    static QAction* bind(QWidget* owner, const QKeySequence& sequence,
                         const std::function<void()>& callback);
};

} // namespace vestra::ui

