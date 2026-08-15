#ifndef AUTHSERVICE_H
#define AUTHSERVICE_H
#include "utils/errortypes.h"
#include <QObject>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include "utils/endpoints.h"
#include "utils/requests/retryable_request.h"

/**
 * Сервис для взаимодействия с API авторизации.
 * Содержит логику сетевых запросов (регистрация, вход, выход, обновление токенов) без привязки к UI.
 */
class AuthService : public QObject
{
    Q_OBJECT
public:
    explicit AuthService(QObject *parent = nullptr);

    /**
     * Выполняет сетевой запрос на регистрацию нового пользователя.
     * \param login введенный логин
     * \param password введенный пароль
     */
    void registerUser(const QString &login, const QString &password);

    /**
     * Выполняет сетевой запрос на авторизацию (вход) пользователя.
     * \param login введенный логин
     * \param password введенный пароль
     */
    void logIn(const QString &login, const QString &password);

    /**
     * Выполняет POST-запрос на завершение сессии (отзыв refresh токена).
     * \param accToken текущий access токен (Access Token)
     * \param refToken текущий refresh токен (Refresh Token)
     */
    void logOut(const QString &accToken, const QString &refToken);

    /**
     * Выполняет сетевой запрос на обновление токенов доступа.
     * \param refToken текущий refresh токен (Refresh Token)
     * \param retryableReq
     */
    void refreshAccessToken(const QString &refToken, RetryableRequest retryableReq);

signals:
    // --- Сигналы процессов ---
    
    /**
     * Сигнал о начале сетевого запроса регистрации.
     */
    void registrationInProgress();

    /**
     * Сигнал о начале сетевого запроса авторизации.
     */
    void logInProgress();

    /**
     * Сигнал о начале сетевого запроса выхода.
     */
    void logOutInProgress();

    /**
     * Сигнал о начале сетевого запроса обновления токена.
     */
    void refreshAccessTokenInProgress();

    // --- Сигналы завершения запросов ---

    /**
     * Сигнал об окончании сетевого запроса регистрации.
     * \param res результат выполнения запроса
     * \param accToken новый access токен при успехе
     * \param refToken новый refresh токен при успехе
     */
    void registrationFinished(const NetworkResult &res, const QString &accToken = "", const QString &refToken = "");
    
    /**
     * Сигнал об окончании сетевого запроса авторизации.
     * \param res результат выполнения запроса
     * \param accToken новый access токен при успехе
     * \param refToken новый refresh токен при успехе
     */
    void logInFinished(const NetworkResult &res, const QString &accToken = "", const QString &refToken = "");
    
    /**
     * Сигнал об окончании сетевого запроса выхода из аккаунта.
     * \param res результат выполнения запроса
     */
    void logOutFinished(const NetworkResult &res);
    
    /**
     * Сигнал об окончании сетевого запроса обновления токена.
     * \param res результат выполнения запроса
     * \param accToken новый access токен при успехе
     * \param refToken новый refresh токен при успехе
     */
    void refreshAccessTokenFinished(const NetworkResult &res, RetryableRequest req, const QString &accToken = "", const QString &refToken = "");

private:
    // --- Утилиты ---

    /**
     * Сохраняет refresh токен в безопасное хранилище (Keychain).
     * \param parent родительский объект
     * \param token токен для сохранения
     */
    void writeRefreshTokenToKeychain(QObject *parent, const QString &token);

    // --- Внутренние объекты сети ---
    QNetworkAccessManager *network;                       //!< Менеджер сети для выполнения HTTP-запросов

    // --- Адреса API (Endpoints) ---
    QString                baseUrl;                       //!< Базовый адрес API
    QString                registerUrl;                   //!< Путь API для регистрации
    QString                logInUrl;                      //!< Путь API для авторизации
    QString                refreshAccessTokenUrl;         //!< Путь API для обновления токена
    QString                logOutUrl;                     //!< Путь API для выхода из аккаунта
};

#endif // AUTHSERVICE_H
