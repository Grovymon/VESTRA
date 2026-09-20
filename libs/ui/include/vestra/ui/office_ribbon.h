#pragma once

#include <QList>
#include <QString>
#include <QTabWidget>

class QLabel;
class QToolButton;
class QWidget;

namespace vestra::ui {

struct RibbonGroup {
    QString titleKey;
    QList<QWidget*> controls;
};

class OfficeRibbon final : public QTabWidget {
    Q_OBJECT

  public:
    explicit OfficeRibbon(QWidget* parent = nullptr);
    void addRibbonTab(const QString& titleKey, const QList<RibbonGroup>& groups);
    void retranslate();

  private:
    struct TabEntry {
        QString titleKey;
        QList<QPair<QLabel*, QString>> groupTitles;
    };
    QList<TabEntry> entries_;
};

QToolButton* makeRibbonButton(QWidget* parent = nullptr);

} // namespace vestra::ui

