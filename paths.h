#ifndef PATHS_H
#define PATHS_H
#include <QString>
#include <QStandardPaths>


inline const QString downloadsDir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
inline const QString appDownloadsDir = downloadsDir + "/Vent Downloads";


#endif // PATHS_H
