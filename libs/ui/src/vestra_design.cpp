#include "vestra/ui/vestra_design.h"

#include "vestra/ui/translation_manager.h"

#include <QAction>
#include <QApplication>
#include <QFile>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPalette>
#include <QSlider>
#include <QSvgRenderer>
#include <QVBoxLayout>

#include <algorithm>

namespace vestra::ui {

QIcon IconProvider::icon(const QString& name, const QColor& color) {
    static const QHash<QString, QString> aliases{
        {QStringLiteral("brush"), QStringLiteral("highlight")},
        {QStringLiteral("font-grow"), QStringLiteral("zoom-in")},
        {QStringLiteral("font-shrink"), QStringLiteral("zoom-out")},
        {QStringLiteral("clear-format"), QStringLiteral("text-color")},
        {QStringLiteral("strike"), QStringLiteral("underline")},
        {QStringLiteral("subscript"), QStringLiteral("italic")},
        {QStringLiteral("superscript"), QStringLiteral("italic")},
        {QStringLiteral("paragraph"), QStringLiteral("numbering")},
        {QStringLiteral("line-spacing"), QStringLiteral("numbering")},
        {QStringLiteral("align-justify"), QStringLiteral("align-center")},
        {QStringLiteral("select"), QStringLiteral("copy")},
        {QStringLiteral("page-break"), QStringLiteral("new")},
        {QStringLiteral("link"), QStringLiteral("open")},
        {QStringLiteral("calendar"), QStringLiteral("table")},
        {QStringLiteral("symbol"), QStringLiteral("text-color")},
        {QStringLiteral("margins"), QStringLiteral("page-view")},
        {QStringLiteral("orientation"), QStringLiteral("page-view")},
        {QStringLiteral("statistics"), QStringLiteral("search")},
        {QStringLiteral("spell"), QStringLiteral("text-color")},
        {QStringLiteral("comment"), QStringLiteral("copy")},
        {QStringLiteral("review"), QStringLiteral("replace")},
        {QStringLiteral("page-width"), QStringLiteral("page-view")},
        {QStringLiteral("settings"), QStringLiteral("save-as")},
    };
    QFile file(QStringLiteral(":/vestra/icons/%1.svg").arg(aliases.value(name, name)));
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }

    QByteArray svg = file.readAll();
    const QColor effective = color.isValid() ? color : qApp->palette().color(QPalette::ButtonText);
    svg.replace("#334155", effective.name(QColor::HexRgb).toUtf8());

    QIcon result;
    for (const int size : {16, 20, 24, 28}) {
        QPixmap pixmap(size, size);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        QSvgRenderer renderer(svg);
        renderer.render(&painter, QRectF(0, 0, size, size));
        result.addPixmap(pixmap);
    }
    return result;
}

VestraRibbonButton::VestraRibbonButton(const QString& textKey, const QString& iconName,
                                       const RibbonButtonSize size, QWidget* parent)
    : QToolButton(parent), textKey_(textKey), iconName_(iconName), size_(size) {
    setObjectName(QStringLiteral("VestraRibbonButton"));
    setAutoRaise(true);
    setCursor(Qt::PointingHandCursor);
    setIcon(IconProvider::icon(iconName_));

    if (size_ == RibbonButtonSize::Large) {
        setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        setIconSize(QSize(28, 28));
        setMinimumSize(Metrics::largeButtonWidth, 66);
        setMaximumHeight(72);
    } else if (size_ == RibbonButtonSize::Medium) {
        setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        setIconSize(QSize(20, 20));
        setMinimumSize(Metrics::mediumButtonWidth, 56);
        setMaximumHeight(62);
    } else {
        setToolButtonStyle(Qt::ToolButtonIconOnly);
        setIconSize(QSize(18, 18));
        setFixedSize(Metrics::compactButtonSize, Metrics::compactButtonSize);
    }
    retranslate();
}

void VestraRibbonButton::retranslate() {
    const QByteArray key = textKey_.toLatin1();
    const QString translated = t(key.constData());
    setText(translated);
    setToolTip(translated);
    setAccessibleName(translated);
}

VestraRibbonGroup::VestraRibbonGroup(const QString& titleKey, QWidget* parent)
    : QFrame(parent), titleKey_(titleKey) {
    setObjectName(QStringLiteral("VestraRibbonGroup"));
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(7, 4, 7, 2);
    outer->setSpacing(1);
    content_ = new QHBoxLayout;
    content_->setContentsMargins(0, 0, 0, 0);
    content_->setSpacing(Metrics::spacing);
    title_ = new QLabel;
    title_->setObjectName(QStringLiteral("VestraRibbonGroupTitle"));
    title_->setAlignment(Qt::AlignCenter);
    outer->addLayout(content_, 1);
    outer->addWidget(title_);
    retranslate();
}

QHBoxLayout* VestraRibbonGroup::contentLayout() const { return content_; }

void VestraRibbonGroup::retranslate() {
    const QByteArray key = titleKey_.toLatin1();
    title_->setText(t(key.constData()));
}

VestraRibbon::VestraRibbon(QWidget* parent) : QTabWidget(parent) {
    setObjectName(QStringLiteral("VestraRibbon"));
    setDocumentMode(true);
    setMovable(false);
    setUsesScrollButtons(true);
    setMinimumHeight(Metrics::ribbonMinimumHeight);
    setMaximumHeight(Metrics::ribbonMaximumHeight);
}

void VestraRibbon::addRibbonTab(const QString& titleKey, QWidget* page) {
    page->setObjectName(QStringLiteral("VestraRibbonPage"));
    titleKeys_.append(titleKey);
    addTab(page, QString());
    retranslate();
}

void VestraRibbon::retranslate() {
    for (qsizetype index = 0; index < titleKeys_.size(); ++index) {
        const QByteArray key = titleKeys_[index].toLatin1();
        setTabText(static_cast<int>(index), t(key.constData()));
    }
    const auto groups = findChildren<VestraRibbonGroup*>();
    for (VestraRibbonGroup* group : groups) {
        group->retranslate();
    }
    const auto buttons = findChildren<VestraRibbonButton*>();
    for (VestraRibbonButton* button : buttons) {
        button->retranslate();
    }
}

VestraRuler::VestraRuler(const Qt::Orientation orientation, QWidget* parent)
    : QWidget(parent), orientation_(orientation) {
    setObjectName(QStringLiteral("VestraRuler"));
    if (orientation_ == Qt::Horizontal) {
        setFixedHeight(Metrics::horizontalRulerHeight);
        setMinimumWidth(300);
    } else {
        setFixedWidth(Metrics::verticalRulerWidth);
        setMinimumHeight(300);
    }
}

void VestraRuler::setZoomPercent(const int percent) {
    zoomPercent_ = std::clamp(percent, 50, 200);
    update();
}

void VestraRuler::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);
    const QColor background = palette().color(QPalette::Base);
    const QColor line = palette().color(QPalette::Mid);
    const QColor text = palette().color(QPalette::Text);
    painter.fillRect(rect(), background);
    painter.setPen(line);
    painter.drawRect(rect().adjusted(0, 0, -1, -1));

    const qreal unit = 48.0 * zoomPercent_ / 100.0;
    const int length = orientation_ == Qt::Horizontal ? width() : height();
    for (qreal position = 0; position < length; position += unit / 4.0) {
        const int tick = qRound(position / (unit / 4.0));
        const int mark = tick % 4 == 0 ? 10 : (tick % 2 == 0 ? 7 : 4);
        painter.setPen(line);
        if (orientation_ == Qt::Horizontal) {
            painter.drawLine(QPointF(position, height() - 1), QPointF(position, height() - mark));
            if (tick > 0 && tick % 4 == 0) {
                painter.setPen(text);
                painter.drawText(QRectF(position + 3, 1, 24, 12), QString::number(tick / 4));
            }
        } else {
            painter.drawLine(QPointF(width() - 1, position), QPointF(width() - mark, position));
            if (tick > 0 && tick % 4 == 0) {
                painter.save();
                painter.setPen(text);
                painter.translate(2, position + 24);
                painter.rotate(-90);
                painter.drawText(QRectF(0, 0, 24, 12), QString::number(tick / 4));
                painter.restore();
            }
        }
    }
}

VestraStatusBar::VestraStatusBar(QWidget* parent) : QWidget(parent) {
    setObjectName(QStringLiteral("VestraStatusBar"));
    setFixedHeight(Metrics::statusBarHeight);
    auto* row = new QHBoxLayout(this);
    row->setContentsMargins(10, 0, 8, 0);
    row->setSpacing(14);

    page_ = new QLabel;
    words_ = new QLabel;
    language_ = new QLabel;
    message_ = new QLabel;
    message_->setObjectName(QStringLiteral("StatusMessage"));
    row->addWidget(page_);
    row->addWidget(words_);
    row->addWidget(language_);
    row->addWidget(message_);
    row->addStretch(1);

    pageView_ = new QToolButton;
    pageView_->setObjectName(QStringLiteral("StatusViewButton"));
    pageView_->setIcon(IconProvider::icon(QStringLiteral("page-view")));
    pageView_->setIconSize(QSize(16, 16));
    pageView_->setAutoRaise(true);
    pageView_->setCheckable(true);
    pageView_->setChecked(true);
    connect(pageView_, &QToolButton::clicked, this, &VestraStatusBar::pageViewRequested);
    row->addWidget(pageView_);

    zoomOut_ = new QToolButton;
    zoomOut_->setObjectName(QStringLiteral("StatusZoomButton"));
    zoomOut_->setText(QStringLiteral("−"));
    zoomOut_->setAutoRaise(true);
    zoomIn_ = new QToolButton;
    zoomIn_->setObjectName(QStringLiteral("StatusZoomButton"));
    zoomIn_->setText(QStringLiteral("+"));
    zoomIn_->setAutoRaise(true);
    zoomSlider_ = new QSlider(Qt::Horizontal);
    zoomSlider_->setRange(50, 200);
    zoomSlider_->setSingleStep(10);
    zoomSlider_->setPageStep(25);
    zoomSlider_->setValue(100);
    zoomSlider_->setFixedWidth(118);
    zoomValue_ = new QLabel(QStringLiteral("100%"));
    zoomValue_->setMinimumWidth(38);
    row->addWidget(zoomOut_);
    row->addWidget(zoomSlider_);
    row->addWidget(zoomIn_);
    row->addWidget(zoomValue_);

    connect(zoomOut_, &QToolButton::clicked, this,
            [this] { emit zoomRequested(std::max(50, zoomSlider_->value() - 10)); });
    connect(zoomIn_, &QToolButton::clicked, this,
            [this] { emit zoomRequested(std::min(200, zoomSlider_->value() + 10)); });
    connect(zoomSlider_, &QSlider::valueChanged, this,
            [this](const int value) { emit zoomRequested(value); });
    retranslate();
}

void VestraStatusBar::setDocumentStats(const int currentPage, const int pageCount,
                                       const int wordCount) {
    currentPage_ = std::max(1, currentPage);
    pageCount_ = std::max(1, pageCount);
    wordCount_ = std::max(0, wordCount);
    retranslate();
}

void VestraStatusBar::setMessage(const QString& message) { message_->setText(message); }

void VestraStatusBar::clearMessage() { message_->clear(); }

void VestraStatusBar::setZoomPercent(const int percent) {
    const int normalized = std::clamp(percent, 50, 200);
    zoomSlider_->blockSignals(true);
    zoomSlider_->setValue(normalized);
    zoomSlider_->blockSignals(false);
    zoomValue_->setText(QStringLiteral("%1%").arg(normalized));
}

void VestraStatusBar::retranslate() {
    page_->setText(t("writer.status_page").arg(currentPage_).arg(pageCount_));
    words_->setText(t("writer.status_words").arg(wordCount_));
    language_->setText(TranslationManager::instance().language() == QStringLiteral("ru")
                           ? t("writer.language_ru")
                           : t("writer.language_en"));
    pageView_->setToolTip(t("writer.page_view"));
    pageView_->setAccessibleName(t("writer.page_view"));
    zoomOut_->setToolTip(t("writer.zoom_out"));
    zoomOut_->setAccessibleName(t("writer.zoom_out"));
    zoomIn_->setToolTip(t("writer.zoom_in"));
    zoomIn_->setAccessibleName(t("writer.zoom_in"));
}

QAction* ShortcutManager::bind(QWidget* owner, const QKeySequence& sequence,
                               const std::function<void()>& callback) {
    auto* action = new QAction(owner);
    action->setShortcut(sequence);
    action->setShortcutContext(Qt::WindowShortcut);
    owner->addAction(action);
    QObject::connect(action, &QAction::triggered, owner, callback);
    return action;
}

} // namespace vestra::ui

