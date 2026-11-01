//
// Created by mark on 10/12/26.
//

#include "parsed_objects.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

std::vector<Attachment> parseAttachments(const QJsonArray &arr)
{
    std::vector<Attachment> attachments;
    attachments.reserve(arr.size());

    for (const auto &arrValue : arr)
    {
        if (!arrValue.isObject())
            continue;
        const QJsonObject attachmentObject = arrValue.toObject();
        Attachment attachment;
        attachment.fileId       = ConvertToUnsignedLongLong(attachmentObject.value("file_id"));
        attachment.fileName     = attachmentObject.value("filename").toString();
        attachment.fileSize     = ConvertToUnsignedLongLong(attachmentObject.value("file_size"), 0);
        attachment.contentType  = attachmentObject.value("content_type").toString();
        attachment.category     = attachmentObject.value("category").toString();
        attachment.duration     = ConvertToUnsignedLongLong(attachmentObject.value("duration"), 0);
        attachment.blurhash     = attachmentObject.value("blurhash").toString();
        attachment.downloadUrl  = attachmentObject.value("download_url").toString();
        attachment.thumbnailUrl = attachmentObject.value("thumbnail_url").toString();
        if (attachmentObject.value("waveform").isArray())
        {
            QJsonArray waveformArray = attachmentObject.value("waveform").toArray();
            attachment.waveform.reserve(waveformArray.size());
            for (const auto &waveFormValue : waveformArray)
            {
                if (!waveFormValue.isDouble())
                    continue;
                attachment.waveform.push_back(waveFormValue.toInt());
            }
        }
        attachments.push_back(attachment);
    }
    return attachments;
}

std::vector<ParsedChatsListArrayObject> parseChatsListArray(const QJsonDocument & doc)
{
    const QJsonArray chats = doc.array();
    std::vector<ParsedChatsListArrayObject> parsedArrayObjects;  //!< Объекты разобранного JSON-массива
    parsedArrayObjects.reserve(chats.size());

    for (const auto & chatValue : chats)
    {
        if (!chatValue.isObject())
            continue;

        const QJsonObject chatObject = chatValue.toObject();

        ParsedChatsListArrayObject paObj;
        paObj.chatId = ConvertToUnsignedLongLong(chatObject.value("chat_id"));
        paObj.chatName = chatObject.value("chat_name").toString();
        paObj.chatAvatarFileId = ConvertToUnsignedLongLong(chatObject.value("chat_avatar_file_id"));
        paObj.type = chatObject.value("type").toString();
        paObj.unreadCount = ConvertToUnsignedLongLong(chatObject.value("unread_count"), 0);

        const QJsonValue interlocutorValue = chatObject.value("interlocutor");
        // Для group-чата user может отсутствовать, поэтому блок опциональный.
        if (interlocutorValue.isObject())
        {
            const QJsonObject interlocutorObj = interlocutorValue.toObject();
            paObj.interlocutor.userId = ConvertToUnsignedLongLong(interlocutorObj.value("user_id"));
            paObj.interlocutor.username = interlocutorObj.value("username").toString();
            paObj.interlocutor.nickname = interlocutorObj.value("nickname").toString();
            paObj.interlocutor.userAvatarFileUrl = interlocutorObj.value("avatar_file_url").toString();
        }

        const QString & chatName = paObj.chatName;
        if (paObj.type.compare("private", Qt::CaseInsensitive) == 0
            && (chatName.isEmpty() || chatName.compare("none", Qt::CaseInsensitive) == 0))
        {
            const QString & nickname = paObj.interlocutor.nickname;
            const QString & username = paObj.interlocutor.username;
            // Для private-диалогов используем nickname/username как отображаемое имя чата.
            if (!nickname.isEmpty())
                paObj.chatName = nickname;
            else if (!username.isEmpty())
                paObj.chatName = username;
        }

        const QJsonValue lastMessageValue = chatObject.value("last_message");
        if (lastMessageValue.isObject())
        {
            // last_message опционален: заполняем только если объект присутствует.
            const QJsonObject lastMessageObject = lastMessageValue.toObject();
            paObj.lastMessage.messageId = ConvertToUnsignedLongLong(lastMessageObject.value("message_id"));
            paObj.lastMessage.message = lastMessageObject.value("message").toString();
            paObj.lastMessage.senderId = ConvertToUnsignedLongLong(lastMessageObject.value("sender_id"));
            paObj.lastMessage.isRead = lastMessageObject.value("read").toBool(false);
            paObj.lastMessage.timestamp = lastMessageObject.value("timestamp").toString();
            paObj.lastMessage.hasAttachments = lastMessageObject.value("has_attachments").toBool(false);
            paObj.lastMessage.category = lastMessageObject.value("category").toString();
            paObj.lastMessage.attachmentsCount = ConvertToUnsignedLongLong(lastMessageObject.value("attachments_count"), 0);
        }


        parsedArrayObjects.push_back(paObj);
    }
    return parsedArrayObjects;
}

std::vector<ParsedChatMessagesArrayObject> parseChatMessagesArray(const QJsonDocument &doc)
{
    const QJsonArray messages = doc.array();
    std::vector<ParsedChatMessagesArrayObject> parsedArrayObjects;
    parsedArrayObjects.reserve(messages.size());

    for (const auto &messageValue : messages)
    {
        if (!messageValue.isObject())
            continue;

        const QJsonObject messageObject = messageValue.toObject();

        ParsedChatMessagesArrayObject paObj;
        paObj.messageId = ConvertToUnsignedLongLong(messageObject.value("message_id"));
        paObj.senderId = ConvertToUnsignedLongLong(messageObject.value("sender_id"));
        paObj.chatId = ConvertToUnsignedLongLong(messageObject.value("chat_id"));
        paObj.message = messageObject.value("message").toString();
        paObj.timestamp = messageObject.value("timestamp").toString();
        paObj.isRead = messageObject.value("read").toBool(false);
        paObj.readAt = messageObject.value("read_at").toString();
        paObj.isEdited = messageObject.value("edited").toBool(false);
        paObj.editedAt = messageObject.value("edited_at").toString();
        if (messageObject.value("attachments").isArray())
        {
            paObj.attachments = parseAttachments(messageObject.value("attachments").toArray());
            paObj.attachmentsCount = static_cast<unsigned int>(paObj.attachments.size());
            paObj.hasAttachments = !paObj.attachments.empty();
        }

        parsedArrayObjects.push_back(paObj);
    }
    // reverse т.к API-шка возвращает массив "новые -> старые" а для отрисовки "сверху - вниз"
    // нужно чтобы сначала были старые сообщения а в конце новые
    std::ranges::reverse(parsedArrayObjects);
    return parsedArrayObjects;
}

ParsedChatMessagesArrayObject parseChatMessageObject(const QJsonObject& messageObject)
{
    ParsedChatMessagesArrayObject newMessage;
    newMessage.messageId = ConvertToUnsignedLongLong(messageObject.value("message_id"), 0);
    newMessage.senderId  = ConvertToUnsignedLongLong(messageObject.value("sender_id"), 0);
    newMessage.chatId    = ConvertToUnsignedLongLong(messageObject.value("chat_id"), 0);
    newMessage.message   = messageObject.value("message").toString();
    newMessage.timestamp = messageObject.value("timestamp").toString();
    newMessage.isRead      = messageObject.value("read").toBool(false);
    newMessage.readAt    = messageObject.value("read_at").toString();
    newMessage.isEdited    = messageObject.value("edited").toBool(false);
    newMessage.editedAt  = messageObject.value("edited_at").toString();
    if (messageObject.value("attachments").isArray())
    {
        newMessage.attachments = parseAttachments(messageObject.value("attachments").toArray());
        newMessage.attachmentsCount = newMessage.attachments.size();
        newMessage.hasAttachments = !newMessage.attachments.empty();
    }
    return newMessage;
}

ParsedDownloadedFileInfo parseDownloadedFileInfo(const QJsonDocument &doc)
{
    //TODO: как будто можно какую то красивую тему сделать с наследованием от ParsedUploadedFileInfo, чтобы не дублировать код
    const QJsonObject obj = doc.object();
    ParsedDownloadedFileInfo fInfo;
    fInfo.fileId = ConvertToUnsignedLongLong(obj.value("file_id"));
    fInfo.fileSize = ConvertToUnsignedLongLong(obj.value("file_size"));
    fInfo.duration = ConvertToUnsignedLongLong(obj.value("duration"), 0);
    fInfo.fileName = obj.value("filename").toString();
    fInfo.contentType = obj.value("content_type").toString();
    fInfo.uploadedAt = obj.value("uploaded_at").toString();
    fInfo.category = obj.value("category").toString();
    fInfo.blurhash = obj.value("blurhash").toString();
    if (obj.value("waveform").isArray())
    {
        const QJsonArray waveformArray = obj.value("waveform").toArray();
        fInfo.waveform.reserve(waveformArray.size());
        for (const auto & waveFormValue : waveformArray)
        {
            if (!waveFormValue.isDouble())
                continue;
            fInfo.waveform.push_back(waveFormValue.toInt());
        }
    }
    fInfo.downloadFileUrl = obj.value("download_url").toString();
    fInfo.thumbnailUrl = obj.value("thumbnail_url").toString();
    return fInfo;
}

ParsedUploadedFileInfo parseUploadedFileInfo(const QJsonDocument &doc)
{
    const QJsonObject obj = doc.object();
    ParsedUploadedFileInfo fInfo;
    fInfo.fileId = ConvertToUnsignedLongLong(obj.value("file_id"));
    fInfo.fileSize = ConvertToUnsignedLongLong(obj.value("file_size"));
    fInfo.duration = ConvertToUnsignedLongLong(obj.value("duration"), 0);
    fInfo.fileName = obj.value("filename").toString();
    fInfo.contentType = obj.value("content_type").toString();
    fInfo.uploadedAt = obj.value("uploaded_at").toString();
    fInfo.category = obj.value("category").toString();
    fInfo.blurhash = obj.value("blurhash").toString();
    if (obj.value("waveform").isArray())
    {
        const QJsonArray waveformArray = obj.value("waveform").toArray();
        fInfo.waveform.reserve(waveformArray.size());
        for (const auto & waveFormValue : waveformArray)
        {
            if (!waveFormValue.isDouble())
                continue;
            fInfo.waveform.push_back(waveFormValue.toInt());
        }
    }
    return fInfo;
}

std::vector<ParsedFoundUsersObject> parseFoundUsersArray(const QJsonDocument &doc)
{
    const QJsonArray users = doc.array();
    std::vector<ParsedFoundUsersObject> parsedArrayObjects;
    parsedArrayObjects.reserve(users.size());

    for (const auto & userValue : users)
    {
        if (!userValue.isObject())
            continue;

        const QJsonObject userObject = userValue.toObject();

        ParsedFoundUsersObject paObj;
        paObj.username = userObject.value("username").toString();
        paObj.nickname = userObject.value("nickname").toString();
        paObj.userId = ConvertToUnsignedLongLong(userObject.value("user_id"));
        paObj.lastSeen = userObject.value("last_seen").toString();
        paObj.avatarFileUrl = userObject.value("avatar_file_url").toString();

        parsedArrayObjects.push_back(paObj);
    }

    return parsedArrayObjects;
}

quint64 ConvertToUnsignedLongLong(const QJsonValue &value, unsigned long long defaultValue)
{
    if (!value.isDouble())
        return defaultValue;

    const double number = value.toDouble();
    if (number < 0 || std::floor(number) != number)
        return defaultValue;

    return static_cast<quint64>(number);
}

std::optional<ParsedMessageAcceptedObject> parseMessageAcceptedObject(const QJsonObject& payload)
{
    const QJsonValue clientMessageIdValue = payload.value("client_message_id");
    const QJsonValue messageIdValue = payload.value("message_id");
    const QJsonValue timestampValue = payload.value("timestamp");
    const QJsonValue dedupedValue = payload.value("deduped");
    const QJsonValue chatIdValue = payload.value("chat_id");
    if (!clientMessageIdValue.isString() || !messageIdValue.isDouble() || !timestampValue.isString() || !chatIdValue.isDouble())
        return std::nullopt;

    const QString clientMessageId = clientMessageIdValue.toString();
    const unsigned long long messageId = ConvertToUnsignedLongLong(messageIdValue, 0);
    const unsigned long long chatId = ConvertToUnsignedLongLong(chatIdValue, 0);
    const QString timestamp = timestampValue.toString();
    const bool deduped = dedupedValue.toBool(false);

    ParsedMessageAcceptedObject paObj;
    paObj.clientMessageId = clientMessageId;
    paObj.messageId = messageId;
    paObj.deduped = deduped;
    paObj.timestamp = timestamp;
    paObj.chatId = chatId;

    return paObj;
}
