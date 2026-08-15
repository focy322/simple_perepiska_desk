#ifndef FILESCONTROLLER_H
#define FILESCONTROLLER_H

#include <QObject>

#include "base_controller.h"
#include "services/fileservice.h"

/**
 * Контроллер для работы с файлами.
 * Принимает запросы от UI, вызывает методы FileService (для загрузки и скачивания файлов) и возвращает результат через сигналы.
 */
class FilesController : public BaseController
{
    Q_OBJECT
public:
    explicit FilesController(QObject *parent = nullptr);

    /**
     * Запрашивает загрузку файлов на сервер.
     * \param accessToken токен доступа (Access Token) для авторизации запроса
     * \param filePaths пути к локальным файлам для загрузки
     * \param chatId идентификатор чата, в который отправляются файлы
     */
    void requestUploadFile(const QString &accessToken, const QSet<QString> &filePaths, const unsigned long long &chatId);

    /**
     * Запрашивает информацию о файлах для последующего скачивания.
     * \param accessToken токен доступа (Access Token) для авторизации запроса
     * \param fileIds список идентификаторов файлов
     */
    void requestDownloadFileInfo(const QString &accessToken, const std::vector<quint64> &fileIds);

signals:
    // --- Сигналы процессов ---
    
    void uploadFileInProgress();          //!< Сигнал о начале загрузки файла на сервер
    void downloadFileInfoInProgress();    //!< Сигнал о начале получения информации о файле
    void downloadFileInProgress();        //!< Сигнал о начале скачивания файла с сервера

    // --- Сигналы завершения запросов ---

    /**
     * Сигнал об окончании загрузки файла на сервер.
     * \param res результат выполнения запроса
     * \param filePath локальный путь загруженного файла
     * \param chatId идентификатор чата
     * \param fileInfo информация о загруженном файле
     */
    void uploadFileFinished(const NetworkResult &res, const QString &filePath, const qulonglong &chatId, const ParsedUploadedFileInfo &fileInfo = {});

    /**
     * Сигнал об окончании получения информации о файле.
     * \param res результат выполнения запроса
     * \param fileInfo метаданные файла (для начала скачивания)
     */
    void downloadFileInfoFinished(const NetworkResult &res, const ParsedDownloadedFileInfo& fileInfo = {});

    /**
     * Сигнал об окончании скачивания файла с сервера.
     * \param res результат выполнения запроса
     * \param fileInfo информация о скачанном файле
     */
    void downloadFileFinished(const NetworkResult &res, const ParsedDownloadedFileInfo &fileInfo = {});

private slots:
    // --- Внутренние обработчики ---

    /**
     * Внутренний слот-обработчик завершения получения информации о файле.
     * Инициирует процесс скачивания самого файла через FileService.
     * \param res результат выполнения запроса
     * \param fileInfo полученные метаданные файла
     */
    void on_downloadFileInfoFinished(const NetworkResult &res, const ParsedDownloadedFileInfo &fileInfo = {});
    void on_uploadFileFinished(const NetworkResult &res, const QString &filePath, const qulonglong &chatId, const ParsedUploadedFileInfo &fileInfo = {});
    void on_downloadFileFinished(const NetworkResult &res, const ParsedDownloadedFileInfo &fileInfo = {});



private:
    // --- Внутренние сервисы ---
    FileService *fileService;             //!< Сервис для работы с загрузкой и скачиванием файлов
};

#endif // FILESCONTROLLER_H
