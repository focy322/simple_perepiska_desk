//
// Created by mark on 10/12/26.
//

#ifndef VENT_PARSED_OBJECTS_H
#define VENT_PARSED_OBJECTS_H

#include <QString>
#include <QJsonArray>
#include <QMetaType>

#include <climits>
#include <vector>

/// Файл содержит структуры для хранения распашенных данных от сервера

/**
 * Структура для хранения распарсенных данных о собеседнике в private-чате.
 */
struct Interlocutor
{
    quint64            userId = ULONG_LONG_MAX;           //!< Идентификатор пользователя
    QString            username;                          //!< Логин собеседника
    QString            nickname;                          //!< Никнейм собеседника
    QString            userAvatarFileUrl;                 //!< Ссылка на аватар пользователя

};

/**
 * Структура для хранения распарсенных данных о предварительном просмотре сообщения(в списке чатов).
 */
struct MessagePreview
{
    quint64            messageId = ULONG_LONG_MAX;        //!< Идентификатор сообщения
    quint64            senderId = ULONG_LONG_MAX;         //!< Идентификатор отправителя
    QString            message;                           //!< Текст сообщения
    QString            timestamp;                         //!< Временная метка отправки

    /**
     * Возможные категории
     * image
     * video
     * audio
     * document
     */
    QString            category;                          //!< Тип аттачмента
    uint               attachmentsCount = 0;              //!< Количество вложений
    bool               isRead = false;                    //!< Флаг прочтения
    bool               hasAttachments = false;            //!< Флаг наличия вложений
};

/**
 * Структура для хранения распарсенных данных о чате из списка чатов.
 */
struct ParsedChatsListArrayObject
{
    // Поля отправленные от сервера
    quint64            chatId = ULONG_LONG_MAX;           //!< Идентификатор чата
    quint64            chatAvatarFileId = ULONG_LONG_MAX; //!< Идентификатор аватара чата
    QString            chatName;                          //!< Имя чата
    QString            type;                              //!< Тип чата
    MessagePreview     lastMessage;                       //!< Данные последнего сообщения в чате
    Interlocutor       interlocutor;                      //!< Данные собеседника (для private-чата)
    uint               unreadCount = 0;                   //!< Количество непрочитанных сообщений

    // Локальные поля
    bool               isPending = false;                 //!< При отправке сообщения для отрисовки значка ожидания(для локалки)
};

/**
 *  Структура для описания attachment'а
 */
struct Attachment
{
    quint64           fileId = ULONG_LONG_MAX;
    quint64           duration = 0;
    QString           fileName;
    QString           contentType;
    QString           category;
    QString           blurhash;
    QString           downloadUrl;
    QString           thumbnailUrl;
    QString           localPath;
    std::vector<int>  waveform;
    uint              fileSize = 0;
};

Q_DECLARE_METATYPE(Attachment)

/**
 * Структура для хранения распарсенных данных об отдельном сообщении.
 */
struct ParsedChatMessagesArrayObject
{
    // Поля отправленные от сервера
    quint64                         messageId = ULONG_LONG_MAX;        //!< Идентификатор сообщения
    quint64                         senderId = ULONG_LONG_MAX;         //!< Идентификатор отправителя
    quint64                         chatId = ULONG_LONG_MAX;           //!< Идентификатор чата
    QString                         message;                           //!< Текст сообщения
    QString                         timestamp;                         //!< Временная метка отправки
    QString                         readAt;                            //!< Временная метка прочтения
    QString                         editedAt;                          //!< Временная метка редактирования
    std::vector<Attachment>         attachments;                       //!< Массив вложений
    bool                            isEdited = false;                  //!< Флаг редактирования
    bool                            isRead = false;                    //!< Флаг прочтения

    // Локальные поля
    QString                         clientMessageId;                   //!< Временный идентификатор на клиенте(uuid)
    uint                            attachmentsCount = 0;              //!< Количество вложений
    bool                            isPending = false;                 //!< Флаг ожидания отправки
    bool                            hasAttachments = false;            //!< Флаг наличия вложений
};

struct FileMeta
{
    quint64    fileId = ULONG_LONG_MAX;           //!< Идентификатор файла на сервере
    quint64    fileSize = 0;                      //!< Размер файла в байтах
    quint64    duration = 0;                      //!< Длительность файла
    QString    fileName;                          //!< Имя загруженного файла
    QString    contentType;                       //!< MIME-тип содержимого
    QString    uploadedAt;                        //!< Временная метка загрузки
    QString    category;
    QString    blurhash;
    std::vector<int> waveform;
};

/**
 * Структура для хранения распарсенных данных об успешно загруженном файле на сервер.
 */
struct ParsedUploadedFileInfo : FileMeta
{ };

/**
 * Структура для хранения распарсенных данных (метаданных) о файле для его скачивания.
 */
struct ParsedDownloadedFileInfo : FileMeta
{
    QString downloadFileUrl;    //!< URL для скачивания
    QString thumbnailUrl;       //!< URL для скачивания превью
};

/**
 * Структура для хранения распарсенных данных о найденном пользователе.
 */
struct ParsedFoundUsersObject
{
    quint64            userId = ULONG_LONG_MAX;           //!< Идентификатор пользователя
    QString            username;                          //!< Логин пользователя
    QString            nickname;                          //!< Никнейм (отображаемое имя)
    QString            lastSeen;                          //!< Временная метка последней активности
    QString            avatarFileUrl;                     //!< Ссылка на аватар
};

/**
 * Структура для хранения распарсенных данных о принятом сервером сообщении.
 */
struct ParsedMessageAcceptedObject
{
    QString            clientMessageId;                   //!< Временный ID на клиенте (UUID)
    QString            timestamp;                         //!< Время принятия сервером
    quint64            messageId = 0;                     //!< Постоянный ID сообщения на сервере
    quint64            chatId = 0;                        //!< Идентификатор чата
    bool               deduped = false;                   //!< Флаг дедупликации (если сообщение отправлено дважды)
};

/**
 * @brief Парсит attachment'ы в вектор
 * @param arr - массив attachment'ов
 * @return распаршенный вектор
 */
std::vector<Attachment> parseAttachments(const QJsonArray &arr);

/**
 * Разбирает JSON-документ со списком чатов в вектор структур ParsedChatsListArrayObject.
 * \param doc JSON документ от API
 * \return список чатов
 */
std::vector<ParsedChatsListArrayObject> parseChatsListArray(const QJsonDocument &doc);

/**
 * Разбирает JSON-документ с истории сообщений в вектор структур ParsedChatMessagesArrayObject.
 * \param doc JSON документ от API
 * \return список сообщений
 */
std::vector<ParsedChatMessagesArrayObject> parseChatMessagesArray(const QJsonDocument &doc);

/**
 * Разбирает JSON-документ с информацией о конкретном сообщении в структуру ParsedChatMessagesArrayObject.
 * \param messageObject JSON документ от API
 * \return структура с информацией о сообщении
 */
ParsedChatMessagesArrayObject parseChatMessageObject(const QJsonObject &messageObject);

/**
 * Разбирает JSON-документ с информацией об успешно загруженном файле.
 * \param doc JSON документ от API
 * \return структура с информацией о загруженном файле
 */
ParsedUploadedFileInfo parseUploadedFileInfo(const QJsonDocument &doc);

/**
 * Разбирает JSON-документ с информацией о файле, доступном для скачивания.
 * \param doc JSON документ от API
 * \return структура с метаданными файла
 */
ParsedDownloadedFileInfo parseDownloadedFileInfo(const QJsonDocument &doc);

/**
 * Разбирает JSON-документ со списком найденных пользователей в вектор структур ParsedFoundUsersObject.
 * \param doc JSON документ от API
 * \return список найденных пользователей
 */
std::vector<ParsedFoundUsersObject> parseFoundUsersArray(const QJsonDocument &doc);

/// Безопасное преобразование JSON-числа к unsigned long long.
quint64 ConvertToUnsignedLongLong(const QJsonValue &value, unsigned long long defaultValue = ULONG_LONG_MAX);

/// Разбирает JSON-объект с информацией о принятом сервером сообщении в структуру ParsedMessageAcceptedObject.
std::optional<ParsedMessageAcceptedObject> parseMessageAcceptedObject(const QJsonObject &payload);

#endif //VENT_PARSED_OBJECTS_H
