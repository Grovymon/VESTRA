#pragma once

#include "vestra/settings/app_settings.h"
#include "vestra/ui/vestra_design.h"
#include <QMainWindow>
#include <QTextCharFormat>
#include <functional>

class PageEditor;
class QComboBox;
class QFontComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QStackedWidget;
class QTimer;
class QCheckBox;
namespace vestra::sandbox { class WorkerSession; }

class WriterWindow final : public QMainWindow {
    Q_OBJECT
  public:
    explicit WriterWindow(QWidget* parent = nullptr);
    void requestOpen(const QString& path, bool protectedViewHint = false);
  protected:
    void closeEvent(QCloseEvent* event) override;
  private:
    QWidget* buildHomeTab();
    QWidget* buildInsertTab();
    QWidget* buildLayoutTab();
    QWidget* buildReviewTab();
    QWidget* buildViewTab();
    QWidget* buildBackstage();
    QWidget* buildNavigation();
    vestra::ui::VestraRibbonButton* command(const char* key, const char* icon,
        vestra::ui::RibbonButtonSize size, const std::function<void()>& handler,
        bool edits = false, const QKeySequence& shortcut = {});
    void retranslate();
    void synchronizeFormat();
    void updateDocumentStats();
    void updateWindowTitle();
    void openDocumentDialog();
    void newDocument(int templateIndex = 0);
    bool confirmDiscard();
    void openTrusted(const QString& path, bool protectedView);
    bool save();
    bool saveAs();
    bool writeDocument(const QString& path);
    void exportPdf();
    void printDocument();
    void printPreview();
    void mergeFormat(const QTextCharFormat& format);
    void toggleCharacter(const QString& attribute);
    void changeFontSize(int delta);
    void chooseColor(bool highlight);
    void setListStyle(int style);
    void setAlignment(Qt::Alignment alignment);
    void changeIndent(int delta);
    void applyStyle(int index);
    void insertTable();
    void insertImage();
    void insertLink();
    void insertPageBreak();
    void paragraphDialog();
    void pageDialog();
    void findText();
    void replaceText();
    void showNavigation(bool replacement = false);
    void setProtectedView(bool enabled);
    void writeRecovery();
    void refreshRecent();
    void showSettings();
    void applyWriterTheme();

    vestra::settings::AppSettings settings_;
    vestra::sandbox::WorkerSession* worker_{nullptr};
    PageEditor* editor_{nullptr};
    vestra::ui::VestraRibbon* ribbon_{nullptr};
    vestra::ui::VestraStatusBar* status_{nullptr};
    QStackedWidget* body_{nullptr};
    QWidget* protectedBanner_{nullptr};
    QLabel* protectedMessage_{nullptr};
    QPushButton* enableEditing_{nullptr};
    QLabel* documentTitle_{nullptr};
    QFontComboBox* fontFamily_{nullptr};
    QComboBox* fontSize_{nullptr};
    QWidget* navigation_{nullptr};
    QLineEdit* query_{nullptr};
    QLineEdit* replacement_{nullptr};
    QCheckBox* matchCase_{nullptr};
    QLabel* searchResult_{nullptr};
    QListWidget* recent_{nullptr};
    QTimer* recoveryTimer_{nullptr};
    QString currentPath_;
    QString recoveryId_;
    bool protectedView_{false};
    bool protectedViewHint_{false};
    bool opening_{false};
    bool formatPainter_{false};
    QTextCharFormat paintedFormat_;
};

