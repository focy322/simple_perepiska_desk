#ifndef USERINFOSERVICE_H
#define USERINFOSERVICE_H

#include <QObject>
#include "utils/errortypes.h"
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "utils/endpoints.h"

/**
 * Структура для хранения распарсенных данных о найденном пользователе.
 */
struct ParsedFoundUsersObject
{
    QString            username;                          //!< Логин пользователя
    QString            nickname;                          //!< Никнейм (отображаемое имя)
    QString            lastSeen;                          //!< Временная метка последней активности
    QString            avatarFileUrl;                     //!< Ссылка на аватар
    unsigned long long userId = ULONG_LONG_MAX;           //!< Идентификатор пользователя
};

/**
 * Сервис для взаимодействия с API информации о пользователях.
 * Отвечает за получение данных о себе и других пользователях, загрузку аватара и поиск.
 */
class UserInfoService : public QObject
{
    Q_OBJECT
public:
    explicit UserInfoService(QObject *parent = nullptr);

    /**
     * Выполняет сетевой запрос на получение информации о текущем авторизованном пользователе.
     * \param accToken токен доступа (Access Token)
     */
    void getMyUserInfo(const QString &accToken);

    /**
     * Выполняет сетевой запрос на получение информации о другом пользователе по его ID.
     * \param accToken токен доступа (Access Token)
     * \param userId идентификатор пользователя
     */
    void getUserInfo(const QString &accToken, unsigned long long userId);

    /**
     * Выполняет сетевой запрос на обновление аватара текущего пользователя.
     * \param accToken токен доступа (Access Token)
     * \param imageData бинарные данные нового аватара
     */
    void uploadAvatar(const QString &accToken, const QByteArray &imageData);

    /**
     * Выполняет сетевой запрос на поиск пользователей по строке.
     * \param accToken токен доступа (Access Token)
     * \param input строка поиска (имя или логин)
     */
    void findUser(const QString &accToken, const QString &input);

    /**
     * Разбирает JSON-документ со списком найденных пользователей в вектор структур ParsedFoundUsersObject.
     * \param doc JSON документ от API
     * \return список найденных пользователей
     */
    const std::vector<ParsedFoundUsersObject> parseFoundUsersArray(const QJsonDocument &doc);

signals:
    // --- Сигналы процессов ---
    
    /**
     * Сигнал о начале загрузки информации о текущем пользователе.
     */
    void getMyUserInfoInProgress();

    /**
     * Сигнал о начале поиска пользователей.
     */
    void findUserInProgress();

    // --- Сигналы завершения запросов ---

    /**
     * Сигнал об окончании загрузки информации о текущем пользователе.
     * \param res результат выполнения запроса
     * \param username имя пользователя
     * \param userId идентификатор пользователя
     * \param avatarUrl ссылка на аватар пользователя
     */
    void getMyUserInfoFinished(const NetworkResult &res, const QString &username = "", unsigned long long userId = ULONG_LONG_MAX, const QString &avatarUrl = "");

    /**
     * Сигнал об окончании загрузки информации о запрашиваемом пользователе.
     * \param res результат выполнения запроса
     * \param user объект с данными найденного пользователя
     */
    void getUserInfoFinished(const NetworkResult &res, const ParsedFoundUsersObject &user = {});

    /**
     * Сигнал об окончании загрузки нового аватара.
     * \param res результат выполнения запроса
     * \param avatarUrl новая ссылка на загруженный аватар
     */
    void uploadAvatarFinished(const NetworkResult &res, const QString &avatarUrl = "");

    /**
     * Сигнал об окончании поиска пользователей.
     * \param res результат выполнения запроса
     * \param paObjects список найденных пользователей
     * \param input строка поиска
     */
    void findUserFinished(const NetworkResult &res, const std::vector<ParsedFoundUsersObject>& paObjects = {}, const QString &input = "" );

private:
    // --- Внутренние объекты сети ---
    QNetworkAccessManager *network;                       //!< Менеджер сети для выполнения HTTP-запросов

    // --- Адреса API (Endpoints) ---
    QString                baseUrl;                       //!< Базовый адрес API
    QString                myUserInfoUrl;                 //!< Путь API для получения информации о себе
    QString                userByUsernameUrl;             //!< Зарезервированный путь API для получения информации о пользователе по логину (не используется)
    QString                findUserUrl;                   //!< Путь API для поиска пользователей
};

#endif // USERINFOSERVICE_H
