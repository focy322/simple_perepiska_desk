#ifndef SEARCHLISTMODEL_H
#define SEARCHLISTMODEL_H

#include <QAbstractListModel>
#include <vector>

#include "services/userinfoservice.h"

/**
 * Модель данных списка пользователей для отображения результатов поиска.
 * Хранит найденных пользователей и предоставляет данные для делегата (SearchItemDelegate).
 */
class SearchListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    /**
     * Роли данных для получения свойств пользователя через метод data().
     */
    enum ChatRoles
    {
        UserIdRole = Qt::UserRole + 1,       //!< ID пользователя
        UsernameRole,                        //!< Имя пользователя
        NicknameRole,                        //!< Никнейм пользователя
        LastSeenRole,                        //!< Временная метка последней активности
        AvatarFileUrlRole,                   //!< Ссылка на аватар пользователя
    };

    explicit SearchListModel(QObject *parent = nullptr);

    /**
     * Возвращает количество элементов в списке.
     * \param parent родительский индекс
     * \return количество пользователей
     */
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    /**
     * Возвращает данные для указанного индекса и роли.
     * \param index индекс элемента
     * \param role роль данных (из ChatRoles)
     * \return значение
     */
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    /**
     * Возвращает соответствие между ролями и их строковыми именами.
     * \return хэш с именами ролей
     */
    QHash<int, QByteArray> roleNames() const override;

    /**
     * Устанавливает новый список пользователей в модель и обновляет представление.
     * \param users список объектов пользователей
     */
    void setUsers(const std::vector<ParsedFoundUsersObject> &users);

    /**
     * Очищает модель от всех пользователей.
     */
    void clear();

private:
    // --- Внутренние данные ---
    std::vector<ParsedFoundUsersObject> m_users; //!< Внутренний массив данных пользователей
};

#endif // SEARCHLISTMODEL_H
