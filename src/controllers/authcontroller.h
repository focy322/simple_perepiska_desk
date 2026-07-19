#ifndef AUTHCONTROLLER_H
#define AUTHCONTROLLER_H

#include <QObject>
#include "services/authservice.h"

/**
 * Принимает запросы от UI, вызывает методы AuthService и возвращает результат через сигналы.
 */
class AuthController : public QObject
{
    Q_OBJECT
public:
    explicit AuthController(QObject *parent = nullptr);

    /**
     * Выполняет запрос на регистрацию через AuthService и посылает сигнал registrationFinished.
     * \param login введенный логин
     * \param password введенный пароль
     * \param passwordConfirm подтверждение пароля
     */
    void requestRegistration(const QString &login, const QString &password, const QString &passwordConfirm);

    /**
     * Выполняет запрос на авторизацию через AuthService и посылает сигнал logInFinished.
     * \param login введенный логин
     * \param password введенный пароль
     */
    void requestLogIn(const QString &login, const QString &password);

    /**
     * Выполняет запрос на выход из аккаунта через AuthService.
     * \param accToken текущий access токен (Access Token)
     * \param refToken текущий refresh токен (Refresh Token)
     */
    void requestLogOut(const QString &accToken, const QString &refToken);

    /**
     * Запрашивает обновление access токена по refresh токену.
     * \param refToken текущий refresh токен (Refresh Token)
     */
    void requestRefreshAccessToken(const QString &refToken);

signals:
    // --- Сигналы завершения процессов ---
    
    /**
     * Сигнал о завершении регистрации.
     * \param res результат выполнения запроса
     * \param accToken новый access токен при успехе
     * \param refToken новый refresh токен при успехе
     */
    void registrationFinished(const NetworkResult &res, const QString &accToken = "", const QString &refToken = "");
    
    /**
     * Сигнал о завершении авторизации.
     * \param res результат выполнения запроса
     * \param accToken новый access токен при успехе
     * \param refToken новый refresh токен при успехе
     */
    void logInFinished(const NetworkResult &res, const QString &accToken = "", const QString &refToken = "");
    
    /**
     * Сигнал о завершении выхода из аккаунта.
     * \param res результат выполнения запроса
     */
    void logOutFinished(const NetworkResult &res);
    
    /**
     * Сигнал о завершении обновления токена.
     * \param res результат выполнения запроса
     * \param accToken новый access токен при успехе
     * \param refToken новый refresh токен при успехе
     */
    void refreshAccessTokenFinished(const NetworkResult &res, const QString &accToken = "", const QString &refToken = "");

    // --- Сигналы начала процессов (например, для блокировки кнопок) ---
    
    void registrationInProgress();        //!< Сигнал о начале процесса регистрации
    void logInProgress();                 //!< Сигнал о начале процесса авторизации
    void logOutInProgress();              //!< Сигнал о начале процесса выхода из аккаунта
    void refreshAccessTokenInProgress();  //!< Сигнал о начале процесса обновления токена

private:
    // --- Внутренние сервисы ---
    AuthService *authService;             //!< Сервис для работы с API авторизации

    // --- Внутренние методы валидации ---
    
    /**
     * Локально валидирует введенные данные для регистрации перед отправкой запроса.
     * \param login введенный логин
     * \param password введенный пароль
     * \param passwordConfirm подтверждение пароля
     * \return результат локальной валидации
     */
    NetworkResult validateRegistration(const QString &login, const QString &password, const QString &passwordConfirm);
    
    /**
     * Локально валидирует введенные данные для авторизации перед отправкой запроса.
     * \param login введенный логин
     * \param password введенный пароль
     * \return результат локальной валидации
     */
    NetworkResult validateLogIn(const QString &login, const QString &password);
};

#endif // AUTHCONTROLLER_H
