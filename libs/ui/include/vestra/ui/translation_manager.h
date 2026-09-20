#pragma once

#include <QHash>
#include <QObject>
#include <QString>

namespace vestra::ui {

class TranslationManager final : public QObject {
    Q_OBJECT

  public:
    static TranslationManager& instance();
    bool setLanguage(const QString& language, QString* error = nullptr);
    [[nodiscard]] QString language() const;
    [[nodiscard]] QString text(const QString& key) const;

  signals:
    void languageChanged();

  private:
    TranslationManager();
    QHash<QString, QString> strings_;
    QString language_{QStringLiteral("en")};
};

[[nodiscard]] inline QString t(const char* key) {
    return TranslationManager::instance().text(QString::fromLatin1(key));
}

} // namespace vestra::ui

