#include "vestra/ui/office_ribbon.h"

#include "vestra/ui/translation_manager.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>
#include <QVBoxLayout>

namespace vestra::ui {

OfficeRibbon::OfficeRibbon(QWidget* parent) : QTabWidget(parent) {
    setObjectName(QStringLiteral("OfficeRibbon"));
    setDocumentMode(true);
    setMovable(false);
    setUsesScrollButtons(true);
    setMinimumHeight(126);
    setMaximumHeight(148);
}

void OfficeRibbon::addRibbonTab(const QString& titleKey, const QList<RibbonGroup>& groups) {
    auto* page = new QWidget;
    page->setObjectName(QStringLiteral("RibbonPage"));
    auto* row = new QHBoxLayout(page);
    row->setContentsMargins(8, 6, 8, 7);
    row->setSpacing(6);

    TabEntry entry{titleKey, {}};
    for (const RibbonGroup& specification : groups) {
        auto* group = new QFrame;
        group->setObjectName(QStringLiteral("RibbonGroup"));
        auto* column = new QVBoxLayout(group);
        column->setContentsMargins(7, 4, 7, 2);
        column->setSpacing(2);
        auto* controls = new QHBoxLayout;
        controls->setSpacing(3);
        for (QWidget* control : specification.controls) {
            controls->addWidget(control);
        }
        controls->addStretch();
        auto* title = new QLabel;
        title->setObjectName(QStringLiteral("RibbonGroupTitle"));
        title->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
        column->addLayout(controls, 1);
        column->addWidget(title);
        row->addWidget(group);
        entry.groupTitles.append({title, specification.titleKey});
    }
    row->addStretch(1);
    entries_.append(entry);
    addTab(page, QString());
    retranslate();
}

void OfficeRibbon::retranslate() {
    for (qsizetype index = 0; index < entries_.size(); ++index) {
        const QByteArray tabKey = entries_[index].titleKey.toLatin1();
        setTabText(static_cast<int>(index), t(tabKey.constData()));
        for (const auto& [label, key] : entries_[index].groupTitles) {
            const QByteArray groupKey = key.toLatin1();
            label->setText(t(groupKey.constData()));
        }
    }
}

QToolButton* makeRibbonButton(QWidget* parent) {
    auto* button = new QToolButton(parent);
    button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    button->setMinimumSize(54, 54);
    button->setAutoRaise(true);
    return button;
}

} // namespace vestra::ui

