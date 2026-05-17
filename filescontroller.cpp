#include "filescontroller.h"

FilesController::FilesController(QObject *parent)
    : QObject{parent}
    , fileService(new FileService(this))

{
    connect(fileService, &FileService::uploadFileInProgress, this, &FilesController::uploadFileInProgress);
    connect(fileService, &FileService::uploadFileFinished, this, &FilesController::uploadFileFinished);
}

void FilesController::requestUploadFile(const QString &accessToken, const QSet<QString> &filePaths, const unsigned long long &chatId)
{
    fileService->uploadFile(accessToken, filePaths, chatId);
}
