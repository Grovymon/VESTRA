#include "writer_window.h"
#include "page_editor.h"
#include "vestra/core/application_paths.h"
#include "vestra/core/file_type.h"
#include "vestra/sandbox/worker_session.h"
#include "vestra/ui/settings_dialog.h"
#include "vestra/ui/theme_manager.h"
#include "vestra/ui/translation_manager.h"

#include <QAbstractButton>
#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QCloseEvent>
#include <QColorDialog>
#include <QComboBox>
#include <QDateTime>
#include <QDialogButtonBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QFontComboBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QImageReader>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QPageLayout>
#include <QPdfWriter>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QPrinter>
#include <QPushButton>
#include <QRegularExpression>
#include <QSaveFile>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QStackedWidget>
#include <QTextBlock>
#include <QTextDocumentWriter>
#include <QTextImageFormat>
#include <QTextFrame>
#include <QTextList>
#include <QTextTable>
#include <QTimer>
#include <QUuid>
#include <QUrl>
#include <QVBoxLayout>

using vestra::ui::t;
using vestra::ui::VestraRibbonButton;
using vestra::ui::VestraRibbonGroup;
using Size = vestra::ui::RibbonButtonSize;

namespace {
QHBoxLayout* row(QWidget* widget = nullptr) {
    auto* layout = new QHBoxLayout(widget);
    layout->setContentsMargins(widget ? 5 : 0, widget ? 3 : 0, widget ? 5 : 0, widget ? 3 : 0);
    layout->setSpacing(3);
    return layout;
}
QVBoxLayout* column() {
    auto* layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);
    return layout;
}
QLabel* label(const char* key, const char* name = "") {
    auto* widget = new QLabel(t(key));
    widget->setProperty("i18n", key);
    widget->setObjectName(QString::fromLatin1(name));
    return widget;
}
QPushButton* textButton(const char* key) {
    auto* widget = new QPushButton(t(key));
    widget->setProperty("i18n", key);
    return widget;
}
QPageLayout printLayout(PageEditor* editor) {
    const QSizeF mm = editor->pageSize() * (25.4 / 96.0);
    return QPageLayout(QPageSize(mm, QPageSize::Millimeter), QPageLayout::Portrait,
                       QMarginsF(), QPageLayout::Millimeter);
}
}

WriterWindow::WriterWindow(QWidget* parent) : QMainWindow(parent) {
    setObjectName(QStringLiteral("WriterWindow"));
    resize(1440, 900);
    setMinimumSize(900, 600);
    recoveryId_ = QUuid::createUuid().toString(QUuid::Id128);
    worker_ = new vestra::sandbox::WorkerSession(this);
    editor_ = new PageEditor;
    editor_->setAccessibleName(t("writer.document_canvas"));
    auto* central = new QWidget;
    auto* root = new QVBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);
    auto* title = new QFrame;
    title->setObjectName(QStringLiteral("VestraTitleBar"));
    title->setFixedHeight(32);
    auto* titleRow = row(title);
    auto* brand = new QLabel(QStringLiteral("V"));
    brand->setObjectName(QStringLiteral("WriterBadge"));
    brand->setAlignment(Qt::AlignCenter);
    brand->setFixedSize(23, 23);
    titleRow->addWidget(brand);
    auto* quickSave = command("common.save", "save", Size::Compact, [this] { save(); }, false, QKeySequence::Save);
    auto* quickUndo = command("writer.undo", "undo", Size::Compact, [this] { editor_->undo(); }, true, QKeySequence::Undo);
    auto* quickRedo = command("writer.redo", "redo", Size::Compact, [this] { editor_->redo(); }, true, QKeySequence::Redo);
    for (auto* button : {quickSave, quickUndo, quickRedo}) {
        button->setObjectName(QStringLiteral("QuickAccessButton"));
        button->setProperty("whiteIcon", true);
        button->setFixedSize(28, 26);
        titleRow->addWidget(button);
    }
    connect(editor_->document(), &QTextDocument::undoAvailable, quickUndo, &QWidget::setEnabled);
    connect(editor_->document(), &QTextDocument::redoAvailable, quickRedo, &QWidget::setEnabled);
    quickUndo->setEnabled(false);
    quickRedo->setEnabled(false);
    titleRow->addStretch();
    documentTitle_ = new QLabel;
    titleRow->addWidget(documentTitle_);
    titleRow->addStretch();
    titleRow->addWidget(label("writer.beta", "BetaBadge"));
    titleRow->addSpacing(8);
    root->addWidget(title);

    body_ = new QStackedWidget;
    auto* editing = new QWidget;
    auto* editingColumn = new QVBoxLayout(editing);
    editingColumn->setContentsMargins(0, 0, 0, 0);
    editingColumn->setSpacing(0);
    ribbon_ = new vestra::ui::VestraRibbon;
    auto* filePage = new QWidget;
    auto* fileRow = row(filePage);
    fileRow->addWidget(command("writer.open", "open", Size::Large, [this] { openDocumentDialog(); }));
    fileRow->addStretch();
    ribbon_->addRibbonTab(QStringLiteral("ribbon.file"), filePage);
    ribbon_->addRibbonTab(QStringLiteral("ribbon.home"), buildHomeTab());
    ribbon_->addRibbonTab(QStringLiteral("ribbon.insert"), buildInsertTab());
    ribbon_->addRibbonTab(QStringLiteral("ribbon.layout"), buildLayoutTab());
    ribbon_->addRibbonTab(QStringLiteral("ribbon.review"), buildReviewTab());
    ribbon_->addRibbonTab(QStringLiteral("ribbon.view"), buildViewTab());
    ribbon_->setCurrentIndex(1);
    editingColumn->addWidget(ribbon_);
    protectedBanner_ = new QWidget;
    protectedBanner_->setObjectName(QStringLiteral("ProtectedBanner"));
    auto* protectedRow = row(protectedBanner_);
    protectedMessage_ = label("protected.message");
    enableEditing_ = textButton("protected.enable_editing");
    protectedRow->addWidget(label("protected.title"));
    protectedRow->addWidget(protectedMessage_);
    protectedRow->addStretch();
    protectedRow->addWidget(enableEditing_);
    editingColumn->addWidget(protectedBanner_);
    protectedBanner_->hide();
    auto* desk = new QWidget;
    auto* deskRow = row(desk);
    deskRow->setContentsMargins(0, 0, 0, 0);
    deskRow->setSpacing(0);
    navigation_ = buildNavigation();
    deskRow->addWidget(navigation_);
    deskRow->addWidget(editor_, 1);
    navigation_->hide();
    editingColumn->addWidget(desk, 1);
    status_ = new vestra::ui::VestraStatusBar;
    editingColumn->addWidget(status_);
    body_->addWidget(editing);
    body_->addWidget(buildBackstage());
    root->addWidget(body_, 1);
    setCentralWidget(central);
    connect(ribbon_, &QTabWidget::currentChanged, this, [this](int index) {
        if (index == 0) { refreshRecent(); body_->setCurrentIndex(1); }
    });
    connect(status_, &vestra::ui::VestraStatusBar::zoomRequested, editor_, &PageEditor::setZoomPercent);
    connect(status_, &vestra::ui::VestraStatusBar::pageViewRequested, editor_, &PageEditor::fitPage);
    connect(editor_, &PageEditor::zoomChanged, status_, &vestra::ui::VestraStatusBar::setZoomPercent);
    connect(editor_, &PageEditor::textChanged, this, &WriterWindow::updateDocumentStats);
    connect(editor_, &PageEditor::cursorPositionChanged, this, &WriterWindow::synchronizeFormat);
    connect(editor_, &PageEditor::pageGeometryChanged, this, &WriterWindow::updateDocumentStats);
    connect(editor_->document(), &QTextDocument::modificationChanged, this, &WriterWindow::updateWindowTitle);
    connect(enableEditing_, &QPushButton::clicked, this, [this] { setProtectedView(false); });
    connect(&vestra::ui::TranslationManager::instance(), &vestra::ui::TranslationManager::languageChanged,
            this, &WriterWindow::retranslate);
    connect(worker_, &vestra::sandbox::WorkerSession::accepted, this,
            [this](const QString& path, bool protectedView, bool macros, const QStringList&) {
        opening_ = false;
        status_->clearMessage();
        if (macros) QMessageBox::warning(this, t("macro.title"), t("macro.message"));
        openTrusted(path, protectedViewHint_ || protectedView || macros);
        setProtectedView(protectedView_);
    });
    connect(worker_, &vestra::sandbox::WorkerSession::rejected, this,
            [this](const QString&, const QString&, const QStringList&) {
        opening_ = false;
        setProtectedView(protectedView_);
        status_->clearMessage();
        QMessageBox::warning(this, t("common.error"), t("launcher.blocked"));
    });
    connect(worker_, &vestra::sandbox::WorkerSession::failed, this,
            [this](const QString&, const QString&) {
        opening_ = false;
        setProtectedView(protectedView_);
        status_->clearMessage();
        QMessageBox::warning(this, t("common.error"), t("launcher.worker_failed"));
    });
    using vestra::ui::ShortcutManager;
    ShortcutManager::bind(this, QKeySequence::New, [this] { newDocument(); });
    ShortcutManager::bind(this, QKeySequence::Open, [this] { openDocumentDialog(); });
    ShortcutManager::bind(this, QKeySequence::SaveAs, [this] { saveAs(); });
    ShortcutManager::bind(this, QKeySequence::Print, [this] { printDocument(); });
    ShortcutManager::bind(this, QKeySequence(Qt::CTRL | Qt::Key_Return), [this] { insertPageBreak(); });
    ShortcutManager::bind(this, QKeySequence(Qt::Key_Escape), [this] {
        body_->setCurrentIndex(0); navigation_->hide(); editor_->setFocus();
    });
    recoveryTimer_ = new QTimer(this);
    recoveryTimer_->setInterval(60000);
    connect(recoveryTimer_, &QTimer::timeout, this, &WriterWindow::writeRecovery);
    recoveryTimer_->start();
    retranslate();
    applyWriterTheme();
    synchronizeFormat();
    QTimer::singleShot(0, editor_, [this] { editor_->setFocus(); });
}

VestraRibbonButton* WriterWindow::command(const char* key, const char* icon, Size size,
        const std::function<void()>& handler, bool edits, const QKeySequence& shortcut) {
    auto* button = new VestraRibbonButton(QString::fromLatin1(key), QString::fromLatin1(icon), size);
    button->setProperty("command", key);
    button->setProperty("i18n", key);
    button->setProperty("edits", edits);
    button->setFocusPolicy(Qt::NoFocus);
    const auto guarded = [this, edits, handler] {
        if (edits && protectedView_) {
            status_->setMessage(t("protected.message"));
            return;
        }
        if (edits && opening_) return;
        handler();
    };
    connect(button, &QToolButton::clicked, this, guarded);
    if (!shortcut.isEmpty()) {
        vestra::ui::ShortcutManager::bind(this, shortcut, guarded);
        button->setProperty("shortcutLabel", shortcut.toString(QKeySequence::NativeText));
    }
    return button;
}

QWidget* WriterWindow::buildHomeTab() {
    auto* page = new QWidget;
    auto* layout = row(page);
    auto* clipboard = new VestraRibbonGroup(QStringLiteral("group.clipboard"));
    clipboard->contentLayout()->addWidget(command("writer.paste", "paste", Size::Large, [this] { editor_->paste(); }, true, QKeySequence::Paste));
    auto* clipColumn = column();
    for (auto* button : {
        command("writer.cut", "cut", Size::Compact, [this] { editor_->cut(); }, true, QKeySequence::Cut),
        command("writer.copy", "copy", Size::Compact, [this] { editor_->copy(); }, false, QKeySequence::Copy),
        command("writer.format_painter", "brush", Size::Compact, [this] {
            paintedFormat_ = editor_->textCursor().charFormat(); formatPainter_ = true;
            status_->setMessage(t("writer.paint_hint"));
        }, true)}) {
        button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        button->setFixedSize(136, 24);
        clipColumn->addWidget(button);
    }
    clipboard->contentLayout()->addLayout(clipColumn);
    layout->addWidget(clipboard);

    auto* fontGroup = new VestraRibbonGroup(QStringLiteral("group.font"));
    auto* fontColumn = column();
    auto* fontRow = row();
    fontFamily_ = new QFontComboBox;
    fontFamily_->setFixedWidth(144);
    fontSize_ = new QComboBox;
    fontSize_->setFixedWidth(54);
    fontSize_->setEditable(true);
    fontSize_->addItems({"8", "9", "10", "11", "12", "14", "16", "18", "20", "24", "28", "36", "48", "72"});
    fontRow->addWidget(fontFamily_);
    fontRow->addWidget(fontSize_);
    fontRow->addWidget(command("writer.grow_font", "font-grow", Size::Compact, [this] { changeFontSize(1); }, true));
    fontRow->addWidget(command("writer.shrink_font", "font-shrink", Size::Compact, [this] { changeFontSize(-1); }, true));
    fontRow->addWidget(command("writer.clear_format", "clear-format", Size::Compact, [this] {
        auto c = editor_->textCursor(); c.setCharFormat(QTextCharFormat()); editor_->setTextCursor(c);
    }, true));
    fontColumn->addLayout(fontRow);
    auto* formats = row();
    const QList<QPair<const char*, const char*>> names{{"writer.bold", "bold"}, {"writer.italic", "italic"},
        {"writer.underline", "underline"}, {"writer.strike", "strike"}, {"writer.subscript", "subscript"}, {"writer.superscript", "superscript"}};
    for (const auto& [key, icon] : names) {
        const QString attribute = QString::fromLatin1(icon);
        QKeySequence shortcut;
        if (attribute == "bold") shortcut = QKeySequence::Bold;
        if (attribute == "italic") shortcut = QKeySequence::Italic;
        if (attribute == "underline") shortcut = QKeySequence::Underline;
        auto* button = command(key, icon, Size::Compact, [this, attribute] { toggleCharacter(attribute); }, true, shortcut);
        button->setCheckable(true);
        formats->addWidget(button);
    }
    formats->addWidget(command("writer.highlight", "highlight", Size::Compact, [this] { chooseColor(true); }, true));
    formats->addWidget(command("writer.text_color", "text-color", Size::Compact, [this] { chooseColor(false); }, true));
    fontColumn->addLayout(formats);
    fontGroup->contentLayout()->addLayout(fontColumn);
    layout->addWidget(fontGroup);
    connect(fontFamily_, &QFontComboBox::currentFontChanged, this, [this](const QFont& font) {
        QTextCharFormat format; format.setFontFamilies({font.family()}); mergeFormat(format);
    });
    const auto applySize = [this] {
        bool valid = false; const qreal points = fontSize_->currentText().toDouble(&valid);
        if (valid && points >= 6 && points <= 144) { QTextCharFormat f; f.setFontPointSize(points); mergeFormat(f); }
        else synchronizeFormat();
    };
    connect(fontSize_, &QComboBox::textActivated, this, [applySize](const QString&) { applySize(); });
    connect(fontSize_->lineEdit(), &QLineEdit::editingFinished, this, applySize);

    auto* paragraph = new VestraRibbonGroup(QStringLiteral("group.paragraph"));
    auto* parColumn = column();
    auto* first = row();
    first->addWidget(command("writer.bullets", "bullets", Size::Compact, [this] { setListStyle(QTextListFormat::ListDisc); }, true));
    first->addWidget(command("writer.numbering", "numbering", Size::Compact, [this] { setListStyle(QTextListFormat::ListDecimal); }, true));
    first->addWidget(command("writer.outdent", "outdent", Size::Compact, [this] { changeIndent(-1); }, true));
    first->addWidget(command("writer.indent", "indent", Size::Compact, [this] { changeIndent(1); }, true));
    auto* marks = command("writer.show_marks", "paragraph", Size::Compact, [this] {
        const bool show = !editor_->document()->defaultTextOption().flags().testFlag(QTextOption::ShowTabsAndSpaces);
        editor_->setFormattingMarks(show);
    });
    marks->setCheckable(true);
    first->addWidget(marks);
    parColumn->addLayout(first);
    auto* second = row();
    const QList<Qt::Alignment> aligns{Qt::AlignLeft, Qt::AlignHCenter, Qt::AlignRight, Qt::AlignJustify};
    const char* keys[]{"writer.align_left", "writer.align_center", "writer.align_right", "writer.align_justify"};
    const char* icons[]{"align-left", "align-center", "align-right", "align-justify"};
    for (int i = 0; i < 4; ++i) {
        auto* b = command(keys[i], icons[i], Size::Compact, [this, a = aligns[i]] { setAlignment(a); }, true);
        b->setCheckable(true); second->addWidget(b);
    }
    second->addWidget(command("writer.paragraph", "line-spacing", Size::Compact, [this] { paragraphDialog(); }, true));
    parColumn->addLayout(second);
    paragraph->contentLayout()->addLayout(parColumn);
    layout->addWidget(paragraph);

    auto* styles = new VestraRibbonGroup(QStringLiteral("group.styles"));
    styles->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    const char* styleKeys[]{"writer.style_normal", "writer.style_title", "writer.style_heading1", "writer.style_heading2", "writer.style_quote"};
    for (int i = 0; i < 5; ++i) {
        auto* tile = textButton(styleKeys[i]);
        tile->setObjectName(QStringLiteral("StyleTile"));
        tile->setProperty("styleIndex", i);
        tile->setProperty("edits", true);
        tile->setCheckable(true);
        tile->setMinimumSize(84, 62);
        tile->setFocusPolicy(Qt::NoFocus);
        auto* tileLayout = new QVBoxLayout(tile);
        tileLayout->setContentsMargins(5, 4, 5, 5);
        auto* preview = new QLabel(QStringLiteral("AaBbCc"));
        preview->setObjectName(QStringLiteral("StylePreview"));
        preview->setAttribute(Qt::WA_TransparentForMouseEvents);
        QFont previewFont(QStringLiteral("Calibri"), i == 1 ? 20 : 16);
        previewFont.setBold(i == 2 || i == 3); previewFont.setItalic(i == 4);
        preview->setFont(previewFont);
        auto* caption = label(styleKeys[i]);
        caption->setAttribute(Qt::WA_TransparentForMouseEvents);
        caption->setAlignment(Qt::AlignCenter);
        preview->setAlignment(Qt::AlignCenter);
        tileLayout->addWidget(preview); tileLayout->addWidget(caption);
        tile->setText(QString());
        tile->setProperty("i18n", QVariant());
        tile->setAccessibleName(t(styleKeys[i]));
        connect(tile, &QPushButton::clicked, this, [this, i] { applyStyle(i); });
        styles->contentLayout()->addWidget(tile);
    }
    layout->addWidget(styles, 1);
    auto* editing = new VestraRibbonGroup(QStringLiteral("group.editing"));
    auto* editColumn = column();
    for (auto* b : {
        command("writer.find", "search", Size::Compact, [this] { showNavigation(); }, false, QKeySequence::Find),
        command("writer.replace", "replace", Size::Compact, [this] { showNavigation(true); }, false, QKeySequence(Qt::CTRL | Qt::Key_H)),
        command("writer.select_all", "select", Size::Compact, [this] { editor_->selectAll(); }, false, QKeySequence::SelectAll)}) {
        b->setToolButtonStyle(Qt::ToolButtonTextBesideIcon); b->setFixedSize(113, 24); editColumn->addWidget(b);
    }
    editing->contentLayout()->addLayout(editColumn);
    layout->addWidget(editing);
    return page;
}

QWidget* WriterWindow::buildInsertTab() {
    auto* page = new QWidget;
    auto* layout = row(page);
    auto addGroup = [layout](const char* name) {
        auto* group = new VestraRibbonGroup(QString::fromLatin1(name)); layout->addWidget(group); return group->contentLayout();
    };
    addGroup("group.pages")->addWidget(command("writer.page_break", "page-break", Size::Large, [this] { insertPageBreak(); }, true));
    auto* tables = addGroup("group.table");
    tables->addWidget(command("writer.table", "table", Size::Large, [this] { insertTable(); }, true));
    auto* tableColumn = column();
    tableColumn->addWidget(command("writer.table_row", "table", Size::Compact, [this] {
        if (auto* table = editor_->textCursor().currentTable()) table->appendRows(1);
        else status_->setMessage(t("writer.select_table"));
    }, true));
    tableColumn->addWidget(command("writer.table_column", "table", Size::Compact, [this] {
        if (auto* table = editor_->textCursor().currentTable()) table->appendColumns(1);
        else status_->setMessage(t("writer.select_table"));
    }, true));
    tables->addLayout(tableColumn);
    addGroup("group.illustrations")->addWidget(command("writer.image", "image", Size::Large, [this] { insertImage(); }, true));
    addGroup("group.links")->addWidget(command("writer.hyperlink", "link", Size::Large, [this] { insertLink(); }, true));
    auto* text = addGroup("group.text");
    text->addWidget(command("writer.date", "calendar", Size::Large, [this] {
        auto c = editor_->textCursor(); c.insertText(QLocale().toString(QDate::currentDate(), QLocale::LongFormat)); editor_->setTextCursor(c);
    }, true));
    text->addWidget(command("writer.symbol", "symbol", Size::Large, [this] {
        bool accepted = false;
        const QString symbol = QInputDialog::getItem(this, t("writer.symbol"), t("writer.symbol"),
            {QString::fromUtf8("©"), QString::fromUtf8("®"), QString::fromUtf8("™"), QString::fromUtf8("§"), QString::fromUtf8("±"), QString::fromUtf8("°"), QString::fromUtf8("€"), QString::fromUtf8("₽")}, 0, false, &accepted);
        if (accepted) { auto c = editor_->textCursor(); c.insertText(symbol); editor_->setTextCursor(c); }
    }, true));
    layout->addStretch();
    return page;
}

QWidget* WriterWindow::buildLayoutTab() {
    auto* page = new QWidget;
    auto* layout = row(page);
    auto* setup = new VestraRibbonGroup(QStringLiteral("group.page_setup"));
    setup->contentLayout()->addWidget(command("writer.page_setup", "margins", Size::Large, [this] { pageDialog(); }, true));
    auto* orientation = command("writer.orientation", "orientation", Size::Large, [] {}, true);
    auto* menu = new QMenu(orientation);
    for (bool landscape : {false, true}) {
        auto* action = menu->addAction(t(landscape ? "writer.landscape" : "writer.portrait"));
        action->setProperty("i18n", landscape ? "writer.landscape" : "writer.portrait");
        connect(action, &QAction::triggered, this, [this, landscape] {
            if (protectedView_ || opening_) return;
            auto size = editor_->pageSize();
            if ((size.width() > size.height()) != landscape) size.transpose();
            editor_->setPageGeometry(size, editor_->pageMargins());
            editor_->document()->setModified(true);
        });
    }
    orientation->setMenu(menu); orientation->setPopupMode(QToolButton::InstantPopup);
    setup->contentLayout()->addWidget(orientation);
    auto* paper = command("writer.paper_size", "page-view", Size::Large, [] {}, true);
    auto* paperMenu = new QMenu(paper);
    for (const auto& item : QList<QPair<QString, QSizeF>>{{QStringLiteral("A4"), QSizeF(793.7,1122.52)}, {QStringLiteral("A5"), QSizeF(559.37,793.7)}, {QStringLiteral("Letter"), QSizeF(816,1056)}}) {
        auto* action = paperMenu->addAction(item.first);
        connect(action, &QAction::triggered, this, [this, size = item.second] {
            if (protectedView_ || opening_) return;
            editor_->setPageGeometry(size, editor_->pageMargins()); editor_->document()->setModified(true);
        });
    }
    paper->setMenu(paperMenu); paper->setPopupMode(QToolButton::InstantPopup);
    setup->contentLayout()->addWidget(paper);
    setup->contentLayout()->addWidget(command("writer.page_break", "page-break", Size::Large, [this] { insertPageBreak(); }, true));
    layout->addWidget(setup);
    auto* paragraph = new VestraRibbonGroup(QStringLiteral("group.paragraph"));
    paragraph->contentLayout()->addWidget(command("writer.paragraph", "line-spacing", Size::Large, [this] { paragraphDialog(); }, true));
    paragraph->contentLayout()->addWidget(command("writer.outdent", "outdent", Size::Large, [this] { changeIndent(-1); }, true));
    paragraph->contentLayout()->addWidget(command("writer.indent", "indent", Size::Large, [this] { changeIndent(1); }, true));
    layout->addWidget(paragraph);
    layout->addStretch();
    return page;
}

QWidget* WriterWindow::buildReviewTab() {
    auto* page = new QWidget;
    auto* layout = row(page);
    auto* proof = new VestraRibbonGroup(QStringLiteral("group.proofing"));
    proof->contentLayout()->addWidget(command("writer.statistics", "statistics", Size::Large, [this] {
        const QString text = editor_->toPlainText();
        const int words = text.count(QRegularExpression(QStringLiteral("[\\p{L}\\p{N}]+(?:['’–-][\\p{L}\\p{N}]+)*")));
        QMessageBox::information(this, t("writer.statistics"), t("writer.statistics_text")
            .arg(editor_->pageCount()).arg(words).arg(text.size()).arg(editor_->document()->blockCount()));
    }));
    auto* spell = command("writer.spelling", "spell", Size::Large, [] {});
    spell->setEnabled(false); spell->setProperty("unavailable", true);
    proof->contentLayout()->addWidget(spell);
    layout->addWidget(proof);
    auto* search = new VestraRibbonGroup(QStringLiteral("group.search"));
    search->contentLayout()->addWidget(command("writer.find", "search", Size::Large, [this] { showNavigation(); }));
    search->contentLayout()->addWidget(command("writer.replace", "replace", Size::Large, [this] { showNavigation(true); }));
    layout->addWidget(search);
    auto* review = new VestraRibbonGroup(QStringLiteral("group.review"));
    for (const auto& [key, icon] : QList<QPair<const char*,const char*>>{{"writer.comments", "comment"}, {"writer.track_changes", "review"}}) {
        auto* button = command(key, icon, Size::Large, [] {});
        button->setEnabled(false); button->setProperty("unavailable", true);
        review->contentLayout()->addWidget(button);
    }
    layout->addWidget(review);
    layout->addStretch();
    return page;
}

QWidget* WriterWindow::buildViewTab() {
    auto* page = new QWidget;
    auto* layout = row(page);
    auto* zoom = new VestraRibbonGroup(QStringLiteral("group.zoom"));
    zoom->contentLayout()->addWidget(command("writer.fit_page", "page-view", Size::Large, [this] { editor_->fitPage(); }));
    zoom->contentLayout()->addWidget(command("writer.fit_width", "page-width", Size::Large, [this] { editor_->fitWidth(); }));
    auto* actual = command("writer.actual_size", "zoom-in", Size::Large, [this] { editor_->setZoomPercent(100); });
    zoom->contentLayout()->addWidget(actual);
    layout->addWidget(zoom);
    auto* display = new VestraRibbonGroup(QStringLiteral("group.display"));
    auto* controls = column();
    auto* rulers = new QCheckBox(t("writer.rulers")); rulers->setProperty("i18n", "writer.rulers"); rulers->setChecked(true);
    connect(rulers, &QCheckBox::toggled, editor_, &PageEditor::setRulersVisible);
    auto* navigation = new QCheckBox(t("writer.navigation")); navigation->setProperty("i18n", "writer.navigation");
    connect(navigation, &QCheckBox::toggled, this, [this](bool visible) { navigation_->setVisible(visible); });
    controls->addWidget(rulers); controls->addWidget(navigation);
    display->contentLayout()->addLayout(controls);
    layout->addWidget(display);
    auto* appearance = new VestraRibbonGroup(QStringLiteral("settings.appearance"));
    appearance->contentLayout()->addWidget(command("launcher.settings", "settings", Size::Large, [this] { showSettings(); }));
    layout->addWidget(appearance);
    layout->addStretch();
    return page;
}

QWidget* WriterWindow::buildBackstage() {
    auto* page = new QWidget;
    page->setObjectName(QStringLiteral("Backstage"));
    auto* layout = row(page); layout->setContentsMargins(0,0,0,0); layout->setSpacing(0);
    auto* sidebar = new QFrame; sidebar->setObjectName(QStringLiteral("BackstageSidebar")); sidebar->setFixedWidth(184);
    auto* side = new QVBoxLayout(sidebar); side->setContentsMargins(0,12,0,12); side->setSpacing(2);
    auto add = [this, side](const char* key, const std::function<void()>& fn) {
        auto* b = textButton(key); b->setObjectName(QStringLiteral("BackstageCommand"));
        b->setMinimumHeight(40); connect(b, &QPushButton::clicked, this, fn); side->addWidget(b);
    };
    add("writer.back", [this] { body_->setCurrentIndex(0); editor_->setFocus(); });
    add("launcher.new_document", [this] { newDocument(); });
    add("common.open", [this] { openDocumentDialog(); });
    add("common.save", [this] { if (save()) body_->setCurrentIndex(0); });
    add("writer.save_as", [this] { if (saveAs()) body_->setCurrentIndex(0); });
    add("writer.print", [this] { printPreview(); });
    add("writer.export_pdf", [this] { exportPdf(); });
    side->addStretch();
    add("launcher.settings", [this] { showSettings(); });
    add("common.close", [this] { close(); });
    layout->addWidget(sidebar);
    auto* content = new QWidget;
    auto* contents = new QVBoxLayout(content); contents->setContentsMargins(42,30,42,30); contents->setSpacing(22);
    contents->addWidget(label("writer.welcome", "BackstageHeading"));
    contents->addWidget(label("writer.local_note", "MutedText"));
    auto* cards = row();
    const char* names[]{"launcher.new_document", "writer.template_report", "writer.template_letter"};
    for (int i=0; i<3; ++i) {
        auto* card = textButton(names[i]); card->setObjectName(QStringLiteral("TemplateCard"));
        card->setIcon(vestra::ui::IconProvider::icon(QStringLiteral("new"), QColor("#3569ae")));
        card->setIconSize(QSize(44,44)); card->setMinimumSize(190,125);
        connect(card, &QPushButton::clicked, this, [this,i] { newDocument(i); }); cards->addWidget(card);
    }
    cards->addStretch(); contents->addLayout(cards);
    contents->addWidget(label("launcher.recent", "BackstageSubheading"));
    recent_ = new QListWidget; recent_->setObjectName(QStringLiteral("RecentDocuments"));
    connect(recent_, &QListWidget::itemActivated, this, [this](QListWidgetItem* item) {
        const QString path = item->data(Qt::UserRole).toString(); if (!path.isEmpty()) requestOpen(path);
    });
    contents->addWidget(recent_,1);
    auto* note = label("writer.beta_note", "MutedText"); note->setWordWrap(true); contents->addWidget(note);
    layout->addWidget(content,1);
    return page;
}

QWidget* WriterWindow::buildNavigation() {
    auto* panel = new QWidget;
    panel->setObjectName(QStringLiteral("NavigationPanel")); panel->setFixedWidth(260);
    auto* layout = new QVBoxLayout(panel); layout->setContentsMargins(15,18,15,18); layout->setSpacing(10);
    auto* header = row(); header->addWidget(label("writer.navigation", "BackstageSubheading")); header->addStretch();
    auto* hide = textButton("common.close");
    connect(hide, &QPushButton::clicked, this, [this] { navigation_->hide(); editor_->setFocus(); });
    header->addWidget(hide); layout->addLayout(header);
    query_ = new QLineEdit; query_->setClearButtonEnabled(true); query_->setObjectName(QStringLiteral("FindQuery"));
    query_->setAccessibleName(t("dialog.find_text")); layout->addWidget(query_);
    connect(query_, &QLineEdit::returnPressed, this, &WriterWindow::findText);
    matchCase_ = new QCheckBox(t("writer.match_case")); matchCase_->setProperty("i18n", "writer.match_case"); layout->addWidget(matchCase_);
    auto* next = textButton("writer.find_next"); connect(next, &QPushButton::clicked, this, &WriterWindow::findText); layout->addWidget(next);
    replacement_ = new QLineEdit; replacement_->setAccessibleName(t("dialog.replace_text")); layout->addWidget(replacement_);
    auto* replace = textButton("writer.replace_all"); replace->setProperty("edits",true);
    connect(replace, &QPushButton::clicked, this, &WriterWindow::replaceText); layout->addWidget(replace);
    searchResult_ = new QLabel; searchResult_->setWordWrap(true); layout->addWidget(searchResult_);
    layout->addStretch();
    return panel;
}

void WriterWindow::retranslate() {
    for (QWidget* widget : findChildren<QWidget*>()) {
        const QVariant key = widget->property("i18n");
        if (!key.isValid()) continue;
        const QString value = vestra::ui::TranslationManager::instance().text(key.toString());
        if (auto* text = qobject_cast<QLabel*>(widget)) text->setText(value);
        else if (auto* button = qobject_cast<QAbstractButton*>(widget)) button->setText(value);
        else if (auto* action = qobject_cast<QAction*>(widget)) action->setText(value);
    }
    ribbon_->retranslate();
    status_->retranslate();
    updateDocumentStats();
    updateWindowTitle();
}

void WriterWindow::synchronizeFormat() {
    if (formatPainter_ && editor_->textCursor().hasSelection()) {
        editor_->mergeFormat(paintedFormat_);
        formatPainter_ = false;
        status_->clearMessage();
    }
    const QTextCharFormat format = editor_->textCursor().charFormat();
    if (fontFamily_) {
        QSignalBlocker block(fontFamily_);
        fontFamily_->setCurrentFont(format.font().family().isEmpty() ? editor_->document()->defaultFont() : format.font());
    }
    if (fontSize_) {
        QSignalBlocker block(fontSize_);
        const int index = fontSize_->findText(QString::number(qRound(format.fontPointSize() > 0 ? format.fontPointSize() : 11)));
        if (index >= 0) fontSize_->setCurrentIndex(index);
    }
    updateDocumentStats();
}

void WriterWindow::updateDocumentStats() {
    const QString text = editor_->toPlainText().trimmed();
    const int words = text.isEmpty() ? 0 : text.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts).size();
    status_->setDocumentStats(editor_->currentPage(), editor_->pageCount(), words);
}

void WriterWindow::updateWindowTitle() {
    const QString fileName = currentPath_.isEmpty() ? t("writer.untitled") : QFileInfo(currentPath_).fileName();
    documentTitle_->setText(editor_->document()->isModified() ? fileName + QStringLiteral(" *") : fileName);
    setWindowTitle(QStringLiteral("%1 — %2").arg(fileName, t("writer.title")));
}

void WriterWindow::openDocumentDialog() {
    const QString path = QFileDialog::getOpenFileName(this, t("writer.open"), {}, t("writer.open_filter"));
    if (!path.isEmpty()) requestOpen(path);
}

void WriterWindow::newDocument(const int templateIndex) {
    if (!confirmDiscard()) return;
    currentPath_.clear();
    protectedViewHint_ = false;
    editor_->setPlainText({});
    if (templateIndex == 1) {
        editor_->setPlainText(t("writer.template_report_content"));
    } else if (templateIndex == 2) {
        editor_->setPlainText(t("writer.template_letter_content"));
    }
    editor_->document()->setModified(false);
    setProtectedView(false);
    body_->setCurrentIndex(0);
    ribbon_->setCurrentIndex(1);
    updateDocumentStats();
    updateWindowTitle();
    editor_->setFocus();
}

bool WriterWindow::confirmDiscard() {
    if (!editor_->document()->isModified()) return true;
    const auto result = QMessageBox::question(this, t("writer.unsaved_title"), t("writer.unsaved_message"),
                                              QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
                                              QMessageBox::Save);
    if (result == QMessageBox::Cancel) return false;
    return result != QMessageBox::Save || save();
}

void WriterWindow::requestOpen(const QString& path, const bool protectedViewHint) {
    if (opening_ || path.isEmpty()) return;
    const auto type = vestra::core::detectFileType(path);
    if (type.kind != vestra::core::FileKind::Writer && type.kind != vestra::core::FileKind::Text) {
        QMessageBox::warning(this, t("common.error"), t("launcher.unsupported"));
        return;
    }
    if (!confirmDiscard()) return;
    protectedViewHint_ = protectedViewHint;
    QString error;
    if (!worker_->inspect(path, &error)) {
        QMessageBox::warning(this, t("common.error"), error);
        return;
    }
    opening_ = true;
    status_->setMessage(t("writer.worker_check"));
}

void WriterWindow::openTrusted(const QString& path, const bool protectedView) {
    const QString suffix = QFileInfo(path).suffix().toLower();
    if (suffix != QStringLiteral("txt")) {
        QMessageBox::information(this, t("writer.title"), t("writer.backend_unavailable"));
        return;
    }
    QFile source(path);
    if (!source.open(QIODevice::ReadOnly) || source.size() > 16 * 1024 * 1024) {
        QMessageBox::warning(this, t("common.error"), t("writer.open_failed"));
        return;
    }
    currentPath_ = QFileInfo(path).absoluteFilePath();
    editor_->setPlainText(QString::fromUtf8(source.readAll()));
    editor_->document()->setModified(false);
    settings_.addRecentFile(currentPath_);
    settings_.sync();
    setProtectedView(protectedView);
    updateDocumentStats();
    updateWindowTitle();
    body_->setCurrentIndex(0);
    ribbon_->setCurrentIndex(1);
    editor_->setFocus();
}

bool WriterWindow::save() {
    return currentPath_.isEmpty() ? saveAs() : writeDocument(currentPath_);
}

bool WriterWindow::saveAs() {
    const QString suggested = currentPath_.isEmpty() ? t("writer.untitled") + QStringLiteral(".txt") : currentPath_;
    QString path = QFileDialog::getSaveFileName(this, t("writer.save_as"), suggested, t("writer.document_filter"));
    if (path.isEmpty()) return false;
    if (QFileInfo(path).suffix().isEmpty()) path += QStringLiteral(".txt");
    return writeDocument(path);
}

bool WriterWindow::writeDocument(const QString& path) {
    if (protectedView_) {
        status_->setMessage(t("protected.message"));
        return false;
    }
    if (QFileInfo(path).suffix().toLower() != QStringLiteral("txt")) {
        QMessageBox::information(this, t("writer.title"), t("writer.backend_unavailable"));
        return false;
    }
    QSaveFile target(path);
    if (!target.open(QIODevice::WriteOnly) || target.write(editor_->toPlainText().toUtf8()) < 0 || !target.commit()) {
        QMessageBox::warning(this, t("common.error"), t("writer.save_failed"));
        return false;
    }
    currentPath_ = QFileInfo(path).absoluteFilePath();
    settings_.addRecentFile(currentPath_);
    settings_.sync();
    editor_->document()->setModified(false);
    status_->setMessage(t("writer.saved"));
    updateWindowTitle();
    return true;
}

void WriterWindow::exportPdf() {
    const QString path = QFileDialog::getSaveFileName(this, t("dialog.export_pdf"), {}, t("pdf.save_filter"));
    if (path.isEmpty()) return;
    QPdfWriter writer(path.endsWith(QStringLiteral(".pdf"), Qt::CaseInsensitive) ? path : path + QStringLiteral(".pdf"));
    writer.setPageLayout(printLayout(editor_));
    writer.setCreator(QStringLiteral("Vestra Writer"));
    editor_->print(&writer);
    status_->setMessage(t("writer.exported"));
}

void WriterWindow::printDocument() {
    QPrinter printer(QPrinter::HighResolution);
    printer.setPageLayout(printLayout(editor_));
    QPrintDialog dialog(&printer, this);
    if (dialog.exec() == QDialog::Accepted) editor_->print(&printer);
}

void WriterWindow::printPreview() {
    QPrinter printer(QPrinter::HighResolution);
    printer.setPageLayout(printLayout(editor_));
    QPrintPreviewDialog preview(&printer, this);
    connect(&preview, &QPrintPreviewDialog::paintRequested, this, [this](QPrinter* output) { editor_->print(output); });
    preview.exec();
}

void WriterWindow::mergeFormat(const QTextCharFormat& format) { editor_->mergeFormat(format); }

void WriterWindow::toggleCharacter(const QString& attribute) {
    QTextCharFormat format;
    const QTextCharFormat active = editor_->textCursor().charFormat();
    if (attribute == QStringLiteral("bold")) format.setFontWeight(active.fontWeight() == QFont::Bold ? QFont::Normal : QFont::Bold);
    else if (attribute == QStringLiteral("italic")) format.setFontItalic(!active.fontItalic());
    else if (attribute == QStringLiteral("underline")) format.setFontUnderline(!active.fontUnderline());
    else if (attribute == QStringLiteral("strike")) format.setFontStrikeOut(!active.fontStrikeOut());
    else if (attribute == QStringLiteral("subscript")) format.setVerticalAlignment(active.verticalAlignment() == QTextCharFormat::AlignSubScript ? QTextCharFormat::AlignNormal : QTextCharFormat::AlignSubScript);
    else if (attribute == QStringLiteral("superscript")) format.setVerticalAlignment(active.verticalAlignment() == QTextCharFormat::AlignSuperScript ? QTextCharFormat::AlignNormal : QTextCharFormat::AlignSuperScript);
    mergeFormat(format);
}

void WriterWindow::changeFontSize(const int delta) {
    const qreal current = editor_->textCursor().charFormat().fontPointSize();
    QTextCharFormat format;
    format.setFontPointSize(qBound<qreal>(6, (current > 0 ? current : 11) + delta, 96));
    mergeFormat(format);
}

void WriterWindow::chooseColor(const bool highlight) {
    const QColor color = QColorDialog::getColor(highlight ? editor_->textCursor().charFormat().background().color()
                                                           : editor_->textCursor().charFormat().foreground().color(), this,
                                                   highlight ? t("writer.highlight") : t("writer.text_color"));
    if (!color.isValid()) return;
    QTextCharFormat format;
    if (highlight) format.setBackground(color); else format.setForeground(color);
    mergeFormat(format);
}

void WriterWindow::setListStyle(const int style) {
    QTextListFormat format;
    format.setStyle(static_cast<QTextListFormat::Style>(style));
    editor_->textCursor().createList(format);
}

void WriterWindow::setAlignment(const Qt::Alignment alignment) {
    auto cursor = editor_->textCursor();
    QTextBlockFormat format = cursor.blockFormat();
    format.setAlignment(alignment);
    cursor.mergeBlockFormat(format);
    editor_->setTextCursor(cursor);
}

void WriterWindow::changeIndent(const int delta) {
    auto cursor = editor_->textCursor();
    QTextBlockFormat format = cursor.blockFormat();
    format.setLeftMargin(qBound<qreal>(0, format.leftMargin() + delta * 24, 288));
    cursor.mergeBlockFormat(format);
    editor_->setTextCursor(cursor);
}

void WriterWindow::applyStyle(const int index) {
    QTextCharFormat character;
    QTextBlockFormat block;
    switch (index) {
    case 1: character.setFontPointSize(26); character.setFontWeight(QFont::Bold); block.setAlignment(Qt::AlignCenter); break;
    case 2: character.setFontPointSize(18); character.setFontWeight(QFont::Bold); break;
    case 3: character.setFontPointSize(14); character.setFontWeight(QFont::Bold); break;
    case 4: character.setFontItalic(true); block.setLeftMargin(36); block.setRightMargin(36); break;
    default: character.setFontPointSize(11); character.setFontWeight(QFont::Normal); character.setFontItalic(false); break;
    }
    auto cursor = editor_->textCursor();
    cursor.mergeCharFormat(character);
    cursor.mergeBlockFormat(block);
    editor_->setTextCursor(cursor);
}

void WriterWindow::insertTable() {
    bool accepted = false;
    const int rows = QInputDialog::getInt(this, t("writer.table"), t("writer.table_rows"), 3, 1, 20, 1, &accepted);
    if (!accepted) return;
    const int columns = QInputDialog::getInt(this, t("writer.table"), t("writer.table_columns"), 3, 1, 12, 1, &accepted);
    if (!accepted) return;
    QTextTableFormat format; format.setBorder(1); format.setCellPadding(5); format.setCellSpacing(0); format.setWidth(QTextLength(QTextLength::PercentageLength, 100));
    editor_->textCursor().insertTable(rows, columns, format);
}

void WriterWindow::insertImage() {
    const QString path = QFileDialog::getOpenFileName(this, t("dialog.select_image"), {}, t("dialog.image_filter"));
    if (path.isEmpty()) return;
    const QFileInfo info(path);
    if (info.size() > 32 * 1024 * 1024) { QMessageBox::warning(this, t("common.error"), t("writer.image_too_large")); return; }
    QImageReader reader(path);
    const QSize size = reader.size();
    if (!size.isValid() || size.width() > 12000 || size.height() > 12000) { QMessageBox::warning(this, t("common.error"), t("writer.image_too_large")); return; }
    const QImage image = reader.read();
    if (image.isNull()) { QMessageBox::warning(this, t("common.error"), t("writer.image_failed")); return; }
    const QString name = QStringLiteral("vestra-image://%1").arg(QUuid::createUuid().toString(QUuid::Id128));
    editor_->document()->addResource(QTextDocument::ImageResource, QUrl(name), image);
    QTextImageFormat format; format.setName(name);
    const QSize scaled = image.size().scaled(QSize(520, 700), Qt::KeepAspectRatio);
    format.setWidth(scaled.width()); format.setHeight(scaled.height());
    editor_->textCursor().insertImage(format);
}

void WriterWindow::insertLink() {
    bool accepted = false;
    const QString address = QInputDialog::getText(this, t("writer.hyperlink"), t("writer.link_address"), QLineEdit::Normal, QStringLiteral("https://"), &accepted).trimmed();
    if (!accepted || address.isEmpty()) return;
    auto cursor = editor_->textCursor();
    const QString caption = cursor.hasSelection() ? cursor.selectedText() : address;
    if (cursor.hasSelection()) cursor.removeSelectedText();
    QTextCharFormat format; format.setAnchor(true); format.setAnchorHref(address); format.setForeground(QColor(QStringLiteral("#1f5fa8"))); format.setFontUnderline(true);
    cursor.insertText(caption, format);
    editor_->setTextCursor(cursor);
}

void WriterWindow::insertPageBreak() {
    auto cursor = editor_->textCursor();
    QTextBlockFormat format; format.setPageBreakPolicy(QTextFormat::PageBreak_AlwaysBefore);
    cursor.insertBlock(format);
    editor_->setTextCursor(cursor);
}

void WriterWindow::paragraphDialog() {
    QDialog dialog(this); dialog.setWindowTitle(t("writer.paragraph"));
    auto* layout = new QVBoxLayout(&dialog); auto* form = new QFormLayout;
    auto* spacing = new QDoubleSpinBox; spacing->setRange(0.8, 3.0); spacing->setSingleStep(0.1); spacing->setValue(1.0);
    auto* before = new QDoubleSpinBox; before->setRange(0, 72); before->setSuffix(QStringLiteral(" pt"));
    auto* after = new QDoubleSpinBox; after->setRange(0, 72); after->setSuffix(QStringLiteral(" pt"));
    form->addRow(t("writer.line_spacing"), spacing); form->addRow(t("writer.space_before"), before); form->addRow(t("writer.space_after"), after); layout->addLayout(form);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel); layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept); connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() != QDialog::Accepted) return;
    auto cursor = editor_->textCursor(); QTextBlockFormat format = cursor.blockFormat();
    format.setLineHeight(qRound(spacing->value() * 100), QTextBlockFormat::ProportionalHeight); format.setTopMargin(before->value()); format.setBottomMargin(after->value());
    cursor.mergeBlockFormat(format); editor_->setTextCursor(cursor);
}

void WriterWindow::pageDialog() {
    QDialog dialog(this); dialog.setWindowTitle(t("writer.page_setup"));
    auto* layout = new QVBoxLayout(&dialog); auto* form = new QFormLayout;
    const QSizeF currentMm = editor_->pageSize() * (25.4 / 96.0); const QMarginsF currentMargins = editor_->pageMargins() * (25.4 / 96.0);
    auto value = [&dialog](qreal minimum, qreal maximum, qreal initial) { auto* spin = new QDoubleSpinBox(&dialog); spin->setRange(minimum, maximum); spin->setDecimals(1); spin->setSuffix(QStringLiteral(" mm")); spin->setValue(initial); return spin; };
    auto* width = value(100, 500, currentMm.width()); auto* height = value(100, 500, currentMm.height());
    auto* left = value(5, 80, currentMargins.left()); auto* right = value(5, 80, currentMargins.right()); auto* top = value(5, 80, currentMargins.top()); auto* bottom = value(5, 80, currentMargins.bottom());
    form->addRow(t("writer.page_width"), width); form->addRow(t("writer.page_height"), height); form->addRow(t("writer.margin_left"), left); form->addRow(t("writer.margin_right"), right); form->addRow(t("writer.margin_top"), top); form->addRow(t("writer.margin_bottom"), bottom); layout->addLayout(form);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel); layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept); connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() == QDialog::Accepted) editor_->setPageGeometry(QSizeF(width->value(), height->value()) * (96.0 / 25.4), QMarginsF(left->value(), top->value(), right->value(), bottom->value()) * (96.0 / 25.4));
}

void WriterWindow::findText() {
    const QString needle = query_ ? query_->text() : QString();
    if (needle.isEmpty()) return;
    QTextDocument::FindFlags flags;
    if (matchCase_ && matchCase_->isChecked()) flags |= QTextDocument::FindCaseSensitively;
    if (editor_->find(needle, flags)) searchResult_->setText(t("writer.find_result")); else searchResult_->setText(t("writer.find_missing"));
}

void WriterWindow::replaceText() {
    const QString needle = query_ ? query_->text() : QString();
    if (needle.isEmpty()) return;
    QTextDocument::FindFlags flags;
    if (matchCase_ && matchCase_->isChecked()) flags |= QTextDocument::FindCaseSensitively;
    int count = 0; QTextCursor start(editor_->document());
    while (true) {
        QTextCursor found = editor_->document()->find(needle, start, flags);
        if (found.isNull()) break;
        found.insertText(replacement_->text());
        start.setPosition(found.position());
        if (++count > 10000) break;
    }
    searchResult_->setText(t("writer.replace_result").arg(count));
}

void WriterWindow::showNavigation(const bool replacement) {
    navigation_->show();
    if (replacement) replacement_->setFocus(); else query_->setFocus();
}

void WriterWindow::setProtectedView(const bool enabled) {
    protectedView_ = enabled;
    editor_->setReadOnly(enabled);
    protectedBanner_->setVisible(enabled);
    if (!enabled) editor_->setFocus();
}

void WriterWindow::writeRecovery() {
    if (!settings_.autoRecoveryEnabled() || protectedView_ || !editor_->document()->isModified()) return;
    QSaveFile recovery(QDir(vestra::core::ApplicationPaths::recovery()).filePath(recoveryId_ + QStringLiteral(".txt")));
    if (recovery.open(QIODevice::WriteOnly) && recovery.write(editor_->toPlainText().toUtf8()) >= 0) recovery.commit();
}

void WriterWindow::refreshRecent() {
    if (!recent_) return;
    recent_->clear();
    const QStringList files = settings_.recentFiles();
    if (files.isEmpty()) { auto* item = new QListWidgetItem(t("launcher.no_recent")); item->setFlags(Qt::NoItemFlags); recent_->addItem(item); return; }
    for (const QString& path : files) {
        auto* item = new QListWidgetItem(QFileInfo(path).fileName() + QStringLiteral("\n") + path, recent_);
        item->setData(Qt::UserRole, path);
    }
}

void WriterWindow::showSettings() {
    vestra::ui::SettingsDialog dialog(settings_, this);
    connect(&dialog, &vestra::ui::SettingsDialog::settingsApplied, this, [this] { applyWriterTheme(); retranslate(); refreshRecent(); });
    connect(&dialog, &vestra::ui::SettingsDialog::recentFilesCleared, this, &WriterWindow::refreshRecent);
    dialog.exec();
}

void WriterWindow::applyWriterTheme() { vestra::ui::ThemeManager::apply(settings_.theme()); }

void WriterWindow::closeEvent(QCloseEvent* event) {
    if (!confirmDiscard()) { event->ignore(); return; }
    if (settings_.autoRecoveryEnabled()) writeRecovery();
    event->accept();
}

