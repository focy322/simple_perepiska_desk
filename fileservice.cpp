#include "fileservice.h"
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QHttpMultiPart>
#include <QFile>
#include <QFileInfo>

FileService::FileService(QObject *parent)
    : QObject{parent}
    , network(new QNetworkAccessManager(this))
    , baseUrl(baseHttpUrl)
    , uploadFileUrl("/api/files/")
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
                NetworkResult res{false, ERROR_TYPES::UNKNOWN_ERROR, generateMessageForError(ERROR_TYPES::UNKNOWN_ERROR)};
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
            NetworkResult res{false, ERROR_TYPES::UNKNOWN_ERROR, generateMessageForError(ERROR_TYPES::UNKNOWN_ERROR)};
            emit uploadFileFinished(res, filePath, chatId);
            reply->deleteLater();
        });
    }

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
