#include "services/websocketservice.h"
#include "utils/parsed_objects.h"

#include <cmath>
#include <QString>

WebsocketService::WebsocketService(QObject *parent)
    : QObject{parent}
    , websocket(new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this))
    , ackFlushTimer(new QTimer(this))
    , outgoingMessagesFlushTimer(new QTimer(this))
    , ackFlushIntervalMs(5000)
    , outgoingMessagesFlushIntervalMs(5000)
    , pendingDeliveryIds()
    , pendingOutgoingMessages()
    , handlersMapByTypeOfMessage()

{
    connect(websocket, &QWebSocket::connected, this, [this]()
    {
        emit socketConnectionFinished(NetworkResult{true, ERROR_TYPES::NO_ERROR, generateMessageForError(ERROR_TYPES::NO_ERROR)});
    });

    connect(websocket, &QWebSocket::disconnected, this, [this]()
    {
        int code = static_cast<int>(websocket->closeCode());
        if (code == 1000)
            emit socketDisonnectionFinished(NetworkResult{true, ERROR_TYPES::NO_ERROR, generateMessageForError(ERROR_TYPES::NO_ERROR)});
        else if (code == 1008)
        {
            emit socketConnectionFinished(NetworkResult{false, ERROR_TYPES::UNAUTHORIZED, generateMessageForError(ERROR_TYPES::UNAUTHORIZED)});
        }
        qDebug() << "websocket disconnected!!!";
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
    //TODO: тут можно походу поставить singleshot и при каждой отправке запускать таймер хотя как будто хуйня или если empty то стопать
    outgoingMessagesFlushTimer->start();
    ackFlushTimer->start();
    fillHandlersMap();
}
void WebsocketService::connectSocket(const QString &accessToken)
{
    emit socketConnectionInProgress();

    QUrl url(baseWebsocketUrl + webSocketUrl);
    QNetworkRequest req(url);
    req.setRawHeader("Authorization", "Bearer " + accessToken.toUtf8());

    websocket->open(req);
}

void WebsocketService::disconnectSocket()
{
    emit socketDisonnectionInProgress();
    websocket->close();
    emit socketDisonnectionFinished(NetworkResult{true, ERROR_TYPES::NO_ERROR, generateMessageForError(ERROR_TYPES::NO_ERROR)});
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
    qint64 chatIdCasted = static_cast<qint64>(message.chatId);
    QJsonObject payload{
        {"chat_id", chatIdCasted},
        {"message", message.message},
        {"client_message_id", message.clientMessageId}
    };
    if (!message.attachments.empty())
    {
        QJsonArray attachmentIds;
        for (const Attachment &attachment : std::as_const(message.attachments))
        {
            if (attachment.fileId != ULONG_LONG_MAX)
                attachmentIds.append(static_cast<qint64>(attachment.fileId)); // Дай бог чтобы не дошло до знакового бита а то пизда тогда будет
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
    handlersMapByTypeOfMessage.insert(QString("chat.message"), &WebsocketService::on_newMessage);
    handlersMapByTypeOfMessage.insert(QString("chat.message.send.accepted"), &WebsocketService::on_messageAccepted);
    handlersMapByTypeOfMessage.insert(QString("chat.message.ack.result"), &WebsocketService::on_ackResult);
    handlersMapByTypeOfMessage.insert(QString("chat.marked_read"), &WebsocketService::on_markedRead);
    handlersMapByTypeOfMessage.insert(QString("message.edited"), &WebsocketService::on_messageEdited);
    handlersMapByTypeOfMessage.insert(QString("message.deleted"), &WebsocketService::on_messageDeleted);
    handlersMapByTypeOfMessage.insert(QString("user.status"), &WebsocketService::on_userStatus);
    handlersMapByTypeOfMessage.insert(QString("error"), &WebsocketService::on_error);

}

void WebsocketService::on_newMessage(const QJsonObject &payload)
{
    const unsigned long long deliveryId = ConvertToUnsignedLongLong(payload.value("delivery_id"), 0);
    QJsonValue messageValue = payload.value("message");
    if (!messageValue.isObject() || deliveryId == 0)
        return;
    const QJsonObject messageObject = messageValue.toObject();

    ParsedChatMessagesArrayObject newMessage = parseChatMessageObject(messageObject);

    pendingDeliveryIds.insert(deliveryId);
    emit newMessageRecieved(newMessage);

}

void WebsocketService::flushPendingAcks()
{
    if (pendingDeliveryIds.isEmpty() || !websocket->isValid())
        return;

    QJsonArray deliveryIdsJson;
    for (const unsigned long long deliveryId : std::as_const(pendingDeliveryIds))
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
    if (pendingOutgoingMessages.isEmpty() || !websocket->isValid())
        return;

    for (auto it = pendingOutgoingMessages.cbegin(); it != pendingOutgoingMessages.cend(); ++it)
    {
        const QString jsonText = QString::fromUtf8(QJsonDocument(it.value()).toJson(QJsonDocument::Compact));
        websocket->sendTextMessage(jsonText);
    }
}

void WebsocketService::on_messageAccepted(const QJsonObject &payload)
{
    std::optional<ParsedMessageAcceptedObject> paObj = parseMessageAcceptedObject(payload);

    if (!paObj.has_value())
        return;

    pendingOutgoingMessages.remove(paObj->clientMessageId);
    emit messageAccepted(*paObj);
}

void WebsocketService::on_ackResult(const QJsonObject &payload)
{
    QJsonValue ackedCountValue = payload.value("acked_count");
    QJsonValue deliveryIdsValue = payload.value("delivery_ids");
    if (!ackedCountValue.isDouble() || !deliveryIdsValue.isArray())
    {
        return;
    }
    unsigned long long ackedCount = ConvertToUnsignedLongLong(ackedCountValue, 0);
    QJsonArray deliveryIds = deliveryIdsValue.toArray();
    if (ackedCount == 0 || deliveryIds.size() == 0)
    {
        return;
    }
    for (const QJsonValue &deliveryIdValue : std::as_const(deliveryIds))
    {
        if(!deliveryIdValue.isDouble())
            continue;
        unsigned long long deliveryId = ConvertToUnsignedLongLong(deliveryIdValue, 0);
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
    quint64 userId = static_cast<quint64>(payload.value("user_id").toInteger(-1));
    bool isOnline = payload.value("online").toBool(false);
    if (userId == ULONG_LONG_MAX)
        return;

    emit userStatus(userId, isOnline);
}

void WebsocketService::on_error(const QJsonObject &payload)
{

}

void WebsocketService::on_messageEdited(const QJsonObject &payload)
{
    QJsonValue editedMessageVal = payload.value("edited_message");
    if (!editedMessageVal.isObject())
        return;

    QJsonObject editedMessageObj = editedMessageVal.toObject();
    quint64 chatId = static_cast<quint64>(editedMessageObj.value("chat_id").toInteger(-1));
    quint64 messageId = static_cast<quint64>(editedMessageObj.value("message_id").toInteger(-1));
    QString message = editedMessageObj.value("message").toString();

    if (chatId == ULONG_LONG_MAX || messageId == ULONG_LONG_MAX || message.isEmpty())
        return;

    emit messageEdited(chatId, messageId, message);
}
void WebsocketService::on_messageDeleted(const QJsonObject &payload)
{
    quint64 chatId = static_cast<quint64>(payload.value("chat_id").toInteger(-1));
    if (chatId == ULONG_LONG_MAX || !(payload.value("deleted_message_ids").isArray()) || payload.value("deleted_message_ids").toArray().isEmpty())
        return;

    QJsonArray deletedMessageIds = payload.value("deleted_message_ids").toArray();
    std::vector<quint64> deletedMessageIdVector;
    for (const QJsonValue &idValue : std::as_const(deletedMessageIds))
    {
        deletedMessageIdVector.push_back(static_cast<quint64>(idValue.toInteger()));
    }

    emit messageDeleted(chatId, deletedMessageIdVector);
}

void WebsocketService::callHandler(const QString &type, const QJsonObject &payload)
{
    const auto it = handlersMapByTypeOfMessage.constFind(type);
    if (it == handlersMapByTypeOfMessage.cend())
        return;

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

