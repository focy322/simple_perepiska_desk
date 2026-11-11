#ifndef STYLEMANAGER_H
#define STYLEMANAGER_H

#include <QColor>
#include <QJsonObject>
#include <QObject>
#include <QStringList>

/// Полный сохранённый стиль и независимый черновик конструктора в памяти.
class StyleManager : public QObject
{
    Q_OBJECT
public:
    explicit StyleManager(const QString &filePath = QString(), QObject *parent = nullptr);

    static QString defaultFilePath();
    QString filePath() const { return m_filePath; }
    QString styleSheet() const { return m_draft; }
    QString savedStyleSheet() const { return m_saved; }
    bool hasChanges() const { return m_draft != m_saved; }

    QString targetKey(const QString &screen, const QString &objectName) const;
    QStringList properties(const QString &target) const;
    QString propertyTitle(const QString &property) const;
    QColor color(const QString &target, const QString &property, const QColor &fallback) const;
    bool setColor(const QString &target, const QString &property, const QColor &color);
    void discard();
    bool save(QString *error = nullptr);

signals:
    void draftChanged();
    void saved();

private:
    QString colorRule(const QString &target, const QString &property, const QColor &color) const;
    QPair<int, int> ruleRange(const QString &target, const QString &property) const;

    QString m_filePath;
    QString m_baseStyle;
    QString m_saved;
    QString m_draft;
    QString m_ruleTemplate;
    QJsonObject m_targets;
    QJsonObject m_properties;
};

#endif // STYLEMANAGER_H
