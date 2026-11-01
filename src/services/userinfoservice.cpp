#include "services/userinfoservice.h"
#include <QUrlQuery>
#include <QHttpMultiPart>
#include <QHttpPart>

#include "requests/retryable_request.h"

UserInfoService::UserInfoService(QObject *parent)
    : QObject{parent}
    , network(new QNetworkAccessManager(this))
{}

void UserInfoService::getMyUserInfo(const QString &accToken, RetryableRequest retryableReq)
{
    emit getMyUserInfoInProgress();
    QUrl url(baseHttpUrl + myUserInfoUrl);
    QNetworkRequest req(url);
    // Передаем токен в заголовке
    req.setRawHeader("Authorization", "Bearer " + accToken.toUtf8());

    QNetworkReply * reply = network->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, retryableReq](){
        auto httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (reply->error() != QNetworkReply::NoError && httpCode == 0)
        {
            NetworkResult res{false, ERROR_TYPES::UNKNOWN_ERROR, generateMessageForError(ERROR_TYPES::UNKNOWN_ERROR)};
            emit getMyUserInfoFinished(res, retryableReq);
            reply->deleteLater();
            return;
        }
        QByteArray raw = reply->readAll();
        QJsonParseError pe;
        QJsonDocument doc = QJsonDocument::fromJson(raw, &pe);
        if (pe.error || !doc.isObject())
        {
            NetworkResult res{false, ERROR_TYPES::UNKNOWN_ERROR, generateMessageForError(ERROR_TYPES::UNKNOWN_ERROR)};
            emit getMyUserInfoFinished(res, retryableReq);
            reply->deleteLater();
            return;
        }
        if (httpCode == 200)
        {
            QString username = doc.object()["username"].toString();
            unsigned long long userId = doc.object()["user_id"].toVariant().toULongLong();
            QString avatarUrl = doc.object()["avatar_file_url"].toString();
            
            NetworkResult res{true, ERROR_TYPES::NO_ERROR, generateMessageForError(ERROR_TYPES::NO_ERROR)};
            emit getMyUserInfoFinished(res,retryableReq, username, userId, avatarUrl);
            reply->deleteLater();
            return;
        }
        NetworkResult res{false, static_cast<ERROR_TYPES>(httpCode), generateMessageForError(static_cast<ERROR_TYPES>(httpCode))};
        emit getMyUserInfoFinished(res,retryableReq);
        reply->deleteLater();
    });
}

void UserInfoService::getUserInfo(const QString &accToken, unsigned long long userId, RetryableRequest retryableReq)
{
    QUrl url(baseHttpUrl + getUserInfoUrl.arg(userId));
    QNetworkRequest req(url);
    req.setRawHeader("Authorization", "Bearer " + accToken.toUtf8());

    QNetworkReply * reply = network->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, retryableReq](){
        auto httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (reply->error() != QNetworkReply::NoError && httpCode == 0)
        {
            NetworkResult res{false, ERROR_TYPES::UNKNOWN_ERROR, generateMessageForError(ERROR_TYPES::UNKNOWN_ERROR)};
            emit getUserInfoFinished(res, retryableReq);
            reply->deleteLater();
            return;
        }
        QByteArray raw = reply->readAll();
        QJsonParseError pe;
        QJsonDocument doc = QJsonDocument::fromJson(raw, &pe);
        if (pe.error || !doc.isObject())
        {
            NetworkResult res{false, ERROR_TYPES::UNKNOWN_ERROR, generateMessageForError(ERROR_TYPES::UNKNOWN_ERROR)};
            emit getUserInfoFinished(res, retryableReq);
            reply->deleteLater();
            return;
        }
        if (httpCode == 200)
        {
            ParsedFoundUsersObject paObj;
            const QJsonObject userObject = doc.object();
            paObj.username = userObject.value("username").toString();
            paObj.nickname = userObject.value("nickname").toString();
            paObj.userId = userObject.value("user_id").toVariant().toULongLong();
            paObj.lastSeen = userObject.value("last_seen").toString();
            paObj.avatarFileUrl = userObject.value("avatar_file_url").toString();
            
            NetworkResult res{true, ERROR_TYPES::NO_ERROR, generateMessageForError(ERROR_TYPES::NO_ERROR)};
            emit getUserInfoFinished(res, retryableReq, paObj);
            reply->deleteLater();
            return;
        }
        NetworkResult res{false, static_cast<ERROR_TYPES>(httpCode), generateMessageForError(static_cast<ERROR_TYPES>(httpCode))};
        emit getUserInfoFinished(res, retryableReq);
        reply->deleteLater();
    });
}

void UserInfoService::uploadAvatar(const QString &accToken, const QByteArray &imageData, RetryableRequest retryableReq)
{
    QUrl url(baseHttpUrl + uploadAvatarUrl);
    QNetworkRequest req(url);
    req.setRawHeader("Authorization", "Bearer " + accToken.toUtf8());

    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    QHttpPart imagePart;
    imagePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("image/png"));
    imagePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"file\"; filename=\"avatar.png\""));
    imagePart.setBody(imageData);
    multiPart->append(imagePart);

    QNetworkReply *reply = network->sendCustomRequest(req, "PATCH", multiPart);
    multiPart->setParent(reply); // удалить вместе с reply

    connect(reply, &QNetworkReply::finished, this, [this, reply, retryableReq](){
        auto httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (reply->error() != QNetworkReply::NoError && httpCode == 0)
        {
            NetworkResult res{false, ERROR_TYPES::UNKNOWN_ERROR, generateMessageForError(ERROR_TYPES::UNKNOWN_ERROR)};
            emit uploadAvatarFinished(res, retryableReq);
            reply->deleteLater();
            return;
        }
        QByteArray raw = reply->readAll();
        QJsonParseError pe;
        QJsonDocument doc = QJsonDocument::fromJson(raw, &pe);
        if (httpCode == 200 && !pe.error && doc.isObject())
        {
            QString avatarUrl = doc.object()["avatar_file_url"].toString();
            NetworkResult res{true, ERROR_TYPES::NO_ERROR, ""};
            emit uploadAvatarFinished(res, retryableReq, avatarUrl);
            reply->deleteLater();
            return;
        }
        QString errMsg = "Ошибка загрузки аватара";
        if (doc.isObject() && doc.object().contains("detail")) {
            errMsg += ": " + doc.object()["detail"].toString();
        } else if (!raw.isEmpty()) {
            errMsg += QString(" (Code: %1, Body: %2)").arg(httpCode).arg(QString::fromUtf8(raw).left(200));
        } else {
            errMsg += QString(" (Code: %1, Error: %2)").arg(httpCode).arg(reply->errorString());
        }
        NetworkResult res{false, static_cast<ERROR_TYPES>(httpCode), errMsg};
        emit uploadAvatarFinished(res, retryableReq);
        reply->deleteLater();
    });
}

void UserInfoService::findUser(const QString &accToken, const QString &input)
{
    emit findUserInProgress();
    QUrl url(baseHttpUrl + findUserUrl);
    QUrlQuery query;
    query.addQueryItem("query", input);
    query.addQueryItem("limit", QString::number(10));
    url.setQuery(query);

    QNetworkRequest req(url);
    req.setRawHeader("Authorization", "Bearer " + accToken.toUtf8());

    QNetworkReply * reply = network->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, input](){
        auto httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (reply->error() != QNetworkReply::NoError && httpCode == 0)
        {
            NetworkResult res{false, ERROR_TYPES::UNKNOWN_ERROR, generateMessageForError(ERROR_TYPES::UNKNOWN_ERROR)};
            emit findUserFinished(res);
            reply->deleteLater();
            return;
        }
        QByteArray raw = reply->readAll();
        QJsonParseError pe;
        QJsonDocument doc = QJsonDocument::fromJson(raw, &pe);
        if (pe.error || !doc.isArray())
        {
            NetworkResult res{false, ERROR_TYPES::UNKNOWN_ERROR, generateMessageForError(ERROR_TYPES::UNKNOWN_ERROR)};
            emit findUserFinished(res);
            reply->deleteLater();
            return;
        }
        if (httpCode == 200)
        {
            auto parsedArrayObjects = parseFoundUsersArray(doc);

            NetworkResult res{true, ERROR_TYPES::NO_ERROR, generateMessageForError(ERROR_TYPES::NO_ERROR)};
            emit findUserFinished(res, parsedArrayObjects);
            reply->deleteLater();
            return;
        }
        if (httpCode == 401)
        {
            NetworkResult res{false, ERROR_TYPES::UNAUTHORIZED, generateMessageForError(ERROR_TYPES::UNAUTHORIZED)};
            emit findUserFinished(res, {}, input);
            reply->deleteLater();
            return;
        }

        NetworkResult res{false, static_cast<ERROR_TYPES>(httpCode), generateMessageForError(static_cast<ERROR_TYPES>(httpCode))};
        emit findUserFinished(res);
        reply->deleteLater();
    });


}
