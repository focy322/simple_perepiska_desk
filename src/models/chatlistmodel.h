#ifndef CHATLISTMODEL_H
#define CHATLISTMODEL_H

#include <QAbstractListModel>
#include <vector>
#include <QPixmap>
#include <QMap>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>

#include "services/chatservice.h"

/**
 * Модель данных списка чатов для отображения в QListView.
 * Хранит список чатов пользователя и предоставляет данные для делегата (ChatListItemDelegate).
 */
class ChatListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    /**
     * Роли данных для получения специфичных свойств чата через метод data().
     */
    enum ChatRoles
    {
        ChatIdRole = Qt::UserRole + 1,       //!< ID чата
        ChatNameRole,                        //!< Название чата
        LastMessageRole,                     //!< Текст последнего сообщения
        LastMessageTimestampRole,            //!< Временная метка последнего сообщения
        AvatarFileIdRole,                    //!< ID файла аватара чата
        ChatTypeRole,                        //!< Тип чата (личный/групповой)
        UserIdRole,                          //!< ID собеседника (для личных чатов)
        UsernameRole,                        //!< Имя собеседника (для личных чатов)
        NicknameRole,                        //!< Никнейм собеседника (для личных чатов)
        UserAvatarFileUrlRole,               //!< Ссылка на аватар собеседника
        UnreadCountRole,                     //!< Количество непрочитанных сообщений
        LastMessageHasAttachmentsRole,       //!< Флаг наличия вложений в последнем сообщении
        LastMessageAttachmentTypeRole,       //!< Тип вложения (изображение, файл и т.д.)
        LastMessageAttachmentsCountRole,     //!< Количество вложений
        AvatarPixmapRole                     //!< Загруженное изображение аватара (QPixmap)
    };

    explicit ChatListModel(QObject *parent = nullptr);

    /**
     * Возвращает количество элементов в списке.
     * \param parent родительский индекс
     * \return количество чатов
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
     * Возвращает соответствие между ролями и их строковыми именами (для QML/дебага).
     * \return хэш с именами ролей
     */
    QHash<int, QByteArray> roleNames() const override;

    /**
     * Устанавливает новый список чатов в модель и обновляет представление.
     * \param chats список объектов чатов
     */
    void setChats(const std::vector<ParsedChatsListArrayObject> &chats);

    /**
     * Очищает модель от всех чатов.
     */
    void clear();

    /**
     * Уменьшает количество непрочитанных сообщений для указанного чата.
     * \param chatId идентификатор чата
     * \param count на сколько уменьшить (по умолчанию 1)
     */
    void decreaseUnreadCount(quint64 chatId, int count);

    /**
     * Устанавливает точное количество непрочитанных сообщений для указанного чата.
     * \param chatId идентификатор чата
     * \param count новое количество непрочитанных
     */
    void setUnreadCount(quint64 chatId, int count);

private slots:
    // --- Внутренние обработчики ---

    /**
     * Слот-обработчик успешного скачивания аватара.
     * \param reply ответ от сети
     * \param row индекс строки чата в модели
     * \param url исходный URL аватара
     */
    void onAvatarDownloaded(QNetworkReply *reply, int row, const QString &url);

private:
    // --- Внутренние методы ---

    /**
     * Инициирует загрузку аватарок для чатов, у которых они еще не загружены.
     */
    void fetchAvatars();

    // --- Внутренние данные ---
    std::vector<ParsedChatsListArrayObject> m_chats; //!< Внутренний массив данных чатов
    QNetworkAccessManager *m_networkManager;         //!< Менеджер сети для загрузки аватарок
    QMap<QString, QPixmap> m_avatarCache;            //!< Кэш загруженных аватарок по URL
};

#endif // CHATLISTMODEL_H
