#include "sheets_window.h"

#include "vestra/sandbox/worker_session.h"
#include "vestra/ui/office_ribbon.h"
#include "vestra/ui/translation_manager.h"

#include <QApplication>
#include <QClipboard>
#include <QColorDialog>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSaveFile>
#include <QSignalBlocker>
#include <QStatusBar>
#include <QTableWidget>
#include <QTextStream>
#include <QToolButton>
#include <QUndoCommand>
#include <QUndoStack>
#include <QVBoxLayout>

#include <algorithm>

using vestra::ui::makeRibbonButton;
using vestra::ui::t;

namespace {

QString columnName(int column) {
    QString result;
    for (++column; column > 0; column = (column - 1) / 26) {
        result.prepend(QChar(u'A' + (column - 1) % 26));
    }
    return result;
}

QStringList parseCsvRow(const QString& line) {
    QStringList cells;
    QString cell;
    bool quoted = false;
    for (qsizetype index = 0; index < line.size(); ++index) {
        const QChar character = line[index];
        if (character == u'"') {
            if (quoted && index + 1 < line.size() && line[index + 1] == u'"') {
                cell.append(u'"');
                ++index;
            } else {
                quoted = !quoted;
            }
        } else if (character == u',' && !quoted) {
            cells.append(cell);
            cell.clear();
        } else {
            cell.append(character);
        }
    }
    cells.append(cell);
    return cells;
}

QString csvCell(QString value) {
    if (value.contains(u',') || value.contains(u'"') || value.contains(u'\n')) {
        value.replace(QStringLiteral("\""), QStringLiteral("\"\""));
        return u'"' + value + u'"';
    }
    return value;
}

class CellEditCommand final : public QUndoCommand {
  public:
    CellEditCommand(QTableWidget* table, int row, int column, QString before, QString after)
        : table_(table), row_(row), column_(column), before_(std::move(before)),
          after_(std::move(after)) {}

    void undo() override { apply(before_); }
    void redo() override { apply(after_); }

  private:
    void apply(const QString& value) {
        QSignalBlocker blocker(table_);
        QTableWidgetItem* item = table_->item(row_, column_);
        if (item == nullptr) {
            item = new QTableWidgetItem;
            table_->setItem(row_, column_, item);
        }
        item->setText(value);
        item->setData(Qt::UserRole, value);
    }

    QTableWidget* table_;
    int row_;
    int column_;
    QString before_;
    QString after_;
};

} // namespace

SheetsWindow::SheetsWindow(QWidget* parent) : QMainWindow(parent) {
    worker_ = new vestra::sandbox::WorkerSession(this);
    undoStack_ = new QUndoStack(this);
    buildUi();
    addSheet();
    retranslate();

    connect(&vestra::ui::TranslationManager::instance(),
            &vestra::ui::TranslationManager::languageChanged, this, &SheetsWindow::retranslate);
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

void SheetsWindow::buildUi() {
    resize(1240, 820);
    setMinimumSize(820, 560);
    auto* central = new QWidget;
    auto* root = new QVBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto* titleBar = new QFrame;
    titleBar->setObjectName(QStringLiteral("TitleBar"));
    titleBar->setFixedHeight(40);
    auto* titleRow = new QHBoxLayout(titleBar);
    titleRow->setContentsMargins(14, 5, 14, 5);
    auto* badge = new QLabel(QStringLiteral("S"));
    badge->setAlignment(Qt::AlignCenter);
    badge->setFixedSize(28, 28);
    badge->setStyleSheet(QStringLiteral(
        "background:#16834b;color:white;font-weight:700;border-radius:5px;font-size:16px;"));
    auto* product = new QLabel(QStringLiteral("Vestra Sheets"));
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

    auto* formulaRow = new QHBoxLayout;
    formulaRow->setContentsMargins(6, 5, 6, 5);
    formulaRow->setSpacing(5);
    cellName_ = new QLineEdit;
    cellName_->setReadOnly(true);
    cellName_->setFixedWidth(75);
    auto* fx = new QLabel(QStringLiteral("fx"));
    fx->setStyleSheet(QStringLiteral("font-style:italic;font-weight:600;padding:0 5px;"));
    formulaBar_ = new QLineEdit;
    formulaRow->addWidget(cellName_);
    formulaRow->addWidget(fx);
    formulaRow->addWidget(formulaBar_, 1);
    root->addLayout(formulaRow);

    sheets_ = new QTabWidget;
    sheets_->setTabsClosable(false);
    sheets_->setMovable(true);
    root->addWidget(sheets_, 1);
    setCentralWidget(central);

    status_ = new QLabel;
    statusBar()->addWidget(status_, 1);

    connect(sheets_, &QTabWidget::currentChanged, this, &SheetsWindow::updateFormulaBar);
    connect(formulaBar_, &QLineEdit::returnPressed, this, [this] {
        QTableWidget* table = currentSheet();
        if (table == nullptr || table->currentItem() == nullptr || protectedBanner_->isVisible()) {
            return;
        }
        table->currentItem()->setText(formulaBar_->text());
    });
    connect(enableEditing_, &QPushButton::clicked, this, [this] { setProtectedView(false); });
}

void SheetsWindow::buildRibbon() {
    ribbon_ = new vestra::ui::OfficeRibbon;
    openButton_ = makeRibbonButton();
    saveButton_ = makeRibbonButton();
    undoButton_ = makeRibbonButton();
    redoButton_ = makeRibbonButton();
    copyButton_ = makeRibbonButton();
    pasteButton_ = makeRibbonButton();
    boldButton_ = makeRibbonButton();
    fillButton_ = makeRibbonButton();
    borderButton_ = makeRibbonButton();
    sortAscButton_ = makeRibbonButton();
    sortDescButton_ = makeRibbonButton();
    newSheetButton_ = makeRibbonButton();
    deleteSheetButton_ = makeRibbonButton();

    auto* alignLeft = makeRibbonButton();
    auto* alignCenter = makeRibbonButton();
    auto* alignRight = makeRibbonButton();
    auto* insertRow = makeRibbonButton();
    auto* insertColumn = makeRibbonButton();

    ribbon_->addRibbonTab(QStringLiteral("ribbon.file"),
                          {{QStringLiteral("group.file"), {openButton_, saveButton_}}});
    ribbon_->addRibbonTab(
        QStringLiteral("ribbon.home"),
        {{QStringLiteral("group.clipboard"), {copyButton_, pasteButton_}},
         {QStringLiteral("group.editing"), {undoButton_, redoButton_}},
         {QStringLiteral("group.font"), {boldButton_, fillButton_, borderButton_}},
         {QStringLiteral("group.alignment"), {alignLeft, alignCenter, alignRight}}});
    ribbon_->addRibbonTab(
        QStringLiteral("ribbon.insert"),
        {{QStringLiteral("group.cells"), {insertRow, insertColumn}},
         {QStringLiteral("group.sheets"), {newSheetButton_, deleteSheetButton_}}});
    ribbon_->addRibbonTab(QStringLiteral("ribbon.layout"), {{QStringLiteral("group.sort_filter"),
                                                             {sortAscButton_, sortDescButton_}}});
    ribbon_->addRibbonTab(QStringLiteral("ribbon.review"), {});
    ribbon_->addRibbonTab(QStringLiteral("ribbon.view"), {});
    ribbon_->setCurrentIndex(1);

    connect(openButton_, &QToolButton::clicked, this, &SheetsWindow::openFile);
    connect(saveButton_, &QToolButton::clicked, this, [this] { saveFile(); });
    connect(undoButton_, &QToolButton::clicked, undoStack_, &QUndoStack::undo);
    connect(redoButton_, &QToolButton::clicked, undoStack_, &QUndoStack::redo);
    connect(copyButton_, &QToolButton::clicked, this, &SheetsWindow::copySelection);
    connect(pasteButton_, &QToolButton::clicked, this, &SheetsWindow::pasteSelection);
    connect(boldButton_, &QToolButton::clicked, this, &SheetsWindow::applyBold);
    connect(fillButton_, &QToolButton::clicked, this, &SheetsWindow::applyFill);
    connect(borderButton_, &QToolButton::clicked, this, &SheetsWindow::applyBorders);
    connect(alignLeft, &QToolButton::clicked, this,
            [this] { applyAlignment(Qt::AlignLeft | Qt::AlignVCenter); });
    connect(alignCenter, &QToolButton::clicked, this, [this] { applyAlignment(Qt::AlignCenter); });
    connect(alignRight, &QToolButton::clicked, this,
            [this] { applyAlignment(Qt::AlignRight | Qt::AlignVCenter); });
    connect(sortAscButton_, &QToolButton::clicked, this,
            [this] { sortSelection(Qt::AscendingOrder); });
    connect(sortDescButton_, &QToolButton::clicked, this,
            [this] { sortSelection(Qt::DescendingOrder); });
    connect(newSheetButton_, &QToolButton::clicked, this, [this] { addSheet(); });
    connect(deleteSheetButton_, &QToolButton::clicked, this, [this] {
        if (sheets_->count() > 1) {
            QWidget* page = sheets_->currentWidget();
            sheets_->removeTab(sheets_->currentIndex());
            page->deleteLater();
        }
    });
    connect(insertRow, &QToolButton::clicked, this, [this] {
        if (QTableWidget* table = currentSheet(); table != nullptr) {
            table->insertRow(std::max(0, table->currentRow()));
        }
    });
    connect(insertColumn, &QToolButton::clicked, this, [this] {
        if (QTableWidget* table = currentSheet(); table != nullptr) {
            table->insertColumn(std::max(0, table->currentColumn()));
        }
    });

    alignLeft->setProperty("translationKey", QStringLiteral("writer.align_left"));
    alignCenter->setProperty("translationKey", QStringLiteral("writer.align_center"));
    alignRight->setProperty("translationKey", QStringLiteral("writer.align_right"));
    insertRow->setProperty("translationKey", QStringLiteral("sheets.insert_row"));
    insertColumn->setProperty("translationKey", QStringLiteral("sheets.insert_column"));
}

void SheetsWindow::retranslate() {
    ribbon_->retranslate();
    openButton_->setText(t("common.open"));
    saveButton_->setText(t("common.save"));
    undoButton_->setText(t("writer.undo"));
    redoButton_->setText(t("writer.redo"));
    copyButton_->setText(t("writer.copy"));
    pasteButton_->setText(t("writer.paste"));
    boldButton_->setText(t("writer.bold"));
    fillButton_->setText(t("sheets.fill"));
    borderButton_->setText(t("sheets.borders"));
    sortAscButton_->setText(t("sheets.sort_asc"));
    sortDescButton_->setText(t("sheets.sort_desc"));
    newSheetButton_->setText(t("sheets.new_sheet"));
    deleteSheetButton_->setText(t("sheets.delete_sheet"));
    for (QToolButton* button : ribbon_->findChildren<QToolButton*>()) {
        const QString key = button->property("translationKey").toString();
        if (!key.isEmpty()) {
            const QByteArray utf8 = key.toLatin1();
            button->setText(t(utf8.constData()));
        }
    }
    protectedMessage_->setText(t("protected.title") + QStringLiteral(" — ") +
                               t("protected.message"));
    enableEditing_->setText(t("protected.enable_editing"));
    status_->setText(t("sheets.status"));
    updateTitle();
}

QTableWidget* SheetsWindow::addSheet(const QString& name) {
    auto* table = new QTableWidget(200, 50);
    table->setAlternatingRowColors(true);
    table->setSelectionMode(QAbstractItemView::ContiguousSelection);
    table->setSelectionBehavior(QAbstractItemView::SelectItems);
    table->horizontalHeader()->setDefaultSectionSize(100);
    table->verticalHeader()->setDefaultSectionSize(24);
    QStringList labels;
    for (int column = 0; column < table->columnCount(); ++column) {
        labels.append(columnName(column));
    }
    table->setHorizontalHeaderLabels(labels);
    const QString tabName = name.isEmpty() ? t("sheets.sheet").arg(sheets_->count() + 1) : name;
    sheets_->addTab(table, tabName);
    sheets_->setCurrentWidget(table);

    connect(table, &QTableWidget::currentCellChanged, this,
            [this](int, int, int, int) { updateFormulaBar(); });
    connect(table, &QTableWidget::itemChanged, this, [this, table](QTableWidgetItem* item) {
        const QString before = item->data(Qt::UserRole).toString();
        const QString after = item->text();
        if (before != after) {
            undoStack_->push(
                new CellEditCommand(table, item->row(), item->column(), before, after));
        }
        updateFormulaBar();
    });
    table->setCurrentCell(0, 0);
    return table;
}

QTableWidget* SheetsWindow::currentSheet() const {
    return qobject_cast<QTableWidget*>(sheets_->currentWidget());
}

void SheetsWindow::updateFormulaBar() {
    QTableWidget* table = currentSheet();
    if (table == nullptr || table->currentRow() < 0 || table->currentColumn() < 0) {
        cellName_->clear();
        formulaBar_->clear();
        return;
    }
    cellName_->setText(columnName(table->currentColumn()) +
                       QString::number(table->currentRow() + 1));
    QSignalBlocker blocker(formulaBar_);
    QTableWidgetItem* item = table->item(table->currentRow(), table->currentColumn());
    formulaBar_->setText(item != nullptr ? item->text() : QString());
}

void SheetsWindow::requestOpen(const QString& path, const bool protectedViewHint) {
    protectedViewHint_ = protectedViewHint;
    QString error;
    if (!worker_->inspect(path, &error)) {
        QMessageBox::critical(this, t("common.error"), error);
    }
}

void SheetsWindow::openTrusted(const QString& path, const bool protectedView) {
    const QString suffix = QFileInfo(path).suffix().toLower();
    bool opened = false;
    if (suffix == QStringLiteral("csv")) {
        opened = loadCsv(path);
    } else if (suffix == QStringLiteral("vestra-sheets")) {
        opened = readWorkbook(path);
    } else {
        QMessageBox::information(this, t("sheets.title"), t("sheets.backend_unavailable"));
        return;
    }
    if (opened) {
        currentPath_ = QFileInfo(path).absoluteFilePath();
        setProtectedView(protectedView);
        updateTitle();
    }
}

bool SheetsWindow::loadCsv(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this, t("common.error"), file.errorString());
        return false;
    }
    sheets_->clear();
    QTableWidget* table = addSheet(t("sheets.sheet").arg(1));
    QTextStream stream(&file);
    int row = 0;
    QSignalBlocker blocker(table);
    while (!stream.atEnd()) {
        const QStringList values = parseCsvRow(stream.readLine());
        if (row >= table->rowCount()) {
            table->insertRow(table->rowCount());
        }
        while (values.size() > table->columnCount()) {
            table->insertColumn(table->columnCount());
        }
        for (int column = 0; column < values.size(); ++column) {
            auto* item = new QTableWidgetItem(values[column]);
            item->setData(Qt::UserRole, values[column]);
            table->setItem(row, column, item);
        }
        ++row;
    }
    undoStack_->clear();
    return true;
}

bool SheetsWindow::saveCsv(const QString& path) {
    QTableWidget* table = currentSheet();
    if (table == nullptr) {
        return false;
    }
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, t("common.error"), file.errorString());
        return false;
    }
    QTextStream stream(&file);
    int lastRow = 0;
    int lastColumn = 0;
    for (int row = 0; row < table->rowCount(); ++row) {
        for (int column = 0; column < table->columnCount(); ++column) {
            if (table->item(row, column) != nullptr &&
                !table->item(row, column)->text().isEmpty()) {
                lastRow = row;
                lastColumn = std::max(lastColumn, column);
            }
        }
    }
    for (int row = 0; row <= lastRow; ++row) {
        QStringList values;
        for (int column = 0; column <= lastColumn; ++column) {
            QTableWidgetItem* item = table->item(row, column);
            values.append(csvCell(item != nullptr ? item->text() : QString()));
        }
        stream << values.join(u',') << '\n';
    }
    if (!file.commit()) {
        QMessageBox::critical(this, t("common.error"), file.errorString());
        return false;
    }
    return true;
}

bool SheetsWindow::readWorkbook(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, t("common.error"), file.errorString());
        return false;
    }
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    const QJsonObject root = document.object();
    if (root.value(QStringLiteral("format")).toString() != QStringLiteral("vestra-sheets-1")) {
        QMessageBox::critical(this, t("common.error"), t("sheets.open_failed"));
        return false;
    }
    sheets_->clear();
    for (const QJsonValue& sheetValue : root.value(QStringLiteral("sheets")).toArray()) {
        const QJsonObject sheetObject = sheetValue.toObject();
        QTableWidget* table = addSheet(sheetObject.value(QStringLiteral("name")).toString());
        QSignalBlocker blocker(table);
        for (const QJsonValue& cellValue : sheetObject.value(QStringLiteral("cells")).toArray()) {
            const QJsonObject cell = cellValue.toObject();
            const int row = cell.value(QStringLiteral("row")).toInt();
            const int column = cell.value(QStringLiteral("column")).toInt();
            if (row < 0 || column < 0 || row >= table->rowCount() ||
                column >= table->columnCount()) {
                continue;
            }
            auto* item = new QTableWidgetItem(cell.value(QStringLiteral("text")).toString());
            item->setData(Qt::UserRole, item->text());
            QFont font = item->font();
            font.setBold(cell.value(QStringLiteral("bold")).toBool());
            item->setFont(font);
            item->setTextAlignment(Qt::Alignment::fromInt(
                cell.value(QStringLiteral("alignment"))
                    .toInt(static_cast<int>(Qt::AlignLeft | Qt::AlignVCenter))));
            const QColor background(cell.value(QStringLiteral("background")).toString());
            if (background.isValid()) {
                item->setBackground(background);
            }
            table->setItem(row, column, item);
        }
    }
    if (sheets_->count() == 0) {
        addSheet();
    }
    sheets_->setCurrentIndex(0);
    undoStack_->clear();
    return true;
}

bool SheetsWindow::writeWorkbook(const QString& path) {
    QJsonArray sheets;
    for (int sheetIndex = 0; sheetIndex < sheets_->count(); ++sheetIndex) {
        auto* table = qobject_cast<QTableWidget*>(sheets_->widget(sheetIndex));
        QJsonArray cells;
        for (int row = 0; row < table->rowCount(); ++row) {
            for (int column = 0; column < table->columnCount(); ++column) {
                QTableWidgetItem* item = table->item(row, column);
                if (item == nullptr ||
                    (item->text().isEmpty() && item->background().style() == Qt::NoBrush &&
                     !item->font().bold())) {
                    continue;
                }
                cells.append(QJsonObject{
                    {QStringLiteral("row"), row},
                    {QStringLiteral("column"), column},
                    {QStringLiteral("text"), item->text()},
                    {QStringLiteral("bold"), item->font().bold()},
                    {QStringLiteral("alignment"), static_cast<int>(item->textAlignment())},
                    {QStringLiteral("background"),
                     item->background().color().name(QColor::HexArgb)},
                });
            }
        }
        sheets.append(QJsonObject{{QStringLiteral("name"), sheets_->tabText(sheetIndex)},
                                  {QStringLiteral("cells"), cells}});
    }
    QSaveFile file(path);
    const QJsonObject root{{QStringLiteral("format"), QStringLiteral("vestra-sheets-1")},
                           {QStringLiteral("sheets"), sheets}};
    if (!file.open(QIODevice::WriteOnly) ||
        file.write(QJsonDocument(root).toJson(QJsonDocument::Indented)) < 0 || !file.commit()) {
        QMessageBox::critical(this, t("common.error"), file.errorString());
        return false;
    }
    return true;
}

void SheetsWindow::openFile() {
    const QString path =
        QFileDialog::getOpenFileName(this, t("common.open"), QString(), t("sheets.open_filter"));
    if (!path.isEmpty()) {
        requestOpen(path);
    }
}

void SheetsWindow::saveFile(const bool forceDialog) {
    if (protectedBanner_->isVisible()) {
        return;
    }
    QString path = currentPath_;
    if (forceDialog || path.isEmpty()) {
        path = QFileDialog::getSaveFileName(
            this, t("common.save"), QStringLiteral("Book1.vestra-sheets"), t("sheets.save_filter"));
    }
    if (path.isEmpty()) {
        return;
    }
    const bool saved =
        QFileInfo(path).suffix().compare(QStringLiteral("csv"), Qt::CaseInsensitive) == 0
            ? saveCsv(path)
            : writeWorkbook(path);
    if (saved) {
        currentPath_ = QFileInfo(path).absoluteFilePath();
        status_->setText(t("sheets.saved"));
        updateTitle();
    }
}

void SheetsWindow::copySelection() {
    QTableWidget* table = currentSheet();
    const QList<QTableWidgetSelectionRange> ranges =
        table != nullptr ? table->selectedRanges() : QList<QTableWidgetSelectionRange>();
    if (ranges.isEmpty()) {
        return;
    }
    const QTableWidgetSelectionRange range = ranges.first();
    QStringList rows;
    for (int row = range.topRow(); row <= range.bottomRow(); ++row) {
        QStringList cells;
        for (int column = range.leftColumn(); column <= range.rightColumn(); ++column) {
            QTableWidgetItem* item = table->item(row, column);
            cells.append(item != nullptr ? item->text() : QString());
        }
        rows.append(cells.join(u'\t'));
    }
    QApplication::clipboard()->setText(rows.join(u'\n'));
}

void SheetsWindow::pasteSelection() {
    QTableWidget* table = currentSheet();
    if (table == nullptr || table->currentRow() < 0 || protectedBanner_->isVisible()) {
        return;
    }
    const QStringList rows = QApplication::clipboard()->text().split(u'\n');
    for (int rowOffset = 0; rowOffset < rows.size(); ++rowOffset) {
        const QStringList cells = rows[rowOffset].split(u'\t');
        for (int columnOffset = 0; columnOffset < cells.size(); ++columnOffset) {
            const int row = table->currentRow() + rowOffset;
            const int column = table->currentColumn() + columnOffset;
            if (row >= table->rowCount() || column >= table->columnCount()) {
                continue;
            }
            QTableWidgetItem* item = table->item(row, column);
            if (item == nullptr) {
                item = new QTableWidgetItem;
                item->setData(Qt::UserRole, QString());
                table->setItem(row, column, item);
            }
            item->setText(cells[columnOffset]);
        }
    }
}

void SheetsWindow::applyBold() {
    if (QTableWidget* table = currentSheet(); table != nullptr) {
        for (QTableWidgetItem* item : table->selectedItems()) {
            QFont font = item->font();
            font.setBold(!font.bold());
            item->setFont(font);
        }
    }
}

void SheetsWindow::applyAlignment(const Qt::Alignment alignment) {
    if (QTableWidget* table = currentSheet(); table != nullptr) {
        for (QTableWidgetItem* item : table->selectedItems()) {
            item->setTextAlignment(alignment);
        }
    }
}

void SheetsWindow::applyFill() {
    const QColor color = QColorDialog::getColor(Qt::yellow, this, t("sheets.fill"));
    if (color.isValid()) {
        if (QTableWidget* table = currentSheet(); table != nullptr) {
            for (QTableWidgetItem* item : table->selectedItems()) {
                item->setBackground(color);
            }
        }
    }
}

void SheetsWindow::applyBorders() {
    if (QTableWidget* table = currentSheet(); table != nullptr) {
        table->setShowGrid(!table->showGrid());
    }
}

void SheetsWindow::sortSelection(const Qt::SortOrder order) {
    QTableWidget* table = currentSheet();
    if (table != nullptr && table->currentColumn() >= 0) {
        table->sortItems(table->currentColumn(), order);
    }
}

void SheetsWindow::setProtectedView(const bool enabled) {
    protectedBanner_->setVisible(enabled);
    for (int index = 0; index < sheets_->count(); ++index) {
        qobject_cast<QTableWidget*>(sheets_->widget(index))
            ->setEditTriggers(enabled ? QAbstractItemView::NoEditTriggers
                                      : QAbstractItemView::AllEditTriggers);
    }
    saveButton_->setEnabled(!enabled);
    pasteButton_->setEnabled(!enabled);
}

void SheetsWindow::updateTitle() {
    const QString name =
        currentPath_.isEmpty() ? t("sheets.book") : QFileInfo(currentPath_).fileName();
    setWindowTitle(QStringLiteral("%1 — %2").arg(name, t("sheets.title")));
}

