#ifndef ENDPOINTS_H
#define ENDPOINTS_H

#include <QString>

#ifdef QT_DEBUG

inline const QString baseHttpUrl      = "http://localhost:8000"; //!< Базовый URL для HTTP запросов (отладка)
inline const QString baseWebsocketUrl = "ws://localhost:8000";   //!< Базовый URL для WebSocket соединений (отладка)

#endif

#ifndef QT_DEBUG

inline const QString baseHttpUrl      = "https://messenger-3yfw.onrender.com"; //!< Базовый URL для HTTP запросов (релиз)
inline const QString baseWebsocketUrl = "wss://messenger-3yfw.onrender.com";   //!< Базовый URL для WebSocket соединений (релиз)

#endif

// --- Auth Service Endpoints ---
inline const QString registerUrl("/api/users/");                       //!< URL для регистрации нового пользователя
inline const QString logInUrl("/api/auth/token");                      //!< URL для авторизации пользователя
inline const QString refreshAccessTokenUrl("/api/auth/token/refresh"); //!< URL для обновления access токена
inline const QString logOutUrl("/api/auth/token/revoke");              //!< URL для выхода из аккаунта

// --- Chat Service Endpoints ---
inline const QString myChatsUrl("/api/chats/");                     //!< URL для получения списка чатов
inline const QString chatMessagesUrl("/api/chats/%1");              //!< URL для получения сообщений чата
inline const QString createDirectChatUrl("/api/chats/create");      //!< URL для создания прямого чата
inline const QString markMessageReadUrl("/api/chats/%1/mark-read"); //!< URL для пометки сообщения как прочитанного
inline const QString deleteMessagesUrl("/api/messages/");           //!< URL для удаления сообщений
inline const QString editMessageUrl("/api/messages/%1");            //!< URL для редактирования сообщения

// --- File Service Endpoints ---
inline const QString uploadFileUrl("/api/files/");        //!< URL для загрузки файла
inline const QString downloadFileUrl("/api/files/%1");    //!< URL для скачивания файла

// --- User Info Service Endpoints ---
inline const QString myUserInfoUrl("/api/users/me");          //!< URL для получения информации о бо мне
inline const QString findUserUrl("/api/users/search");        //!< URL для поиска пользователя по имени
inline const QString uploadAvatarUrl("/api/users/me/avatar"); //!< URL для загрузки аватара пользователя
inline const QString getUserInfoUrl("/api/users/%1");         //!< URL для получения информации о пользователе по userID

// --- WebSocket Service Endpoints ---
inline const QString webSocketUrl("/ws/"); //!< URL для подключения к WebSocket

#endif // ENDPOINTS_H
