#ifndef ENDPOINTS_H
#define ENDPOINTS_H

#include <QString>

#ifdef QT_DEBUG

inline const QString baseHttpUrl = "http://localhost:8000";
inline const QString baseWebsocketUrl = "ws://localhost:8000";

#endif

#ifndef QT_DEBUG

inline const QString baseHttpUrl = "https://messenger-3yfw.onrender.com";
inline const QString baseWebsocketUrl = "wss://messenger-3yfw.onrender.com";

#endif


#endif // ENDPOINTS_H
