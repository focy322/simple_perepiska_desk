#include "services/fileservice.h"
#include "utils/paths.h"

#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QHttpMultiPart>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QDir>

FileService::FileService(QObject *parent)
    : QObject{parent}
    , network(new QNetworkAccessManager(this))
    , baseUrl(baseHttpUrl)
    , uploadFileUrl("/api/files/")
    , downloadFileUrl("/api/files/%1")
{}

void FileService::uploadFile(const QString &accessToken, const QSet<QString> &filePaths, const unsigned long long &chatId)
{
    emit uploadFileInProgress();
    QUrl url (baseUrl + uploadFileUrl);
    QUrlQuery query;
    query.addQueryItem("chat_id", QString::number(chatId));
    url.setQuery(query);
    QNetworkRequest req(url);
    req.setRawHeader("Authorization", "Bearer " + accessToken.toUtf8());

    for (const auto &filePath : std::as_const(filePaths))
    {
        QFile *file = new QFile(filePath);
        if (!file->open(QIODevice::ReadOnly))
        {
            NetworkResult res{false, ERROR_TYPES::UNKNOWN_ERROR, QString("Failed to open file: %1").arg(filePath)};
            emit uploadFileFinished(res, filePath, chatId);
            file->deleteLater();
            continue;
        }

        QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
        QHttpPart filePart;
        const QString fileName = QFileInfo(filePath).fileName();
        const QString disposition = QString("form-data; name=\"file\"; filename=\"%1\"").arg(fileName);
        filePart.setHeader(QNetworkRequest::ContentDispositionHeader, disposition);
        filePart.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");
        filePart.setBodyDevice(file);
        file->setParent(multiPart);
        multiPart->append(filePart);

        QNetworkReply *reply = network->post(req, multiPart);
        multiPart->setParent(reply);

        connect(reply, &QNetworkReply::finished, this, [this, reply, filePath, chatId]()
        {
            auto httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (reply->error() != QNetworkReply::NoError && httpCode == 0)
            {
                NetworkResult res{false, ERROR_TYPES::UNKNOWN_ERROR, reply->errorString()};
                emit uploadFileFinished(res, filePath, chatId);
                reply->deleteLater();
                return;
            }
            QByteArray raw = reply->readAll();
            QJsonParseError pe;
            QJsonDocument doc = QJsonDocument::fromJson(raw, &pe);
            if (pe.error || !doc.isObject())
            {
                NetworkResult res{false, ERROR_TYPES::UNKNOWN_ERROR, QString("JSON/HTTP Error %1: %2").arg(httpCode).arg(QString(raw))};
                emit uploadFileFinished(res, filePath, chatId);
                reply->deleteLater();
                return;
            }
            if (httpCode == 200 || httpCode == 201)
            {
                const auto fInfo = parseUploadedFileInfo(doc);

                NetworkResult res{true, ERROR_TYPES::NO_ERROR, generateMessageForError(ERROR_TYPES::NO_ERROR)};
                emit uploadFileFinished(res, filePath, chatId, fInfo);
                reply->deleteLater();
                return;
            }
            NetworkResult res{false, ERROR_TYPES::UNKNOWN_ERROR, QString("HTTP Error %1: %2").arg(httpCode).arg(QString(raw))};
            emit uploadFileFinished(res, filePath, chatId);
            reply->deleteLater();
        });
    }

}

void FileService::downloadFileInfo(const QString &accessToken, const std::vector<quint64> &fileIds)
{
    emit downloadFileInfoInProgress();
    for (int id : fileIds)
    {
        QUrl url(baseUrl + downloadFileUrl.arg(id));
        QNetworkRequest req(url);
        req.setRawHeader("Authorization", "Bearer " + accessToken.toUtf8());
        QNetworkReply *reply = network->get(req);
        connect(reply, &QNetworkReply::finished, this, [this, reply](){
            auto httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (reply->error() != QNetworkReply::NoError && httpCode == 0)
            {
                NetworkResult res{false, ERROR_TYPES::UNKNOWN_ERROR, reply->errorString()};
                emit downloadFileInfoFinished(res);
                reply->deleteLater();
                return;
            }
            QByteArray raw = reply->readAll();
            QJsonParseError pe;
            QJsonDocument doc = QJsonDocument::fromJson(raw, &pe);
            if (pe.error || !doc.isObject())
            {
                NetworkResult res{false, ERROR_TYPES::UNKNOWN_ERROR, generateMessageForError(ERROR_TYPES::UNKNOWN_ERROR)};
                emit downloadFileInfoFinished(res);
                reply->deleteLater();
                return;
            }
            if (httpCode == 200 || httpCode == 201)
            {
                const auto fInfo = parseDownloadedFileInfo(doc);

                NetworkResult res{true, ERROR_TYPES::NO_ERROR, generateMessageForError(ERROR_TYPES::NO_ERROR)};
                emit downloadFileInfoFinished(res, fInfo);
                reply->deleteLater();
                return;
            }
            NetworkResult res{false, ERROR_TYPES::UNKNOWN_ERROR, generateMessageForError(ERROR_TYPES::UNKNOWN_ERROR)};
            emit downloadFileInfoFinished(res);
            reply->deleteLater();
        });
    }
}

void FileService::downloadFile(const ParsedDownloadedFileInfo &fileInfo)
{
    emit downloadFileInProgress();
    QUrl url = fileInfo.downloadUrl;
    QNetworkRequest req(url);
    QNetworkReply *reply = network->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, fileInfo](){
        auto httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (reply->error() != QNetworkReply::NoError && httpCode == 0)
        {
            NetworkResult res{false, ERROR_TYPES::UNKNOWN_ERROR, reply->errorString()};
            emit downloadFileFinished(res);
            reply->deleteLater();
            return;
        }
        QByteArray raw = reply->readAll();
        //QDir().mkpath(appDownloadsDir); хз как будто лишнее как будто нет хз
        QString filePath = appDownloadsDir + "/" + fileInfo.filename;
        QSaveFile saveFile(filePath, this);
        if (saveFile.open(QIODevice::WriteOnly))
        {
            if (saveFile.write(raw) != fileInfo.fileSize)
            {
                NetworkResult res{false, ERROR_TYPES::UNKNOWN_ERROR, "Incorrect number of bytes written"};
                emit downloadFileFinished(res);
                reply->deleteLater();
                return;
            }

            if (!saveFile.commit())
            {
                NetworkResult res{false, ERROR_TYPES::UNKNOWN_ERROR, "Couldn't commit file"};
                emit downloadFileFinished(res);
                reply->deleteLater();
                return;
            }
            NetworkResult res{false, ERROR_TYPES::NO_ERROR, generateMessageForError(ERROR_TYPES::NO_ERROR)};
            emit downloadFileFinished(res, fileInfo);
            reply->deleteLater();
            return;
        }
        else
        {
            NetworkResult res{false, ERROR_TYPES::UNKNOWN_ERROR, "Couldn't save file"};
            emit downloadFileFinished(res);
            reply->deleteLater();
            return;
        }

    });

}

const ParsedDownloadedFileInfo FileService::parseDownloadedFileInfo(const QJsonDocument &doc)
{
    // Безопасное преобразование JSON-числа к unsigned long long.
    auto toUnsignedLongLong = [](const QJsonValue &value, unsigned long long defaultValue = ULONG_LONG_MAX) -> unsigned long long
    {
        if (!value.isDouble())
            return defaultValue;

        const double number = value.toDouble();
        if (number < 0 || std::floor(number) != number)
            return defaultValue;

        return static_cast<unsigned long long>(number);
    };

    QJsonObject obj = doc.object();
    ParsedDownloadedFileInfo fInfo;
    fInfo.fileId = toUnsignedLongLong(obj.value("file_id"));
    fInfo.filename = obj.value("filename").toString();
    fInfo.contentType = obj.value("content_type").toString();
    fInfo.fileSize = toUnsignedLongLong(obj.value("file_size"));
    fInfo.uploadedAt = obj.value("uploaded_at").toString();
    fInfo.downloadUrl = obj.value("download_url").toString();
    return fInfo;
}

const ParsedUploadedFileInfo FileService::parseUploadedFileInfo(const QJsonDocument &doc)
{
    // Безопасное преобразование JSON-числа к unsigned long long.
    auto toUnsignedLongLong = [](const QJsonValue &value, unsigned long long defaultValue = ULONG_LONG_MAX) -> unsigned long long
    {
        if (!value.isDouble())
            return defaultValue;

        const double number = value.toDouble();
        if (number < 0 || std::floor(number) != number)
            return defaultValue;

        return static_cast<unsigned long long>(number);
    };

    QJsonObject obj = doc.object();
    ParsedUploadedFileInfo fInfo;
    fInfo.fileId = toUnsignedLongLong(obj.value("file_id"));
    fInfo.filename = obj.value("filename").toString();
    fInfo.contentType = obj.value("content_type").toString();
    fInfo.fileSize = toUnsignedLongLong(obj.value("file_size"));
    fInfo.uploadedAt = obj.value("uploaded_at").toString();
    return fInfo;
}
