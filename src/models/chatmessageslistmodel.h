#ifndef CHATMESSAGESLISTMODEL_H
#define CHATMESSAGESLISTMODEL_H

#include <QAbstractListModel>
#include <vector>

#include "services/chatservice.h"

/**
 * Модель данных списка сообщений для отображения в QListView.
 * Хранит историю сообщений конкретного чата и предоставляет данные для делегата (ChatMessagesItemDelegate).
 */
class ChatMessagesListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    /**
     * Роли данных для получения специфичных свойств сообщения через метод data().
     */
    enum MessageRoles
    {
        MessageIdRole = Qt::UserRole + 1,    //!< ID сообщения
        SenderIdRole,                        //!< ID отправителя
        ChatIdRole,                          //!< ID чата
        MessageTextRole,                     //!< Текст сообщения
        TimestampRole,                       //!< Временная метка отправки
        IsPendingRole,                       //!< Флаг ожидания отправки (на сервере еще не сохранено)
        ReadRole,                            //!< Флаг прочтения сообщения
        ReadAtRole,                          //!< Временная метка прочтения
        EditedRole,                          //!< Флаг того, что сообщение было изменено
        EditedAtRole,                        //!< Временная метка изменения
        HasAttachmentsRole,                  //!< Флаг наличия вложений
        AttachmentsCountRole,                //!< Количество вложений
        AttachmentsRole                      //!< Список объектов вложений
    };

    explicit ChatMessagesListModel(QObject *parent = nullptr);

    /**
     * Возвращает количество элементов в списке.
     * \param parent родительский индекс
     * \return количество сообщений
     */
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    /**
     * Возвращает данные для указанного индекса и роли.
     * \param index индекс элемента
     * \param role роль данных (из MessageRoles)
     * \return значение
     */
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    /**
     * Возвращает соответствие между ролями и их строковыми именами (для QML/дебага).
     * \return хэш с именами ролей
     */
    QHash<int, QByteArray> roleNames() const override;

    /**
     * Устанавливает новый список сообщений в модель и обновляет представление.
     * \param messages список объектов сообщений
     */
    void setMessages(const std::vector<ParsedChatMessagesArrayObject> &messages);

    /**
     * Добавляет одно новое сообщение в конец списка.
     * \param message объект добавляемого сообщения
     */
    void appendMessage(const ParsedChatMessagesArrayObject &message);

    /**
     * Очищает модель от всех сообщений.
     */
    void clear();

private:
    // --- Внутренние данные ---
    std::vector<ParsedChatMessagesArrayObject> m_messages; //!< Внутренний массив данных сообщений
};

#endif // CHATMESSAGESLISTMODEL_H
