#include "chatservice.h"
#include <cmath>
#include <algorithm>
#include <QUrlQuery>

ChatService::ChatService(QObject *parent)
    : QObject{parent}
    , network(new QNetworkAccessManager(this))
    , baseUrl(baseHttpUrl)
    , myChatsUrl("/api/chats/")
    , chatMessagesUrl("/api/chats/%1")
    , createDirectChatUrl("/api/chats/create")
{}

void ChatService::getMyChats(const QString &accToken)
{
    emit getMyChatsInProgress();
    QUrl url(baseUrl + myChatsUrl);
    QNetworkRequest req(url);
    // Передаем токен в заголовке
    req.setRawHeader("Authorization", "Bearer " + accToken.toUtf8());

    QNetworkReply * reply = network->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply](){
        auto httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (reply->error() != QNetworkReply::NoError && httpCode == 0)
        {
            NetworkResult res{false, ERROR_TYPES::UNKNOWN_ERROR, messageForError(ERROR_TYPES::UNKNOWN_ERROR)};
            emit getMyChatsFinished(res);
            reply->deleteLater();
            return;
        }
        QByteArray raw = reply->readAll();
        QJsonParseError pe;
        QJsonDocument doc = QJsonDocument::fromJson(raw, &pe);
        if (pe.error || !doc.isArray())
        {
            NetworkResult res{false, ERROR_TYPES::UNKNOWN_ERROR, messageForError(ERROR_TYPES::UNKNOWN_ERROR)};
            emit getMyChatsFinished(res);
            reply->deleteLater();
            return;
        }
        if (httpCode == 200)
        {
            const auto parsedArrayObjects = parseChatsListArray(doc);
            NetworkResult res{true, ERROR_TYPES::NO_ERROR, messageForError(ERROR_TYPES::NO_ERROR)};
            emit getMyChatsFinished(res, parsedArrayObjects);
            reply->deleteLater();
            return;
        }
        NetworkResult res{false, ERROR_TYPES::UNKNOWN_ERROR, messageForError(ERROR_TYPES::UNKNOWN_ERROR)};
        emit getMyChatsFinished(res);
        reply->deleteLater();
    });

}

const std::vector<ParsedChatsListArrayObject> ChatService::parseChatsListArray(const QJsonDocument &doc)
{
    std::vector<ParsedChatsListArrayObject> parsedArrayObjects;  //!< Объекты разобранного JSON-массива

    // Безопасное преобразование JSON-числа к unsigned long long.
    auto toUnsignedLongLong = [](const QJsonValue &value, unsigned long long defaultValue = ULONG_LONG_MAX) -> unsigned long long
    {
        if (!value.isDouble())
            return defaultValue;

        const double number = value.toDouble();
        if (number < 0 || std::floor(number) != number)
            return defaultValue;

        return static_cast<unsigned long long>(number);
    };

    const QJsonArray chats = doc.array();
    for (const QJsonValue &chatValue : chats)
    {
        if (!chatValue.isObject())
            continue;

        const QJsonObject chatObject = chatValue.toObject();

        ParsedChatsListArrayObject paObj;
        paObj.chatId = toUnsignedLongLong(chatObject.value("chat_id"));
        paObj.chatName = chatObject.value("chat_name").toString();
        paObj.chatAvatarFileId = toUnsignedLongLong(chatObject.value("chat_avatar_file_id"));
        paObj.type = chatObject.value("type").toString();

        const QJsonValue interlocutorValue = chatObject.value("interlocutor");
        if (interlocutorValue.isObject())
        {
            // Для group-чата user может отсутствовать, поэтому блок опциональный.
            const QJsonObject interlocutorObj = interlocutorValue.toObject();
            paObj.userId = toUnsignedLongLong(interlocutorObj.value("user_id"));
            paObj.username = interlocutorObj.value("username").toString();
            paObj.userAvatarFileId = toUnsignedLongLong(interlocutorObj.value("avatar_file_id"));
        }

        const QString normalizedChatName = paObj.chatName.trimmed();
        if (paObj.type.compare("private", Qt::CaseInsensitive) == 0
            && (normalizedChatName.isEmpty() || normalizedChatName.compare("none", Qt::CaseInsensitive) == 0)
            && !paObj.username.trimmed().isEmpty())
        {
            // Для private-диалогов используем username как отображаемое имя чата.
            paObj.chatName = paObj.username;
        }

        const QJsonValue lastMessageValue = chatObject.value("last_message");
        if (lastMessageValue.isObject())
        {
            // last_message опционален: заполняем только если объект присутствует.
            const QJsonObject lastMessageObject = lastMessageValue.toObject();
            paObj.lastMessageId = toUnsignedLongLong(lastMessageObject.value("message_id"));
            paObj.lastMessageSenderId = toUnsignedLongLong(lastMessageObject.value("sender_id"));
            paObj.lastMessageChatId = toUnsignedLongLong(lastMessageObject.value("chat_id"));
            paObj.lastMessage = lastMessageObject.value("message").toString();
            paObj.lastMessageFileId = toUnsignedLongLong(lastMessageObject.value("file_id"));
            paObj.lastMessageTimestamp = lastMessageObject.value("timestamp").toString();
            paObj.lastMessageRead = lastMessageObject.value("read").toBool(false);
            paObj.lastMessageReadAt = lastMessageObject.value("read_at").toString();
            paObj.lastMessageEdited = lastMessageObject.value("edited").toBool(false);
            paObj.lastMessageEditedAt = lastMessageObject.value("edited_at").toString();
        }

        parsedArrayObjects.push_back(paObj);
    }
    return parsedArrayObjects;
}

void ChatService::getChatMessages(const unsigned long long &chatId, const QString &accToken)
{
    emit getChatMessagesInProgress();
    QString currentChatMessagesUrl = QString(chatMessagesUrl).arg(chatId);
    QUrl url(baseUrl + currentChatMessagesUrl);
    QNetworkRequest req(url);
    // Передаем токен в заголовке
    req.setRawHeader("Authorization", "Bearer " + accToken.toUtf8());

    QNetworkReply * reply = network->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, chatId, reply](){
        auto httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (reply->error() != QNetworkReply::NoError && httpCode == 0)
        {
            NetworkResult res{false, ERROR_TYPES::UNKNOWN_ERROR, messageForError(ERROR_TYPES::UNKNOWN_ERROR)};
            emit getChatMessagesFinished(res);
            reply->deleteLater();
            return;
        }
        QByteArray raw = reply->readAll();
        QJsonParseError pe;
        QJsonDocument doc = QJsonDocument::fromJson(raw, &pe);
        if (pe.error || !doc.isArray())
        {
            NetworkResult res{false, ERROR_TYPES::UNKNOWN_ERROR, messageForError(ERROR_TYPES::UNKNOWN_ERROR)};
            emit getChatMessagesFinished(res);
            reply->deleteLater();
            return;
        }
        if (httpCode == 200)
        {
            const auto parsedArrayObjects = parseChatMessagesArray(doc);
            NetworkResult res{true, ERROR_TYPES::NO_ERROR, messageForError(ERROR_TYPES::NO_ERROR)};
            emit getChatMessagesFinished(res, chatId, parsedArrayObjects);
            reply->deleteLater();
            return;
        }
        NetworkResult res{false, ERROR_TYPES::UNKNOWN_ERROR, messageForError(ERROR_TYPES::UNKNOWN_ERROR)};
        emit getChatMessagesFinished(res);
        reply->deleteLater();
    });
}

const std::vector<ParsedChatMessagesArrayObject> ChatService::parseChatMessagesArray(const QJsonDocument &doc)
{
    std::vector<ParsedChatMessagesArrayObject> parsedArrayObjects;

    // Безопасное преобразование JSON-числа к unsigned long long.
    auto toUnsignedLongLong = [](const QJsonValue &value, unsigned long long defaultValue = ULONG_LONG_MAX) -> unsigned long long
    {
        if (!value.isDouble())
            return defaultValue;

        const double number = value.toDouble();
        if (number < 0 || std::floor(number) != number)
            return defaultValue;

        return static_cast<unsigned long long>(number);
    };

    const QJsonArray messages = doc.array();
    for (const QJsonValue &messageValue : messages)
    {
        if (!messageValue.isObject())
            continue;

        const QJsonObject messageObject = messageValue.toObject();

        ParsedChatMessagesArrayObject paObj;
        paObj.messageId = toUnsignedLongLong(messageObject.value("message_id"));
        paObj.senderId = toUnsignedLongLong(messageObject.value("sender_id"));
        paObj.chatId = toUnsignedLongLong(messageObject.value("chat_id"));
        paObj.message = messageObject.value("message").toString();
        paObj.timestamp = messageObject.value("timestamp").toString();
        paObj.read = messageObject.value("read").toBool(false);
        paObj.readAt = messageObject.value("read_at").toString();
        paObj.edited = messageObject.value("edited").toBool(false);
        paObj.editedAt = messageObject.value("edited_at").toString();

        parsedArrayObjects.push_back(paObj);
    }
    // reverse т.к API-шка возвращает массив "новые -> старые" а мне для отрисовки "сверху - вниз"
    // нужно чтобы сначала были старые сообщения а в конце новые
    std::reverse(parsedArrayObjects.begin(), parsedArrayObjects.end());
    return parsedArrayObjects;
}

void ChatService::createDirectChat(const unsigned long long &userId, const QString &accToken)
{
    emit createDirectChatInProgress();
    QUrl url(baseUrl + createDirectChatUrl);
    QUrlQuery query;
    query.addQueryItem("user_id", QString::number(userId));
    url.setQuery(query);

    QNetworkRequest req(url);
    req.setRawHeader("Authorization", "Bearer " + accToken.toUtf8());

    QNetworkReply * reply = network->post(req, QByteArray("{}"));
    connect(reply, &QNetworkReply::finished, this, [this, reply](){
        auto httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        // TODO: обработка ошибок httpCode
        if (reply->error() != QNetworkReply::NoError && httpCode == 0)
        {
            // TODO: Определение конкретной ошибки
            NetworkResult res{false, ERROR_TYPES::UNKNOWN_ERROR, messageForError(ERROR_TYPES::UNKNOWN_ERROR)};
            emit createDirectChatFinished(res);
            reply->deleteLater();
            return;
        }
        QByteArray raw = reply->readAll();
        QJsonParseError pe;
        QJsonDocument doc = QJsonDocument::fromJson(raw, &pe);
        if (pe.error || !doc.isObject())
        {
            // TODO: Определение конкретной ошибки
            NetworkResult res{false, ERROR_TYPES::UNKNOWN_ERROR, messageForError(ERROR_TYPES::UNKNOWN_ERROR)};
            emit createDirectChatFinished(res);
            reply->deleteLater();
            return;
        }
        if (httpCode == 200 || httpCode == 201)
        {

            NetworkResult res{true, ERROR_TYPES::NO_ERROR, messageForError(ERROR_TYPES::NO_ERROR)};
            emit createDirectChatFinished(res);
            reply->deleteLater();
            return;
        }
        NetworkResult res{false, ERROR_TYPES::UNKNOWN_ERROR, messageForError(ERROR_TYPES::UNKNOWN_ERROR)};
#ifdef QT_DEBUG
        qDebug() << doc;
#endif
        emit createDirectChatFinished(res);
        reply->deleteLater();
        return;
    } );
}

