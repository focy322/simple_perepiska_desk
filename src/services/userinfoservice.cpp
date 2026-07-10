#include "services/userinfoservice.h"
#include <QUrlQuery>

UserInfoService::UserInfoService(QObject *parent)
    : QObject{parent}
    , network(new QNetworkAccessManager(this))
    , baseUrl(baseHttpUrl)
    , myUserInfoUrl("/api/users/me")
    , userByUsernameUrl("/api/users/search")
    , findUserUrl("/api/users/search")
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
            NetworkResult res{false, ERROR_TYPES::UNKNOWN_ERROR, generateMessageForError(ERROR_TYPES::UNKNOWN_ERROR)};
            emit getMyUserInfoFinished(res);
            reply->deleteLater();
            return;
        }
        QByteArray raw = reply->readAll();
        QJsonParseError pe;
        QJsonDocument doc = QJsonDocument::fromJson(raw, &pe);
        if (pe.error || !doc.isObject())
        {
            NetworkResult res{false, ERROR_TYPES::UNKNOWN_ERROR, generateMessageForError(ERROR_TYPES::UNKNOWN_ERROR)};
            emit getMyUserInfoFinished(res);
            reply->deleteLater();
            return;
        }
        if (httpCode == 200)
        {
            QString username = doc.object()["username"].toString();
            unsigned long long userId = doc.object()["user_id"].toVariant().toULongLong();
            
            NetworkResult res{true, ERROR_TYPES::NO_ERROR, generateMessageForError(ERROR_TYPES::NO_ERROR)};
            emit getMyUserInfoFinished(res, username, userId);
            reply->deleteLater();
            return;
        }
        NetworkResult res{false, ERROR_TYPES::UNKNOWN_ERROR, generateMessageForError(ERROR_TYPES::UNKNOWN_ERROR)};
        emit getMyUserInfoFinished(res);
        reply->deleteLater();
    });
}


const std::vector<ParsedFoundUsersObject> UserInfoService::parseFoundUsersArray(const QJsonDocument &doc)
{
    std::vector<ParsedFoundUsersObject> parsedArrayObjects;

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

    const QJsonArray users = doc.array();
    for (const QJsonValue &userValue : users)
    {
        if (!userValue.isObject())
            continue;

        const QJsonObject userObject = userValue.toObject();

        ParsedFoundUsersObject paObj;
        paObj.username = userObject.value("username").toString();
        paObj.nickname = userObject.value("nickname").toString();
        paObj.userId = toUnsignedLongLong(userObject.value("user_id"));
        paObj.lastSeen = userObject.value("last_seen").toString();
        paObj.avatarFileUrl = userObject.value("avatar_file_url").toString();

        parsedArrayObjects.push_back(paObj);
    }

    return parsedArrayObjects;
}


void UserInfoService::findUser(const QString &accToken, const QString &input)
{
    emit findUserInProgress();
    QUrl url(baseUrl + findUserUrl);
    QUrlQuery query;
    query.addQueryItem("query", input);
    query.addQueryItem("limit", QString::number(10));
    url.setQuery(query);

    QNetworkRequest req(url);
    req.setRawHeader("Authorization", "Bearer " + accToken.toUtf8());

    QNetworkReply * reply = network->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply](){
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
            const auto parsedArrayObjects = parseFoundUsersArray(doc);
            NetworkResult res{true, ERROR_TYPES::NO_ERROR, generateMessageForError(ERROR_TYPES::NO_ERROR)};
            emit findUserFinished(res, parsedArrayObjects);
            reply->deleteLater();
            return;
        }
        NetworkResult res{false, ERROR_TYPES::UNKNOWN_ERROR, generateMessageForError(ERROR_TYPES::UNKNOWN_ERROR)};
        emit findUserFinished(res);
        reply->deleteLater();
    });


}
