#include "authservice.h"
#include <QSysInfo>
#include <fstream>
#include <QUrlQuery>

AuthService::AuthService(QObject *parent)
    : QObject{parent}
    , network(new QNetworkAccessManager(this))
    , baseUrl("https://messenger-3yfw.onrender.com")
    , registerUrl("/api/users/")
    , logInUrl("/api/auth/token")
{

}


void AuthService::registerUser(const QString &login, const QString &password, const QString &passwordConfirm)
{
    auto res = validateRegistration(login, password, passwordConfirm);
    if (!res.ok)
    {
        emit registrationFinished(res);
        return;
    }

    QUrl url(baseUrl + registerUrl);
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    // Данные о системе
    QString ua = QString("Simple_Perepiska_lol_kek/0.01 (%1; %2)").arg(QSysInfo::prettyProductName(), QSysInfo::currentCpuArchitecture());
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
    emit registrationInProgress();
    connect(reply, &QNetworkReply::finished, this, [this, reply](){
        auto httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        // TODO: обработка ошибок httpCode
        if (reply->error() != QNetworkReply::NoError && httpCode == 0)
        {
            // TODO: Определение конкретной ошибки
            AuthResult res{false, AUTH_ERRORS::UNKNOWN_ERROR, messageForError(AUTH_ERRORS::UNKNOWN_ERROR)};
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
            AuthResult res{false, AUTH_ERRORS::UNKNOWN_ERROR, messageForError(AUTH_ERRORS::UNKNOWN_ERROR)};
            emit registrationFinished(res);
            reply->deleteLater();
            return;
        }
        // TODO: ошибки от 400 до 499
        if (httpCode == 400)
        {
            AuthResult res{false, AUTH_ERRORS::LOGIN_ALREADY_EXISTS, messageForError(AUTH_ERRORS::LOGIN_ALREADY_EXISTS)};
            emit registrationFinished(res);
            reply->deleteLater();
            return;
        }
        if (httpCode == 200 || httpCode == 201)
        {
            std::fstream accessToken("accessToken.txt", std::ios_base::trunc | std::ios_base::out);
            QString accToken = doc.object()["token"].toObject()["access_token"].toString();
            accessToken.write(accToken.toStdString().c_str(), accToken.size());

            std::fstream refreshToken("refreshToken.txt", std::ios_base::trunc | std::ios_base::out);
            QString refToken = doc.object()["token"].toObject()["refresh_token"].toString();
            refreshToken.write(refToken.toStdString().c_str(), refToken.size());

            AuthResult res{true, AUTH_ERRORS::NO_ERROR, messageForError(AUTH_ERRORS::NO_ERROR)};
            emit registrationFinished(res);
            reply->deleteLater();
            return;
        }
        AuthResult res{false, AUTH_ERRORS::UNKNOWN_ERROR, messageForError(AUTH_ERRORS::UNKNOWN_ERROR)};
        emit registrationFinished(res);
        reply->deleteLater();
        return;
    } );

}


void AuthService::logIn(const QString &login, const QString &password)
{

    const AuthResult& res = validateLogIn(login, password);
    if (!res.ok)
    {
        emit logInFinished(res);
        return;
    }

    QUrl url(baseUrl + logInUrl);
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded"); // Поставить JSON-заголовок
    QString ua = QString("Simple_Perepiska_lol_kek/0.01 (%1; %2)").arg(QSysInfo::prettyProductName(), QSysInfo::currentCpuArchitecture());
    req.setHeader(QNetworkRequest::UserAgentHeader, ua);

    QUrlQuery form; // Форма для запроса Авторизации
    form.addQueryItem("username", login);
    form.addQueryItem("password", password);

    QByteArray body = form.query(QUrl::FullyEncoded).toUtf8();
#ifdef QT_DEBUG
    qDebug() << body;
#endif

    QNetworkReply * reply = network->post(req, body);
    emit logInProgress();
    connect(reply, &QNetworkReply::finished, this, [this, reply](){
        auto httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        // TODO: обработка ошибок httpCode
        if (reply->error() != QNetworkReply::NoError && httpCode == 0)
        {
            // TODO: Определение конкретной ошибки
            AuthResult res{false, AUTH_ERRORS::UNKNOWN_ERROR, messageForError(AUTH_ERRORS::UNKNOWN_ERROR)};
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
            AuthResult res{false, AUTH_ERRORS::UNKNOWN_ERROR, messageForError(AUTH_ERRORS::UNKNOWN_ERROR)};
            emit logInFinished(res);
            reply->deleteLater();
            return;
        }
        // TODO: ошибки от 400 до 499
        if (httpCode == 422)
        {
            qDebug() << doc;
            AuthResult res{false, AUTH_ERRORS::UNKNOWN_ERROR, messageForError(AUTH_ERRORS::UNKNOWN_ERROR)};
            emit logInFinished(res);
            reply->deleteLater();
            return;
        }
        if (httpCode == 200 || httpCode == 201)
        {
            std::fstream accessToken("accessToken.txt", std::ios_base::trunc | std::ios_base::out);
            QString accToken = doc.object()["access_token"].toString();
            accessToken.write(accToken.toStdString().c_str(), accToken.size());

            std::fstream refreshToken("refreshToken.txt", std::ios_base::trunc | std::ios_base::out);
            QString refToken = doc.object()["refresh_token"].toString();
            refreshToken.write(refToken.toStdString().c_str(), refToken.size());
            AuthResult res{true, AUTH_ERRORS::NO_ERROR, messageForError(AUTH_ERRORS::NO_ERROR)};
            emit logInFinished(res);
            reply->deleteLater();
            return;
        }
        AuthResult res{false, AUTH_ERRORS::UNKNOWN_ERROR, messageForError(AUTH_ERRORS::UNKNOWN_ERROR)};
        emit logInFinished(res);
        reply->deleteLater();
        return;
    } );
}


void AuthService::logOut()
{
    // TODO: Реализовать обращение к API-шке (её пока нет)
    //emit logOutInProgress();
    emit logOutFinished(AuthResult{true, AUTH_ERRORS::NO_ERROR, messageForError(AUTH_ERRORS::NO_ERROR)});
    return;
}
