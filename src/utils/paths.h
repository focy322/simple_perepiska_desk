#ifndef PATHS_H
#define PATHS_H
#include <QString>
#include <QStandardPaths>


inline const QString downloadsDir    = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation); //!< Стандартная директория загрузок ОС
inline const QString appDownloadsDir = downloadsDir + "/Vent Downloads";                                   //!< Директория загрузок приложения


#endif // PATHS_H
