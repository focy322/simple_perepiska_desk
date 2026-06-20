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
    void requestDownloadFileInfo(const QString &accessToken, const std::vector<quint64> &fileIds);

private:
    FileService *fileService;

signals:
    void uploadFileInProgress();
    void uploadFileFinished(const NetworkResult &res, const QString &filePath, const qulonglong &chatId, const ParsedUploadedFileInfo &fileInfo = {});
    void downloadFileInfoInProgress();
    void downloadFileInfoFinished(const NetworkResult &res, const ParsedDownloadedFileInfo& fileInfo = {});
    void downloadFileInProgress();
    void downloadFileFinished(const NetworkResult &res, const ParsedDownloadedFileInfo &fileInfo = {});

private slots:
    void on_downloadFileInfoFinished(const NetworkResult &res, const ParsedDownloadedFileInfo &fileInfo = {});
};

#endif // FILESCONTROLLER_H
