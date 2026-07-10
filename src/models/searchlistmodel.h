#ifndef SEARCHLISTMODEL_H
#define SEARCHLISTMODEL_H

#include <QAbstractListModel>
#include <vector>

#include "services/userinfoservice.h"

class SearchListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum ChatRoles
    {
        UserIdRole = Qt::UserRole + 1,
        UsernameRole,
        NicknameRole,
        LastSeenRole,
        AvatarFileUrlRole,
    };

    explicit SearchListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setUsers(const std::vector<ParsedFoundUsersObject> &users);
    void clear();

private:
    std::vector<ParsedFoundUsersObject> m_users;
};




#endif // SEARCHLISTMODEL_H
