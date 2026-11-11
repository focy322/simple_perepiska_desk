#include "stylemanager.h"
#include "stylehelper.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>

StyleManager::StyleManager(const QString &filePath, QObject *parent)
    : QObject(parent)
    , m_filePath(filePath.isEmpty() ? defaultFilePath() : filePath)
    , m_baseStyle(StyleHelper::load(":/styles/main.qss"))
    , m_ruleTemplate(StyleHelper::load(":/styles/customization-color-rule.qss"))
{
    const auto catalog = QJsonDocument::fromJson(StyleHelper::load(":/styles/customization-targets.json").toUtf8()).object();
    m_targets = catalog.value("targets").toObject();
    m_properties = catalog.value("properties").toObject();

    QFile file(m_filePath);
    if (file.open(QIODevice::ReadOnly))
        m_saved = QString::fromUtf8(file.readAll());
    if (m_saved.trimmed().isEmpty())
        m_saved = m_baseStyle;
    m_draft = m_saved;
}

QString StyleManager::defaultFilePath()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/Vent/style.qss";
}

QString StyleManager::targetKey(const QString &screen, const QString &objectName) const
{
    const QString scoped = screen + "." + objectName;
    return m_targets.contains(scoped) ? scoped : (m_targets.contains(objectName) ? objectName : QString());
}

QStringList StyleManager::properties(const QString &target) const
{
    QStringList result;
    for (const auto &property : m_targets.value(target).toObject().value("properties").toArray())
        result.append(property.toString());
    return result;
}

QString StyleManager::propertyTitle(const QString &property) const
{
    return m_properties.value(property).toObject().value("title").toString();
}

QString StyleManager::colorRule(const QString &target, const QString &property, const QColor &color) const
{
    const auto definition = m_targets.value(target).toObject();
    const auto colorProperty = m_properties.value(property).toObject();
    const QString selector = property == "text" && definition.contains("textSelector")
        ? definition.value("textSelector").toString() : definition.value("selector").toString();
    const QString value = colorProperty.value("value").toString()
        .arg(color.red()).arg(color.green()).arg(color.blue()).arg(color.alpha());
    return m_ruleTemplate.arg(target, property, selector, colorProperty.value("css").toString(), value).trimmed();
}

QPair<int, int> StyleManager::ruleRange(const QString &target, const QString &property) const
{
    const QString rule = colorRule(target, property, Qt::black);
    const QString first = rule.section('\n', 0, 0).trimmed();
    const QString last = rule.section('\n', -1).trimmed();
    const int start = m_draft.indexOf(first);
    const int end = start < 0 ? -1 : m_draft.indexOf(last, start + first.size());
    return {start, end < 0 ? -1 : end + last.size()};
}

QColor StyleManager::color(const QString &target, const QString &property, const QColor &fallback) const
{
    if (!properties(target).contains(property))
        return fallback;
    const auto [start, end] = ruleRange(target, property);
    if (start < 0 || end < 0)
        return fallback;
    static const QRegularExpression rgba("rgba\\(\\s*(\\d+)\\s*,\\s*(\\d+)\\s*,\\s*(\\d+)\\s*,\\s*(\\d+)\\s*\\)");
    const auto match = rgba.match(m_draft.mid(start, end - start));
    return match.hasMatch() ? QColor(match.captured(1).toInt(), match.captured(2).toInt(),
                                    match.captured(3).toInt(), match.captured(4).toInt()) : fallback;
}

bool StyleManager::setColor(const QString &target, const QString &property, const QColor &color)
{
    if (!color.isValid() || !properties(target).contains(property))
        return false;
    const QString rule = colorRule(target, property, color);
    const auto [start, end] = ruleRange(target, property);
    QString updated = m_draft;
    if (start >= 0 && end >= 0)
        updated.replace(start, end - start, rule);
    else
        updated += "\n\n" + rule + "\n";
    // Любой создаваемый конструктором стиль включает базу, даже если исходный файл был фрагментом.
    if (!updated.startsWith(m_baseStyle))
        updated = m_baseStyle + "\n\n" + updated;
    if (updated != m_draft) {
        m_draft = updated;
        emit draftChanged();
    }
    return true;
}

void StyleManager::discard()
{
    if (hasChanges()) {
        m_draft = m_saved;
        emit draftChanged();
    }
}

bool StyleManager::save(QString *error)
{
    const auto directory = QFileInfo(m_filePath).absoluteDir();
    if (!QDir().mkpath(directory.absolutePath())) {
        if (error)
            *error = tr("Не удалось создать каталог стилей: %1").arg(directory.absolutePath());
        return false;
    }
    QSaveFile file(m_filePath);
    const QString complete = m_draft.startsWith(m_baseStyle) ? m_draft : m_baseStyle + "\n\n" + m_draft;
    const QByteArray contents = complete.toUtf8();
    if (!file.open(QIODevice::WriteOnly) || file.write(contents) != contents.size() || !file.commit()) {
        if (error)
            *error = file.errorString();
        return false;
    }
    m_saved = m_draft = complete;
    emit saved();
    return true;
}
