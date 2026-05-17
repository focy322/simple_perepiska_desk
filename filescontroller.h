#ifndef FILESCONTROLLER_H
#define FILESCONTROLLER_H

#include <QObject>
#include "fileservice.h"

class FilesController : public QObject
{
    Q_OBJECT
public:
    explicit FilesController(QObject *parent = nullptr);
    void requestUploadFile(const QString &accessToken, const QSet<QString> &filePaths, const unsigned long long &chatId);

private:
    FileService *fileService;

signals:
    void uploadFileInProgress();
    void uploadFileFinished(const NetworkResult &res, const QString &filePath, const qulonglong &chatId, const ParsedUploadedFileInfo& fileInfo = {});
};

#endif // FILESCONTROLLER_H
