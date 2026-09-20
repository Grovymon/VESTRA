#include "vestra/ui/theme_manager.h"

#include <QApplication>
#include <QPalette>
#include <QStyle>
#include <QStyleHints>

namespace vestra::ui {

void ThemeManager::apply(const settings::Theme theme) {
    settings::Theme effectiveTheme = theme;
    if (theme == settings::Theme::System) {
        effectiveTheme = QApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark
                             ? settings::Theme::Dark
                             : settings::Theme::Light;
    }
    QApplication::setPalette(QApplication::style()->standardPalette());
    if (effectiveTheme == settings::Theme::Dark) {
        QPalette palette;
        palette.setColor(QPalette::Window, QColor(30, 32, 36));
        palette.setColor(QPalette::WindowText, QColor(238, 240, 244));
        palette.setColor(QPalette::Base, QColor(23, 25, 28));
        palette.setColor(QPalette::AlternateBase, QColor(38, 41, 46));
        palette.setColor(QPalette::Text, QColor(238, 240, 244));
        palette.setColor(QPalette::Button, QColor(43, 47, 53));
        palette.setColor(QPalette::ButtonText, QColor(238, 240, 244));
        palette.setColor(QPalette::Highlight, QColor(59, 103, 209));
        palette.setColor(QPalette::HighlightedText, Qt::white);
        palette.setColor(QPalette::PlaceholderText, QColor(160, 165, 175));
        qApp->setPalette(palette);
        qApp->setStyleSheet(QStringLiteral(
            "QToolTip{color:#f5f6f8;background:#292c31;border:1px solid #555b65;}"
            "QPushButton{padding:7px 12px;border:1px solid "
            "#555b65;border-radius:6px;background:#2b2f35;}"
            "QPushButton:hover{background:#353a42;border-color:#6d7684;}"
            "QPushButton:disabled{color:#7d838c;background:#25282d;}"
            "QToolButton{padding:5px;border:1px solid transparent;border-radius:5px;}"
            "QToolButton:hover{background:#353a42;border-color:#5a6370;}"
            "QListWidget,QTextEdit,QLineEdit,QComboBox,QSpinBox{border:1px solid "
            "#4a505a;border-radius:5px;background:#17191c;}"
            "QTabWidget::pane{border:1px solid #454b55;}"
            "QTabBar::tab{padding:8px 15px;}QTabBar::tab:selected{color:#79a0ff;border-bottom:2px "
            "solid #5b83e6;}"
            "#OfficeRibbon::pane{border:0;border-top:1px solid #454b55;border-bottom:1px solid "
            "#454b55;background:#25282d;}"
            "#OfficeRibbon QTabBar::tab{background:transparent;border:0;padding:8px 17px;}"
            "#RibbonPage{background:#25282d;}#RibbonGroup{border-right:1px solid "
            "#454b55;background:transparent;}"
            "#RibbonGroupTitle{color:#aeb4bf;font-size:10px;}"
            "#VestraTitleBar{background:#234f91;color:white;}"
            "#VestraTitleBar QLabel{color:white;}"
            "#QuickAccessButton{color:white;border:0;background:transparent;padding:3px;}"
            "#QuickAccessButton:hover{background:#3a67a4;border:0;}"
            "#VestraRibbon::pane{border:0;border-top:1px solid #454b55;border-bottom:1px solid "
            "#454b55;background:#25282d;}"
            "#VestraRibbon QTabBar::tab{height:32px;background:#234f91;color:white;border:0;"
            "padding:0 16px;}"
            "#VestraRibbon QTabBar::tab:selected{background:#25282d;color:#f4f6fa;"
            "border-bottom:2px solid #7ea5ef;}"
            "#VestraRibbon QTabBar::tab:hover:!selected{background:#35619d;}"
            "#VestraRibbonPage{background:#25282d;}"
            "#VestraRibbonGroup{background:transparent;border:0;border-right:1px solid #454b55;}"
            "#VestraRibbonGroupTitle{color:#aeb4bf;font-size:10px;}"
            "#VestraRibbonButton{font-size:11px;padding:3px;border-radius:5px;}"
            "#VestraRibbonButton:hover{background:#353a42;border:1px solid #5a6370;}"
            "#VestraRibbon QComboBox{min-height:26px;padding:0 6px;}"
            "#VestraStatusBar{background:#202328;border-top:1px solid #454b55;}"
            "#VestraStatusBar QLabel{font-size:11px;color:#d5d9e0;}"
            "#StatusMessage{color:#91b0ef;}"
            "#StatusZoomButton,#StatusViewButton{padding:1px;border:0;min-width:20px;min-height:"
            "20px;}"
            "#VestraRuler{background:#292c31;}"
            "#Workspace{background:#202328;}#Paper{background:#fff;color:#202124;border:1px solid "
            "#111;}"
            "#TitleBar{background:#25282d;border-bottom:1px solid #454b55;}"
            "#LauncherTitle{color:#82a7ff;}#LauncherTagline{color:#b4bbc6;}"
            "#ModuleCard{background:#292d33;border:1px solid #454b55;border-radius:12px;}"
            "#ModuleCard:hover{border-color:#7798e8;background:#303640;}"
            "QPushButton[moduleCard=\"true\"]{background:#292d33;border:1px solid #454b55;}"
            "QPushButton[moduleCard=\"true\"]:hover{background:#303640;border-color:#7798e8;}"
            "QTableWidget{gridline-color:#454b55;background:#1d2024;alternate-background-color:#"
            "23272c;}"));
        return;
    }

    QPalette palette = QApplication::style()->standardPalette();
    palette.setColor(QPalette::Highlight, QColor(45, 91, 203));
    qApp->setPalette(palette);
    qApp->setStyleSheet(QStringLiteral(
        "QPushButton{padding:7px 12px;border:1px solid "
        "#c7ccd5;border-radius:6px;background:#ffffff;}"
        "QPushButton:hover{background:#f1f5ff;border-color:#8aa8eb;}"
        "QPushButton:disabled{color:#9da3ad;background:#f2f3f5;}"
        "QToolButton{padding:5px;border:1px solid transparent;border-radius:5px;}"
        "QToolButton:hover{background:#eef3ff;border-color:#c5d3ef;}"
        "QListWidget,QTextEdit,QLineEdit,QComboBox,QSpinBox{border:1px solid "
        "#cbd0d8;border-radius:5px;background:#ffffff;}"
        "QTabWidget::pane{border:1px solid #d5d9df;}"
        "QTabBar::tab{padding:8px 15px;}QTabBar::tab:selected{color:#2455bd;border-bottom:2px "
        "solid #2d5bcb;}"
        "#OfficeRibbon::pane{border:0;border-top:1px solid #d7dbe2;border-bottom:1px solid "
        "#c9ced7;background:#fff;}"
        "#OfficeRibbon QTabBar::tab{background:transparent;border:0;padding:8px 17px;}"
        "#RibbonPage{background:#fff;}#RibbonGroup{border-right:1px solid "
        "#dfe2e7;background:transparent;}"
        "#RibbonGroupTitle{color:#717780;font-size:10px;}"
        "#VestraTitleBar{background:#285596;color:white;}"
        "#VestraTitleBar QLabel{color:white;}"
        "#QuickAccessButton{color:white;border:0;background:transparent;padding:3px;}"
        "#QuickAccessButton:hover{background:#3d68a4;border:0;}"
        "#VestraRibbon::pane{border:0;border-top:0;border-bottom:1px solid "
        "#c9ced7;background:#fff;}"
        "#VestraRibbon QTabBar::tab{height:32px;background:#285596;color:white;border:0;"
        "padding:0 16px;}"
        "#VestraRibbon QTabBar::tab:selected{background:#fff;color:#234f91;"
        "border-bottom:2px solid #2f64b2;}"
        "#VestraRibbon QTabBar::tab:hover:!selected{background:#3b67a4;}"
        "#VestraRibbonPage{background:#fff;}"
        "#VestraRibbonGroup{background:transparent;border:0;border-right:1px solid #dfe2e7;}"
        "#VestraRibbonGroupTitle{color:#6d737c;font-size:10px;}"
        "#VestraRibbonButton{font-size:11px;padding:3px;border-radius:5px;}"
        "#VestraRibbonButton:hover{background:#eef3ff;border:1px solid #c5d3ef;}"
        "#VestraRibbon QComboBox{min-height:26px;padding:0 6px;}"
        "#VestraStatusBar{background:#f7f8fa;border-top:1px solid #cfd4dc;}"
        "#VestraStatusBar QLabel{font-size:11px;color:#3d424a;}"
        "#StatusMessage{color:#285596;}"
        "#StatusZoomButton,#StatusViewButton{padding:1px;border:0;min-width:20px;min-height:20px;}"
        "#VestraRuler{background:#fafafa;}"
        "#Workspace{background:#e7e9ed;}#Paper{background:#fff;color:#202124;border:1px solid "
        "#c7cbd2;}"
        "#TitleBar{background:#f8f9fb;border-bottom:1px solid #d7dbe2;}"
        "#LauncherTitle{color:#254f9e;}#LauncherTagline{color:#68717f;}"
        "#ModuleCard{background:#fff;border:1px solid #dfe2e7;border-radius:12px;}"
        "#ModuleCard:hover{border-color:#91aceb;background:#f8faff;}"
        "QPushButton[moduleCard=\"true\"]{background:#fff;border:1px solid #dfe2e7;}"
        "QPushButton[moduleCard=\"true\"]:hover{background:#f8faff;border-color:#91aceb;}"
        "QTableWidget{gridline-color:#d9dde3;background:#fff;alternate-background-color:#fafbfc;"
        "}"));
}

} // namespace vestra::ui

