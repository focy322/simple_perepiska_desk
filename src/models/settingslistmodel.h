#ifndef SETTINGSLISTMODEL_H
#define SETTINGSLISTMODEL_H

#include <QAbstractListModel>
#include <QIcon>
#include <array>

class SettingsListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum class SettingId
    {
        Customization
    };
    Q_ENUM(SettingId)

    enum Roles
    {
        SettingIdRole = Qt::UserRole + 1
    };

    explicit SettingsListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

private:
    struct Item
    {
        SettingId id;
        QString title;
        QIcon icon;
    };

    const std::array<Item, 1> m_items;
};

#endif // SETTINGSLISTMODEL_H
