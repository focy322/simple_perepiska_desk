#ifndef AUTHCONTROLLER_H
#define AUTHCONTROLLER_H

#include <QObject>
#include "services/authservice.h"

// Принимает запросы от UI, дергает AuthService, возвращает результат через сигналы.
class AuthController : public QObject
{
    Q_OBJECT
public:
    explicit AuthController(QObject *parent = nullptr);

    /**
      * Выполняет запрос на регисатрацию через AuthService и посылает сигнал registrationFinished
      * с AuthResult в качестве входного параметра полученным от authService
      * @param login - Введенный логин
      * @param password - Введенный пароль
      * @param passwordConfirm - Введенное подтверждение пароля
      * @return
      */
    void requestRegistration(const QString &login, const QString &password, const QString &passwordConfirm);


    /**
      * Выполняет запрос на авторизацию через AuthService и посылает сигнал logInFinished
      * с AuthResult в качестве входного параметра полученным от authService
      * @param login - Введенный логин
      * @param password - Введенный пароль
      * @return
      */
    void requestLogIn(const QString &login, const QString &password);

    /**
      * Выполняет запрос на выход из аккаунта через AuthService (Пока его нет)
      * @param
      * @return
      */
    void requestLogOut(const QString &accToken, const QString &refToken);

    void requestRefreshAccessToken(const QString &refToken);

private:
    AuthService *authService; // Содержит правила регистрации/входа и работу с данными пользователей, без UI-зависимостей.
    NetworkResult validateRegistration(const QString &login, const QString &password, const QString &passwordConfirm);
    NetworkResult validateLogIn(const QString &login, const QString &password);

signals:
    // Сигналы прокидываются от AuthService в MainWindow
    void registrationFinished(const NetworkResult &res, const QString &accToken = "", const QString &refToken = ""); //!< Сигнал о завершении регистрации (может быть как успешным так и нет)
    void logInFinished(const NetworkResult &res, const QString &accToken = "", const QString &refToken = "");        //!< Сигнал о завершении авторизации (может быть как успешным так и нет)
    void logOutFinished(const NetworkResult &res);       //!< Сигнал о завершении выхода из аккаунта (может быть как успешным так и нет)
    void registrationInProgress();                    //!< Сигнал о том что регистрация в процессе и нужно заморозить кнопки
    void logInProgress();                             //!< Сигнал о том что авторизация в процессе и нужно заморозить кнопки
    void logOutInProgress();                          //!< Сигнал о том что выход из аккаунта в процессе и нужно заморозить кнопки
    void refreshAccessTokenInProgress();
    void refreshAccessTokenFinished(const NetworkResult &res, const QString &accToken = "", const QString &refToken = "");
};

#endif // AUTHCONTROLLER_H
