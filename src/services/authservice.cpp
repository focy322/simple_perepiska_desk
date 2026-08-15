#include "services/authservice.h"

#include <keychain.h>

#include <QSysInfo>
#include <QUrlQuery>

AuthService::AuthService(QObject *parent)
    : QObject{parent}
    , network(new QNetworkAccessManager(this))
    , baseUrl(baseHttpUrl)
    , registerUrl("/api/users/")
    , logInUrl("/api/auth/token")
    , refreshAccessTokenUrl("/api/auth/token/refresh")
    , logOutUrl("/api/auth/token/revoke")
{

}

void AuthService::writeRefreshTokenToKeychain(QObject *parent, const QString &token)
{
    auto *job = new QKeychain::WritePasswordJob("Vent", parent);
    job->setKey("vent_refresh_token");
    job->setTextData(token);

    connect(job, &QKeychain::Job::finished, job, [job]() {
#ifdef QT_DEBUG
        if (job->error())
            qDebug() << "qtkeychain write error:" << job->errorString();
#endif
        job->deleteLater();
    });

    job->start();
}

void AuthService::registerUser(const QString &login, const QString &password)
{
    emit registrationInProgress();

    QUrl url(baseUrl + registerUrl);
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    // Данные о системе
    QString ua = QString("Vent Desktop (%1; %2)").arg(QSysInfo::prettyProductName(), QSysInfo::currentCpuArchitecture());
    req.setHeader(QNetworkRequest::UserAgentHeader, ua);

    QJsonObject obj // Вид тела запроса для регистрации
    {
        {"username", login},
        {"password", password},
    };

    QByteArray body = QJsonDocument(obj).toJson(QJsonDocument::Compact);
#ifdef QT_DEBUG
    qDebug() << body;
#endif

    QNetworkReply * reply = network->post(req, body);
    connect(reply, &QNetworkReply::finished, this, [this, reply](){
        auto httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        // TODO: обработка ошибок httpCode
        if (reply->error() != QNetworkReply::NoError && httpCode == 0)
        {
            // TODO: Определение конкретной ошибки
            NetworkResult res{false, ERROR_TYPES::UNKNOWN_ERROR, generateMessageForError(ERROR_TYPES::UNKNOWN_ERROR)};
            emit registrationFinished(res);
            reply->deleteLater();
            return;
        }
        QByteArray raw = reply->readAll();
        QJsonParseError pe;
        QJsonDocument doc = QJsonDocument::fromJson(raw, &pe);
        if (pe.error || !doc.isObject())
        {
            // TODO: Определение конкретной ошибки
            NetworkResult res{false, ERROR_TYPES::UNKNOWN_ERROR, generateMessageForError(ERROR_TYPES::UNKNOWN_ERROR)};
            emit registrationFinished(res);
            reply->deleteLater();
            return;
        }
        // TODO: ошибки от 400 до 499
        if (httpCode == 400)
        {
            NetworkResult res{false, ERROR_TYPES::LOGIN_ALREADY_EXISTS, generateMessageForError(ERROR_TYPES::LOGIN_ALREADY_EXISTS)};
            emit registrationFinished(res);
            reply->deleteLater();
            return;
        }
        if (httpCode == 200 || httpCode == 201)
        {
            QString accToken = doc.object()["token"].toObject()["access_token"].toString();

            QString refToken = doc.object()["token"].toObject()["refresh_token"].toString();
            writeRefreshTokenToKeychain(this, refToken);

            NetworkResult res{true, ERROR_TYPES::NO_ERROR, generateMessageForError(ERROR_TYPES::NO_ERROR)};
            emit registrationFinished(res, accToken, refToken);
            reply->deleteLater();
            return;
        }
        NetworkResult res{false, static_cast<ERROR_TYPES>(httpCode), generateMessageForError(static_cast<ERROR_TYPES>(httpCode))};
        emit registrationFinished(res);
        reply->deleteLater();
        return;
    } );

}


void AuthService::logIn(const QString &login, const QString &password)
{
    emit logInProgress();
    QUrl url(baseUrl + logInUrl);
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded"); // Поставить JSON-заголовок
    QString ua = QString("Vent Desktop/0.01 (%1; %2)").arg(QSysInfo::prettyProductName(), QSysInfo::currentCpuArchitecture());
    req.setHeader(QNetworkRequest::UserAgentHeader, ua);

    QUrlQuery form; // Форма для запроса Авторизации
    form.addQueryItem("username", login);
    form.addQueryItem("password", password);

    QByteArray body = form.query(QUrl::FullyEncoded).toUtf8();
#ifdef QT_DEBUG
    qDebug() << body;
#endif

    QNetworkReply * reply = network->post(req, body);
    connect(reply, &QNetworkReply::finished, this, [this, reply](){
        auto httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        // TODO: обработка ошибок httpCode
        if (reply->error() != QNetworkReply::NoError && httpCode == 0)
        {
            // TODO: Определение конкретной ошибки
            NetworkResult res{false, ERROR_TYPES::UNKNOWN_ERROR, generateMessageForError(ERROR_TYPES::UNKNOWN_ERROR)};
            emit logInFinished(res);
            reply->deleteLater();
            return;
        }
        QByteArray raw = reply->readAll();
        QJsonParseError pe;
        QJsonDocument doc = QJsonDocument::fromJson(raw, &pe);
        if (pe.error || !doc.isObject())
        {
            // TODO: Определение конкретной ошибки
            NetworkResult res{false, ERROR_TYPES::UNKNOWN_ERROR, generateMessageForError(ERROR_TYPES::UNKNOWN_ERROR)};
            emit logInFinished(res);
            reply->deleteLater();
            return;
        }
        // TODO: ошибки от 400 до 499
        if (httpCode == 422)
        {
#ifdef QT_DEBUG
            qDebug() << doc;
#endif
            NetworkResult res{false, static_cast<ERROR_TYPES>(httpCode), generateMessageForError(static_cast<ERROR_TYPES>(httpCode))};
            emit logInFinished(res);
            reply->deleteLater();
            return;
        }
        if (httpCode == 200 || httpCode == 201)
        {
            QString accToken = doc.object()["access_token"].toString();

            QString refToken = doc.object()["refresh_token"].toString();
            writeRefreshTokenToKeychain(this, refToken);
            NetworkResult res{true, ERROR_TYPES::NO_ERROR, generateMessageForError(ERROR_TYPES::NO_ERROR)};
            emit logInFinished(res, accToken, refToken);
            reply->deleteLater();
            return;
        }
        NetworkResult res{false, static_cast<ERROR_TYPES>(httpCode), generateMessageForError(static_cast<ERROR_TYPES>(httpCode))};
#ifdef QT_DEBUG
        qDebug() << doc;
#endif
        emit logInFinished(res);
        reply->deleteLater();
        return;
    } );
}


void AuthService::logOut(const QString &accToken, const QString &refToken)
{
    emit logOutInProgress();
    QUrl url(baseUrl + logOutUrl);
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    // Передаем токен в заголовке
    req.setRawHeader("Authorization", "Bearer " + accToken.toUtf8());

    QJsonObject obj
        {
            {"refresh_token", refToken}
        };

    QByteArray body = QJsonDocument(obj).toJson(QJsonDocument::Compact);
#ifdef QT_DEBUG
    qDebug() << "RefreshToken" << body;
#endif

    QNetworkReply * reply = network->post(req, body);
    connect(reply, &QNetworkReply::finished, this, [this, reply](){
        auto httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (reply->error() != QNetworkReply::NoError && httpCode == 0)
        {
            NetworkResult res{false, ERROR_TYPES::UNKNOWN_ERROR, generateMessageForError(ERROR_TYPES::UNKNOWN_ERROR)};
            emit logOutFinished(res);
            reply->deleteLater();
            return;
        }
        if (httpCode == 200 || httpCode == 201)
        {
            NetworkResult res{true, ERROR_TYPES::NO_ERROR, generateMessageForError(ERROR_TYPES::NO_ERROR)};
            emit logOutFinished(res);
            reply->deleteLater();
            return;
        }
        NetworkResult res{false, static_cast<ERROR_TYPES>(httpCode), generateMessageForError(static_cast<ERROR_TYPES>(httpCode))};
#ifdef QT_DEBUG
        qDebug() << "logOutError code: " << httpCode;
#endif
        emit logOutFinished(res);
        reply->deleteLater();
    });
}

void AuthService::refreshAccessToken(const QString &refToken, RetryableRequest retryableReq)
{
    emit refreshAccessTokenInProgress();
    QUrl url(baseUrl + refreshAccessTokenUrl);
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    // Данные о системе
    QString ua = QString("Vent Desktop/0.01 (%1; %2)").arg(QSysInfo::prettyProductName(), QSysInfo::currentCpuArchitecture());
    req.setHeader(QNetworkRequest::UserAgentHeader, ua);

    QJsonObject obj
    {
        {"refresh_token", refToken}
    };

    QByteArray body = QJsonDocument(obj).toJson(QJsonDocument::Compact);
#ifdef QT_DEBUG
    qDebug() << "RefreshToken" << body;
#endif

    QNetworkReply * reply = network->post(req, body);
    connect(reply, &QNetworkReply::finished, this, [this, reply, retryableReq]()
    {
        auto httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (reply->error() != QNetworkReply::NoError && httpCode == 0)
        {
            NetworkResult res{false, ERROR_TYPES::UNKNOWN_ERROR, generateMessageForError(ERROR_TYPES::UNKNOWN_ERROR)};
            emit refreshAccessTokenFinished(res, retryableReq);
            reply->deleteLater();
            return;
        }
        QByteArray raw = reply->readAll();
        QJsonParseError pe;
        QJsonDocument doc = QJsonDocument::fromJson(raw, &pe);
        if (pe.error || !doc.isObject())
        {
            NetworkResult res{false, ERROR_TYPES::UNKNOWN_ERROR, generateMessageForError(ERROR_TYPES::UNKNOWN_ERROR)};
            emit refreshAccessTokenFinished(res, retryableReq);
            reply->deleteLater();
            return;
        }
        if (httpCode == 200 || httpCode == 201)
        {
            QString accToken = doc.object()["access_token"].toString();
            QString newRefToken = doc.object()["refresh_token"].toString();
            
            writeRefreshTokenToKeychain(this, newRefToken);
            
            NetworkResult res{true, ERROR_TYPES::NO_ERROR, generateMessageForError(ERROR_TYPES::NO_ERROR)};
            emit refreshAccessTokenFinished(res, retryableReq, accToken, newRefToken);
            reply->deleteLater();
            return;
        }
        NetworkResult res{false, static_cast<ERROR_TYPES>(httpCode), generateMessageForError(static_cast<ERROR_TYPES>(httpCode))};
        emit refreshAccessTokenFinished(res, retryableReq);
        reply->deleteLater();
    });
}
