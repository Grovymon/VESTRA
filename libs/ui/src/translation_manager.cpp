#include "vestra/ui/translation_manager.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QResource>

#include <utility>

int qInitResources_vestra_resources();

namespace {
void initializeVestraResources() { ::qInitResources_vestra_resources(); }
} // namespace

namespace vestra::ui {

TranslationManager& TranslationManager::instance() {
    static TranslationManager manager;
    return manager;
}

TranslationManager::TranslationManager() {
    initializeVestraResources();
    setLanguage(QStringLiteral("en"));
}

bool TranslationManager::setLanguage(const QString& language, QString* error) {
    const QString normalized =
        language == QStringLiteral("ru") ? QStringLiteral("ru") : QStringLiteral("en");
    QFile file(QStringLiteral(":/vestra/i18n/%1.json").arg(normalized));
    if (!file.open(QIODevice::ReadOnly)) {
        if (error != nullptr) {
            *error = QStringLiteral("Could not open translation catalog: %1").arg(normalized);
        }
        return false;
    }
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (error != nullptr) {
            *error =
                QStringLiteral("Invalid translation catalog: %1").arg(parseError.errorString());
        }
        return false;
    }
    QHash<QString, QString> next;
    const QJsonObject object = document.object();
    for (auto iterator = object.constBegin(); iterator != object.constEnd(); ++iterator) {
        if (iterator.value().isString()) {
            next.insert(iterator.key(), iterator.value().toString());
        }
    }
    strings_ = std::move(next);
    language_ = normalized;
    emit languageChanged();
    return true;
}

QString TranslationManager::language() const { return language_; }

QString TranslationManager::text(const QString& key) const { return strings_.value(key, key); }

} // namespace vestra::ui

