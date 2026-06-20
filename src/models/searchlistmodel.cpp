#include "searchlistmodel.h"

SearchListModel::SearchListModel(QObject *parent)
    : QAbstractListModel(parent)
{}

int SearchListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return static_cast<int>(m_users.size());
}

QVariant SearchListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    const int row = index.row();
    if (row < 0 || row >= static_cast<int>(m_users.size()))
        return {};

    const ParsedFoundUsersObject &user = m_users[static_cast<size_t>(row)];


    switch (role)
    {
    case Qt::DisplayRole:
        return QVariant::fromValue<qulonglong>(user.userId);
    case UserIdRole:
        return QVariant::fromValue<qulonglong>(user.userId);
    case UsernameRole:
        return user.username.trimmed();
    case NicknameRole:
        return user.nickname.trimmed();
    case LastSeenRole:
        return user.lastSeen;
    case AvatarFileUrlRole:
        return user.avatarFileUrl;
    default:
        return {};
    }
}

QHash<int, QByteArray> SearchListModel::roleNames() const
{

    QHash<int, QByteArray> roles;
    roles[UserIdRole] = "userId";
    roles[UsernameRole] = "username";
    roles[NicknameRole] = "nickname";
    roles[LastSeenRole] = "lastSeen";
    roles[AvatarFileUrlRole] = "avatarFileId";
    return roles;
}

void SearchListModel::setUsers(const std::vector<ParsedFoundUsersObject> &users)
{
    beginResetModel();
    m_users = users;
    endResetModel();
}

void SearchListModel::clear()
{
    beginResetModel();
    m_users.clear();
    endResetModel();
}

