#pragma once

#include <QMainWindow>

class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;
class QTabWidget;
class QToolButton;
class QUndoStack;

namespace vestra::sandbox {
class WorkerSession;
}

namespace vestra::ui {
class OfficeRibbon;
}

class SheetsWindow final : public QMainWindow {
    Q_OBJECT

  public:
    explicit SheetsWindow(QWidget* parent = nullptr);
    void requestOpen(const QString& path, bool protectedViewHint = false);

  private:
    void buildUi();
    void buildRibbon();
    void retranslate();
    QTableWidget* addSheet(const QString& name = {});
    [[nodiscard]] QTableWidget* currentSheet() const;
    void updateFormulaBar();
    void openTrusted(const QString& path, bool protectedView);
    bool loadCsv(const QString& path);
    bool saveCsv(const QString& path);
    bool readWorkbook(const QString& path);
    bool writeWorkbook(const QString& path);
    void openFile();
    void saveFile(bool forceDialog = false);
    void copySelection();
    void pasteSelection();
    void applyBold();
    void applyAlignment(Qt::Alignment alignment);
    void applyFill();
    void applyBorders();
    void sortSelection(Qt::SortOrder order);
    void setProtectedView(bool enabled);
    void updateTitle();

    vestra::sandbox::WorkerSession* worker_{nullptr};
    vestra::ui::OfficeRibbon* ribbon_{nullptr};
    QTabWidget* sheets_{nullptr};
    QUndoStack* undoStack_{nullptr};
    QWidget* protectedBanner_{nullptr};
    QLabel* protectedMessage_{nullptr};
    QPushButton* enableEditing_{nullptr};
    QLineEdit* cellName_{nullptr};
    QLineEdit* formulaBar_{nullptr};
    QLabel* status_{nullptr};
    QToolButton* openButton_{nullptr};
    QToolButton* saveButton_{nullptr};
    QToolButton* undoButton_{nullptr};
    QToolButton* redoButton_{nullptr};
    QToolButton* copyButton_{nullptr};
    QToolButton* pasteButton_{nullptr};
    QToolButton* boldButton_{nullptr};
    QToolButton* fillButton_{nullptr};
    QToolButton* borderButton_{nullptr};
    QToolButton* sortAscButton_{nullptr};
    QToolButton* sortDescButton_{nullptr};
    QToolButton* newSheetButton_{nullptr};
    QToolButton* deleteSheetButton_{nullptr};
    QString currentPath_;
    bool protectedViewHint_{false};
};

