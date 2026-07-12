#include "services/websocketservice.h"

#include <cmath>
#include <QString>

WebsocketService::WebsocketService(QObject *parent)
    : QObject{parent}
    , websocket(new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this))
    , baseUrl(baseWebsocketUrl)
    , webSocketUrl("/ws/")
    , ackFlushTimer(new QTimer(this))
    , outgoingMessagesFlushTimer(new QTimer(this))
    , pendingDeliveryIds()
    , pendingOutgoingMessages()
    , ackFlushIntervalMs(5000)
    , outgoingMessagesFlushIntervalMs(5000)
    , handlersMapByTypeOfMessage()

{
    connect(websocket, &QWebSocket::connected, this, [this]()
    {
        emit socketConnectionFinished(NetworkResult{true, ERROR_TYPES::NO_ERROR, generateMessageForError(ERROR_TYPES::NO_ERROR)});
    });

    connect(websocket, &QWebSocket::disconnected, this, [this]()
    {
        emit socketDisonnectionFinished(NetworkResult{true, ERROR_TYPES::NO_ERROR, generateMessageForError(ERROR_TYPES::NO_ERROR)});
#ifdef QT_DEBUG
        qDebug() << "websocket disconnected!!!";
#endif
    });

    //TODO: может для ошибки тоже замутить отдельную функцию а не лямбду
    connect(websocket, &QWebSocket::errorOccurred, this, [this](QAbstractSocket::SocketError error)
    {
        const int errorCode = static_cast<int>(error);
        const QString message = QString("WebSocket error [%1]: %2").arg(errorCode).arg(websocket->errorString());
        emit socketConnectionFinished(NetworkResult{false, ERROR_TYPES::UNKNOWN_ERROR, message});
    });

    connect(websocket, &QWebSocket::textMessageReceived, this, &WebsocketService::on_textMessageReceived);
    ackFlushTimer->setInterval(ackFlushIntervalMs);
    outgoingMessagesFlushTimer->setInterval(outgoingMessagesFlushIntervalMs);
    connect(ackFlushTimer, &QTimer::timeout, this, &WebsocketService::flushPendingAcks);
    connect(outgoingMessagesFlushTimer, &QTimer::timeout, this, &WebsocketService::flushPendingOutgoingMessages);
    //TODO: тут можно походу поставить singleshot и при каждой отправке запускать таймер хотя как будто хуйня
    outgoingMessagesFlushTimer->start();
    ackFlushTimer->start();
    fillHandlersMap();
}
void WebsocketService::connectSocket(const QString &accessToken)
{
    emit socketConnectionInProgress();

    QUrl url(baseUrl + webSocketUrl);
    QNetworkRequest req(url);
    req.setRawHeader("Authorization", "Bearer " + accessToken.toUtf8());

    websocket->open(req);
}

void WebsocketService::disconnectSocket()
{
    emit socketDisonnectionInProgress();
    websocket->close();
}

void WebsocketService::sendMessage(const ParsedChatMessagesArrayObject &message)
{
    emit sendingMessageInProgress();
    if (message.clientMessageId.isEmpty())
    {
        emit sendingMessageFinished(NetworkResult{false, ERROR_TYPES::UNKNOWN_ERROR, "client_message_id is empty"});
#ifdef QT_DEBUG
        qDebug() << "client_message_id is empty!!!";
#endif
        return;
    }

    // Приходитсья кастить к signed так как jsonvalue не принимает ULL тип
    qint64 chatIdCasted = message.chatId;
    QJsonObject payload{
        {"chat_id", chatIdCasted},
        {"message", message.message},
        {"client_message_id", message.clientMessageId}
    };
    if (!message.attachments.isEmpty())
    {
        QJsonArray attachmentIds;
        for (const QJsonValue &value : std::as_const(message.attachments))
        {
            if (value.isObject())
            {
                const QJsonObject obj = value.toObject();
                if (!obj.value("file_id").isUndefined())
                    attachmentIds.append(obj.value("file_id").toVariant().toLongLong());
            }
            else if (value.isDouble())
            {
                attachmentIds.append(value.toVariant().toLongLong());
            }
        }
        if (!attachmentIds.isEmpty())
            payload.insert("attachments_ids", attachmentIds);
    }
    QJsonObject obj{
        {"type", "chat.message.send"},
        {"payload", payload}
    };

    pendingOutgoingMessages.insert(message.clientMessageId, obj);
    if (!websocket->isValid())
    {
        emit sendingMessageFinished(NetworkResult{false, ERROR_TYPES::UNKNOWN_ERROR, "websocket is invalid"});
#ifdef QT_DEBUG
        qDebug() << "websocket invalid, message queued, pending count:" << pendingOutgoingMessages.size();
#endif
        return;
    }

    const QString jsonText = QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact));
    const qint64 bytesSent = websocket->sendTextMessage(jsonText);
    if (bytesSent > 0)
    {
        emit sendingMessageFinished(NetworkResult{true, ERROR_TYPES::NO_ERROR, generateMessageForError(ERROR_TYPES::NO_ERROR)});
#ifdef QT_DEBUG
        qDebug() << "websocket message sent and queued, pending count:" << pendingOutgoingMessages.size();
#endif
        return;
    }

    emit sendingMessageFinished(NetworkResult{false, ERROR_TYPES::UNKNOWN_ERROR, "websocket send failed, message queued"});
#ifdef QT_DEBUG
    qDebug() << "websocket send failed, message queued, pending count:" << pendingOutgoingMessages.size();
#endif
}

void WebsocketService::fillHandlersMap()
{
    handlersMapByTypeOfMessage.insert(QString("pong"), &WebsocketService::on_pong);
    handlersMapByTypeOfMessage.insert(QString("chat.message"), &WebsocketService::on_newMessage);
    handlersMapByTypeOfMessage.insert(QString("chat.message.send.accepted"), &WebsocketService::on_messageAccepted);
    handlersMapByTypeOfMessage.insert(QString("chat.message.ack.result"), &WebsocketService::on_ackResult);
    handlersMapByTypeOfMessage.insert(QString("chat.marked_read"), &WebsocketService::on_markedRead);
    handlersMapByTypeOfMessage.insert(QString("message.edited"), &WebsocketService::on_messageEdited);
    handlersMapByTypeOfMessage.insert(QString("message.deleted"), &WebsocketService::on_messageDeleted);
    handlersMapByTypeOfMessage.insert(QString("user.status"), &WebsocketService::on_userStatus);
    handlersMapByTypeOfMessage.insert(QString("error"), &WebsocketService::on_error);

}

void WebsocketService::on_pong(const QJsonObject &payload)
{

}

void WebsocketService::on_newMessage(const QJsonObject &payload)
{
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

    const unsigned long long deliveryId = toUnsignedLongLong(payload.value("delivery_id"));
    QJsonValue messageValue = payload.value("message");
    if (!messageValue.isObject() || deliveryId == 0)
    {
        //TODO: реализация
        return;
    }
    const QJsonObject messageObject = messageValue.toObject();

    ParsedChatMessagesArrayObject newMessage;
    newMessage.messageId = toUnsignedLongLong(messageObject.value("message_id"));
    newMessage.senderId = toUnsignedLongLong(messageObject.value("sender_id"));
    newMessage.chatId = toUnsignedLongLong(messageObject.value("chat_id"));
    newMessage.message = messageObject.value("message").toString();
    newMessage.timestamp = messageObject.value("timestamp").toString();
    newMessage.read = messageObject.value("read").toBool(false);
    newMessage.readAt = messageObject.value("read_at").toString();
    newMessage.edited = messageObject.value("edited").toBool(false);
    newMessage.editedAt = messageObject.value("edited_at").toString();
    if (messageObject.value("attachments").isArray())
    {
        newMessage.attachments = messageObject.value("attachments").toArray();
        newMessage.attachmentsCount = static_cast<unsigned int>(newMessage.attachments.size());
        newMessage.hasAttachments = !newMessage.attachments.isEmpty();
    }

    pendingDeliveryIds.insert(deliveryId);
    emit newMessageRecieved(newMessage);

}

void WebsocketService::flushPendingAcks()
{
    if (pendingDeliveryIds.isEmpty())
    {
        return;
    }

    if (!websocket->isValid())
    {
        return;
    }

    QJsonArray deliveryIdsJson;
    for (unsigned long long deliveryId : std::as_const(pendingDeliveryIds))
    {
        deliveryIdsJson.append(static_cast<qint64>(deliveryId));
    }

    QJsonObject payload{
        {"delivery_ids", deliveryIdsJson}
    };
    QJsonObject obj{
        {"type", "chat.message.ack"},
        {"payload", payload}
    };

    const QString jsonText = QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact));
    websocket->sendTextMessage(jsonText);
}

void WebsocketService::flushPendingOutgoingMessages()
{
    if (pendingOutgoingMessages.isEmpty())
    {
        return;
    }

    if (!websocket->isValid())
    {
        return;
    }

    for (auto it = pendingOutgoingMessages.cbegin(); it != pendingOutgoingMessages.cend(); ++it)
    {
        const QString jsonText = QString::fromUtf8(QJsonDocument(it.value()).toJson(QJsonDocument::Compact));
        websocket->sendTextMessage(jsonText);
    }
}

void WebsocketService::on_messageAccepted(const QJsonObject &payload)
{
    auto toUnsignedLongLong = [](const QJsonValue &value, unsigned long long defaultValue = 0ULL) -> unsigned long long
    {
        if (!value.isDouble())
            return defaultValue;

        const double number = value.toDouble();
        if (number < 0 || std::floor(number) != number)
            return defaultValue;

        return static_cast<unsigned long long>(number);
    };

    const QJsonValue clientMessageIdValue = payload.value("client_message_id");
    const QJsonValue messageIdValue = payload.value("message_id");
    const QJsonValue timestampValue = payload.value("timestamp");
    const QJsonValue dedupedValue = payload.value("deduped");
    const QJsonValue chatIdValue = payload.value("chat_id");
    if (!clientMessageIdValue.isString() || !messageIdValue.isDouble() || !timestampValue.isString() || !chatIdValue.isDouble())
    {
        // TODO: обработка некорректного payload
        return;
    }

    const QString clientMessageId = clientMessageIdValue.toString();
    const unsigned long long messageId = toUnsignedLongLong(messageIdValue);
    const unsigned long long chatId = toUnsignedLongLong(chatIdValue);
    const QString timestamp = timestampValue.toString();
    const bool deduped = dedupedValue.toBool(false);

    if (clientMessageId.isEmpty() || messageId == 0 || timestamp.isEmpty() || chatId == 0)
    {
        // TODO: обработка некорректного payload
        return;
    }

    ParsedMessageAcceptedObject paObj;
    paObj.clientMessageId = clientMessageId;
    paObj.messageId = messageId;
    paObj.deduped = deduped;
    paObj.timestamp = timestamp;
    paObj.chatId = chatId;

    pendingOutgoingMessages.remove(clientMessageId);
    emit messageAccepted(paObj);
}

void WebsocketService::on_ackResult(const QJsonObject &payload)
{
    auto toUnsignedLongLong = [](const QJsonValue &value, unsigned long long defaultValue = 0ULL) -> unsigned long long
    {
        if (!value.isDouble())
            return defaultValue;

        const double number = value.toDouble();
        if (number < 0 || std::floor(number) != number)
            return defaultValue;

        return static_cast<unsigned long long>(number);
    };
    QJsonValue ackedCountValue = payload.value("acked_count");
    QJsonValue deliveryIdsValue = payload.value("delivery_ids");
    if (!ackedCountValue.isDouble() || !deliveryIdsValue.isArray())
    {
        return;
    }
    unsigned long long ackedCount = toUnsignedLongLong(ackedCountValue);
    QJsonArray deliveryIds = deliveryIdsValue.toArray();
    if (ackedCount == 0 || deliveryIds.size() == 0)
    {
        return;
    }
    for (const QJsonValue &deliveryIdValue : std::as_const(deliveryIds))
    {
        if(!deliveryIdValue.isDouble())
            continue;
        unsigned long long deliveryId = toUnsignedLongLong(deliveryIdValue);
        pendingDeliveryIds.remove(deliveryId);
    }
}

void WebsocketService::on_markedRead(const QJsonObject &payload)
{
    const quint64 userId = static_cast<quint64>(payload.value("user_id").toInteger(-1));
    const quint64 chatId = static_cast<quint64>(payload.value("chat_id").toInteger(-1));
    const quint64 lastReadMessageId = static_cast<quint64>(payload.value("last_read_message_id").toInteger(-1));
    if (userId == ULONG_LONG_MAX || chatId == ULONG_LONG_MAX || lastReadMessageId == ULONG_LONG_MAX)
        return;

    emit messageMarkedRead(userId, chatId, lastReadMessageId);

}

void WebsocketService::on_userStatus(const QJsonObject &payload)
{

}

void WebsocketService::on_error(const QJsonObject &payload)
{

}

void WebsocketService::on_messageEdited(const QJsonObject &payload)
{

}

void WebsocketService::on_messageDeleted(const QJsonObject &payload)
{

}

void WebsocketService::callHandler(const QString &type, const QJsonObject &payload)
{
    const auto it = handlersMapByTypeOfMessage.constFind(type);
    if (it == handlersMapByTypeOfMessage.cend())
    {
        return;
    }

    auto func = it.value();
    (this->*func)(payload);

}

void WebsocketService::on_textMessageReceived(const QString &message)
{
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8(), &parseError);

    if (parseError.error != QJsonParseError::NoError || !doc.isObject())
    {
        const QString err = QString("Invalid JSON [%1]: %2")
        .arg(static_cast<int>(parseError.error))
            .arg(parseError.errorString());
        //TODO: добавить обработку
        return;
    }

    const QJsonObject obj = doc.object();
    QJsonValue messageTypeValue = obj.value("type");
    if (!messageTypeValue.isString())
    {
        //TODO: тоже обработку
        return;
    }
    QString messageTypeString = messageTypeValue.toString();
    QJsonValue payloadValue = obj.value("payload");
    if (!payloadValue.isObject())
    {
        //TODO: тоже обработку
        return;
    }
    QJsonObject payloadObj = payloadValue.toObject();
    callHandler(messageTypeString, payloadObj);
}

