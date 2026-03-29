#include "chatservice.h"
#include <cmath>
#include <algorithm>

ChatService::ChatService(QObject *parent)
    : QObject{parent}
    , network(new QNetworkAccessManager(this))
    , baseUrl("https://messenger-3yfw.onrender.com")
    , myChatsUrl("/api/chats/")
    , chatMessagesUrl("/api/chats/%1")
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
            AuthResult res{false, ERROR_TYPES::UNKNOWN_ERROR, messageForError(ERROR_TYPES::UNKNOWN_ERROR)};
            emit getMyChatsFinished(res);
            reply->deleteLater();
            return;
        }
        QByteArray raw = reply->readAll();
        QJsonParseError pe;
        QJsonDocument doc = QJsonDocument::fromJson(raw, &pe);
        if (pe.error || !doc.isArray())
        {
            AuthResult res{false, ERROR_TYPES::UNKNOWN_ERROR, messageForError(ERROR_TYPES::UNKNOWN_ERROR)};
            emit getMyChatsFinished(res);
            reply->deleteLater();
            return;
        }
        if (httpCode == 200)
        {
            const auto parsedArrayObjects = parseChatsListArray(doc);
            AuthResult res{true, ERROR_TYPES::NO_ERROR, messageForError(ERROR_TYPES::NO_ERROR)};
            emit getMyChatsFinished(res, parsedArrayObjects);
            reply->deleteLater();
            return;
        }
        AuthResult res{false, ERROR_TYPES::UNKNOWN_ERROR, messageForError(ERROR_TYPES::UNKNOWN_ERROR)};
        emit getMyChatsFinished(res);
        reply->deleteLater();
    });

}

const std::vector<ParsedChatsListArrayObject> ChatService::parseChatsListArray(const QJsonDocument &doc)
{
    std::vector<ParsedChatsListArrayObject> parsedArrayObjects;  //!< Объекты разобранного JSON-массива

    // Безопасное преобразование JSON-числа к unsigned long long.
    auto toUnsignedLongLong = [](const QJsonValue &value, unsigned long long defaultValue = 0ULL) -> unsigned long long
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

        const QJsonValue userValue = chatObject.value("user");
        if (userValue.isObject())
        {
            // Для private-чата user может отсутствовать, поэтому блок опциональный.
            const QJsonObject userObject = userValue.toObject();
            paObj.userId = toUnsignedLongLong(userObject.value("user_id"));
            paObj.username = userObject.value("username").toString();
            paObj.userAvatarFileId = toUnsignedLongLong(userObject.value("avatar_file_id"));
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
            AuthResult res{false, ERROR_TYPES::UNKNOWN_ERROR, messageForError(ERROR_TYPES::UNKNOWN_ERROR)};
            emit getChatMessagesFinished(res);
            reply->deleteLater();
            return;
        }
        QByteArray raw = reply->readAll();
        QJsonParseError pe;
        QJsonDocument doc = QJsonDocument::fromJson(raw, &pe);
        if (pe.error || !doc.isArray())
        {
            AuthResult res{false, ERROR_TYPES::UNKNOWN_ERROR, messageForError(ERROR_TYPES::UNKNOWN_ERROR)};
            emit getChatMessagesFinished(res);
            reply->deleteLater();
            return;
        }
        if (httpCode == 200)
        {
            const auto parsedArrayObjects = parseChatMessagesArray(doc);
            AuthResult res{true, ERROR_TYPES::NO_ERROR, messageForError(ERROR_TYPES::NO_ERROR)};
            emit getChatMessagesFinished(res, chatId, parsedArrayObjects);
            reply->deleteLater();
            return;
        }
        AuthResult res{false, ERROR_TYPES::UNKNOWN_ERROR, messageForError(ERROR_TYPES::UNKNOWN_ERROR)};
        emit getChatMessagesFinished(res);
        reply->deleteLater();
    });
}

const std::vector<ParsedChatMessagesArrayObject> ChatService::parseChatMessagesArray(const QJsonDocument &doc)
{
    std::vector<ParsedChatMessagesArrayObject> parsedArrayObjects;

    // Безопасное преобразование JSON-числа к unsigned long long.
    auto toUnsignedLongLong = [](const QJsonValue &value, unsigned long long defaultValue = 0ULL) -> unsigned long long
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
        paObj.fileId = toUnsignedLongLong(messageObject.value("file_id"));
        paObj.timestamp = messageObject.value("timestamp").toString();
        paObj.read = messageObject.value("read").toBool(false);
        paObj.readAt = messageObject.value("read_at").toString();
        paObj.edited = messageObject.value("edited").toBool(false);
        paObj.editedAt = messageObject.value("edited_at").toString();

        parsedArrayObjects.push_back(paObj);
    }
    std::reverse(parsedArrayObjects.begin(), parsedArrayObjects.end());
    return parsedArrayObjects;
}