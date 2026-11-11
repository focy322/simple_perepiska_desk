#include "settingslistmodel.h"

SettingsListModel::SettingsListModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_items{{{SettingId::Customization, tr("Кастомизация"),
                QIcon::fromTheme("preferences-desktop-theme", QIcon(":/icons/palette.svg"))}}}
{
}

int SettingsListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_items.size());
}

QVariant SettingsListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.model() != this || index.column() != 0
        || index.row() < 0 || index.row() >= rowCount())
        return {};

    const Item &item = m_items.at(index.row());
    switch (role) {
    case Qt::DisplayRole:
    case Qt::ToolTipRole:
    case Qt::AccessibleTextRole:
        return item.title;
    case Qt::DecorationRole:
        return item.icon;
    case SettingIdRole:
        return QVariant::fromValue(item.id);
    default:
        return {};
    }
}

QHash<int, QByteArray> SettingsListModel::roleNames() const
{
    auto roles = QAbstractListModel::roleNames();
    roles.insert(SettingIdRole, "settingId");
    return roles;
}
