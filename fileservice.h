#ifndef FILESERVICE_H
#define FILESERVICE_H

#include <QObject>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include "errortypes.h"
#include "endpoints.h"


struct ParsedUploadedFileInfo
{
    QString filename;
    QString contentType;
    QString uploadedAt;
    qulonglong fileId = ULONG_LONG_MAX;
    qulonglong fileSize = ULONG_LONG_MAX;
};

class FileService : public QObject
{
    Q_OBJECT
public:
    explicit FileService(QObject *parent = nullptr);
    void uploadFile(const QString &accessToken, const QSet<QString> &filePaths, const unsigned long long &chatId);
    const ParsedUploadedFileInfo parseUploadedFileInfo(const QJsonDocument &doc);

private:
    QNetworkAccessManager *network;
    QString baseUrl;
    QString uploadFileUrl;

signals:
    void uploadFileInProgress();
    void uploadFileFinished(const NetworkResult &res, const QString &filePath, const qulonglong &chatId, const ParsedUploadedFileInfo& fileInfo = {});
};

#endif // FILESERVICE_H
