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


#endif // ENDPOINTS_H
