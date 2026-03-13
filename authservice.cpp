#include "authservice.h"

AuthService::AuthService(QObject *parent)
    : QObject{parent}
    , network(new QNetworkAccessManager(this))
    , baseUrl("https://simplemessenger-00tn.onrender.com")
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


    // TODO: Как боб поменяет API-шку под нее перестроить реализацию обращения к ней
    QUrl url(baseUrl + registerUrl);
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json"); // Поставить JSON-заголовок

    QJsonObject obj // Вид тела запроса для регистрации
    {
        {"username", login},
        {"password", password},
    };

    QByteArray body = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    QNetworkReply * reply = network->post(req, body);
    emit registrationInProgress();
    connect(reply, &QNetworkReply::finished, this, [this, reply](){
        if (reply->error() != QNetworkReply::NoError)
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
        auto httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (httpCode == 200)
        {
            // TODO: Определение конкретной ошибки
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
    // TODO: обращение к API (но el bob там пока что то мутит)
    //emit logInProgress();
    emit logInFinished(AuthResult{true, AUTH_ERRORS::NO_ERROR, messageForError(AUTH_ERRORS::NO_ERROR)});
    return;
}


void AuthService::logOut()
{
    // TODO: Реализовать обращение к API-шке (её пока нет)
    //emit logOutInProgress();
    emit logOutFinished(AuthResult{true, AUTH_ERRORS::NO_ERROR, messageForError(AUTH_ERRORS::NO_ERROR)});
    return;
}
