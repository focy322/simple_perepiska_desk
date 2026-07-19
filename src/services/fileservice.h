#ifndef FILESERVICE_H
#define FILESERVICE_H

#include <QObject>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include "utils/errortypes.h"
#include "utils/endpoints.h"

/**
 * Структура для хранения распарсенных данных об успешно загруженном файле на сервер.
 */
struct ParsedUploadedFileInfo
{
    QString    filename;                          //!< Имя загруженного файла
    QString    contentType;                       //!< MIME-тип содержимого
    QString    uploadedAt;                        //!< Временная метка загрузки
    qulonglong fileId = ULONG_LONG_MAX;           //!< Идентификатор файла на сервере
    qulonglong fileSize = ULONG_LONG_MAX;         //!< Размер файла в байтах
};

/**
 * Структура для хранения распарсенных данных (метаданных) о файле для его скачивания.
 */
struct ParsedDownloadedFileInfo
{
    QString    filename;                          //!< Имя файла
    QString    contentType;                       //!< MIME-тип содержимого
    QString    uploadedAt;                        //!< Временная метка загрузки файла
    QString    downloadUrl;                       //!< URL для скачивания
    qulonglong fileId = ULONG_LONG_MAX;           //!< Идентификатор файла на сервере
    qulonglong fileSize = ULONG_LONG_MAX;         //!< Размер файла в байтах
};

/**
 * Сервис для взаимодействия с API файлов.
 * Отвечает за загрузку файлов на сервер (upload) и скачивание файлов (download).
 */
class FileService : public QObject
{
    Q_OBJECT
public:
    explicit FileService(QObject *parent = nullptr);

    /**
     * Выполняет сетевой запрос на загрузку одного или нескольких файлов на сервер.
     * \param accessToken токен доступа (Access Token)
     * \param filePaths пути к локальным файлам
     * \param chatId идентификатор чата, куда отправляются файлы
     */
    void uploadFile(const QString &accessToken, const QSet<QString> &filePaths, const unsigned long long &chatId);

    /**
     * Выполняет сетевой запрос на получение метаданных файлов для последующего скачивания.
     * \param accessToken токен доступа (Access Token)
     * \param fileIds список идентификаторов запрашиваемых файлов
     */
    void downloadFileInfo(const QString &accessToken, const std::vector<quint64> &fileIds);

    /**
     * Выполняет скачивание самого файла (бинарных данных) по уже полученным метаданным.
     * \param fileInfo метаданные файла, содержащие URL для скачивания
     */
    void downloadFile(const ParsedDownloadedFileInfo &fileInfo);

    /**
     * Разбирает JSON-документ с информацией об успешно загруженном файле.
     * \param doc JSON документ от API
     * \return структура с информацией о загруженном файле
     */
    const ParsedUploadedFileInfo parseUploadedFileInfo(const QJsonDocument &doc);

    /**
     * Разбирает JSON-документ с информацией о файле, доступном для скачивания.
     * \param doc JSON документ от API
     * \return структура с метаданными файла
     */
    const ParsedDownloadedFileInfo parseDownloadedFileInfo(const QJsonDocument &doc);

signals:
    // --- Сигналы процессов ---
    
    /**
     * Сигнал о начале загрузки файла на сервер.
     */
    void uploadFileInProgress();

    /**
     * Сигнал о начале получения метаданных файла.
     */
    void downloadFileInfoInProgress();

    /**
     * Сигнал о начале скачивания бинарных данных файла.
     */
    void downloadFileInProgress();

    // --- Сигналы завершения запросов ---

    /**
     * Сигнал об окончании загрузки файла на сервер.
     * \param res результат выполнения запроса
     * \param filePath исходный путь загруженного файла
     * \param chatId идентификатор чата
     * \param fileInfo распарсенная информация о файле (ID, размер и т.д.)
     */
    void uploadFileFinished(const NetworkResult &res, const QString &filePath, const qulonglong &chatId, const ParsedUploadedFileInfo& fileInfo = {});

    /**
     * Сигнал об окончании получения метаданных файла.
     * \param res результат выполнения запроса
     * \param fileInfo распарсенные метаданные файла (включая downloadUrl)
     */
    void downloadFileInfoFinished(const NetworkResult &res, const ParsedDownloadedFileInfo& fileInfo = {});

    /**
     * Сигнал об окончании скачивания бинарных данных файла.
     * \param res результат выполнения запроса
     * \param fileInfo метаданные скачанного файла
     */
    void downloadFileFinished(const NetworkResult &res, const ParsedDownloadedFileInfo &fileInfo = {});

private:
    // --- Внутренние объекты сети ---
    QNetworkAccessManager *network;                       //!< Менеджер сети для выполнения HTTP-запросов

    // --- Адреса API (Endpoints) ---
    QString                baseUrl;                       //!< Базовый адрес API
    QString                uploadFileUrl;                 //!< Путь API для загрузки файла на сервер
    QString                downloadFileUrl;               //!< Путь API для получения информации о файле
};

#endif // FILESERVICE_H
