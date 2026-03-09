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
    if (login.isEmpty())
    {
        emit registrationFinished(AuthResult{false, AUTH_ERRORS::EmptyLogin, messageForError(AUTH_ERRORS::EmptyLogin)});
        return;
    }
    if (login.size() < 3)
    {
        emit registrationFinished(AuthResult{false, AUTH_ERRORS::ShortLogin, messageForError(AUTH_ERRORS::ShortLogin)});
        return;
    }
    if (password.isEmpty())
    {
        emit registrationFinished(AuthResult{false, AUTH_ERRORS::EmptyPassword, messageForError(AUTH_ERRORS::EmptyPassword)});
        return;
    }
    if (password.size() < 6)
    {
        emit registrationFinished(AuthResult{false, AUTH_ERRORS::ShortPassword, messageForError(AUTH_ERRORS::ShortPassword)});
        return;
    }
    if (passwordConfirm.isEmpty())
    {
        emit registrationFinished(AuthResult{false, AUTH_ERRORS::EmptyPasswordConfirm, messageForError(AUTH_ERRORS::EmptyPasswordConfirm)});
        return;
    }
    if (password != passwordConfirm)
    {
        emit registrationFinished(AuthResult{false, AUTH_ERRORS::PasswordMismatch, messageForError(AUTH_ERRORS::PasswordMismatch)});
        return;
    }

    QUrl url(baseUrl + registerUrl);
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json"); // Поставить JSON-заголовок
    QJsonObject obj
    {
        {"username", login},
        {"password", password},
    };
    QByteArray body = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    QNetworkReply * reply = network->post(req, body);
    connect(reply, &QNetworkReply::finished, this, [this, reply](){
        if (reply->error() != QNetworkReply::NoError)
        {
            // TODO: Определение конкретной ошибки
            AuthResult res{false, AUTH_ERRORS::ErrorsCount, messageForError(AUTH_ERRORS::ErrorsCount)};
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
            AuthResult res{false, AUTH_ERRORS::ErrorsCount, messageForError(AUTH_ERRORS::ErrorsCount)};
            emit registrationFinished(res);
            reply->deleteLater();
            return;
        }
        auto httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (httpCode == 200)
        {
            // TODO: Определение конкретной ошибки
            AuthResult res{true, AUTH_ERRORS::NoError, messageForError(AUTH_ERRORS::NoError)};
            emit registrationFinished(res);
            reply->deleteLater();
            return;
        }
        AuthResult res{false, AUTH_ERRORS::ErrorsCount, messageForError(AUTH_ERRORS::ErrorsCount)};
        emit registrationFinished(res);
        reply->deleteLater();
        return;
    } );

}


void AuthService::logIn(const QString &login, const QString &password)
{
    if (login.isEmpty())
    {
        emit logInFinished(AuthResult{false, AUTH_ERRORS::EmptyLogin, messageForError(AUTH_ERRORS::EmptyLogin)});
        return;
    }
    if (login.size() < 3)
    {
        emit logInFinished(AuthResult{false, AUTH_ERRORS::ShortLogin, messageForError(AUTH_ERRORS::ShortLogin)});
        return;
    }
    if (password.isEmpty())
    {
        emit logInFinished(AuthResult{false, AUTH_ERRORS::EmptyPassword, messageForError(AUTH_ERRORS::EmptyPassword)});
        return;
    }
    if (password.size() < 6)
    {
        emit logInFinished(AuthResult{false, AUTH_ERRORS::ShortPassword, messageForError(AUTH_ERRORS::ShortPassword)});
        return;
    }
    // тут к апишке обращение надо
    emit logInFinished(AuthResult{true, AUTH_ERRORS::NoError, messageForError(AUTH_ERRORS::NoError)});
    return;
}


void AuthService::logOut()
{
    emit logOutFinished(AuthResult{true, AUTH_ERRORS::NoError, messageForError(AUTH_ERRORS::NoError)});
    return;
}
