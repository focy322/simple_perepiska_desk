#include "userinfoservice.h"

UserInfoService::UserInfoService(QObject *parent)
    : QObject{parent}
    , network(new QNetworkAccessManager(this))
    , baseUrl("https://messenger-3yfw.onrender.com")
    , myUserInfoUrl("/api/users/me")
    , userByUsernameUrl("/api/users/search")
{}

void UserInfoService::getMyUserInfo(const QString &accToken)
{
    emit getMyUserInfoInProgress();
    QUrl url(baseUrl + myUserInfoUrl);
    QNetworkRequest req(url);
    // Передаем токен в заголовке
    req.setRawHeader("Authorization", "Bearer " + accToken.toUtf8());

    QNetworkReply * reply = network->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply](){
        auto httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (reply->error() != QNetworkReply::NoError && httpCode == 0)
        {
            NetworkResult res{false, ERROR_TYPES::UNKNOWN_ERROR, messageForError(ERROR_TYPES::UNKNOWN_ERROR)};
            emit getMyUserInfoFinished(res);
            reply->deleteLater();
            return;
        }
        QByteArray raw = reply->readAll();
        QJsonParseError pe;
        QJsonDocument doc = QJsonDocument::fromJson(raw, &pe);
        if (pe.error || !doc.isObject())
        {
            NetworkResult res{false, ERROR_TYPES::UNKNOWN_ERROR, messageForError(ERROR_TYPES::UNKNOWN_ERROR)};
            emit getMyUserInfoFinished(res);
            reply->deleteLater();
            return;
        }
        if (httpCode == 200)
        {
            QString username = doc.object()["username"].toString();
            unsigned long long userId = doc.object()["user_id"].toVariant().toULongLong();
            
            NetworkResult res{true, ERROR_TYPES::NO_ERROR, messageForError(ERROR_TYPES::NO_ERROR)};
            emit getMyUserInfoFinished(res, username, userId);
            reply->deleteLater();
            return;
        }
        NetworkResult res{false, ERROR_TYPES::UNKNOWN_ERROR, messageForError(ERROR_TYPES::UNKNOWN_ERROR)};
        emit getMyUserInfoFinished(res);
        reply->deleteLater();
    });
}
