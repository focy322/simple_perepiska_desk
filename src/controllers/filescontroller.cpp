#include "controllers/filescontroller.h"

FilesController::FilesController(QObject *parent)
    : QObject{parent}
    , fileService(new FileService(this))

{
    connect(fileService, &FileService::uploadFileInProgress, this, &FilesController::uploadFileInProgress);
    connect(fileService, &FileService::uploadFileFinished, this, &FilesController::uploadFileFinished);
    connect(fileService, &FileService::downloadFileInfoInProgress, this, &FilesController::downloadFileInfoInProgress);
    connect(fileService, &FileService::downloadFileInfoFinished, this, &FilesController::on_downloadFileInfoFinished);
    connect(fileService, &FileService::downloadFileInfoFinished, this, &FilesController::downloadFileInfoFinished);
    connect(fileService, &FileService::downloadFileInProgress, this, &FilesController::downloadFileInProgress);
    connect(fileService, &FileService::downloadFileFinished, this, &FilesController::downloadFileFinished);
}

void FilesController::requestUploadFile(const QString &accessToken, const QSet<QString> &filePaths, const unsigned long long &chatId)
{
    fileService->uploadFile(accessToken, filePaths, chatId);
}

void FilesController::requestDownloadFileInfo(const QString &accessToken, const std::vector<quint64> &fileIds)
{
    fileService->downloadFileInfo(accessToken, fileIds);
}

void FilesController::on_downloadFileInfoFinished(const NetworkResult &res, const ParsedDownloadedFileInfo &fileInfo)
{
    if (res.ok && !fileInfo.downloadUrl.isEmpty())
    {
        fileService->downloadFile(fileInfo);
    }
}
