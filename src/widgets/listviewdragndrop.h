#ifndef LISTVIEWDRAGNDROP_H
#define LISTVIEWDRAGNDROP_H

#include "qtimer.h"
#include <QListView>
#include <QDragEnterEvent>
#include <QMouseEvent>
#include <QMimeData>
#include <QDropEvent>
#include <QFileInfo>
#include <climits>
#include <QHash>
#include <QSet>

/**
 * \brief Пользовательский QListView с поддержкой Drag & Drop для файлов.
 *
 * Используется для списка сообщений чата, позволяет перетаскивать файлы прямо в окно сообщений.
 * Также отслеживает прокрутку списка для отметки сообщений как прочитанных.
 */
class ListViewDragNDrop : public QListView
{
    Q_OBJECT

public:
    explicit ListViewDragNDrop(QWidget *parent = nullptr);

    /**
     * \brief Устанавливает ID текущего активного чата.
     *
     * \param chatId Идентификатор чата.
     */
    void setCurrentChatId(unsigned long long chatId) { currentChatId = chatId; }

    /**
     * \brief Возвращает пути к файлам, подготовленным для отправки в указанном чате.
     *
     * \param chatId Идентификатор чата.
     * \return Набор путей к файлам.
     */
    QSet<QString> getFilePaths(unsigned long long chatId) const;

    /**
     * \brief Очищает список подготовленных файлов для указанного чата.
     *
     * \param chatId Идентификатор чата.
     */
    void clearFilePaths(unsigned long long chatId);

    /**
     * \brief Очищает списки подготовленных файлов для всех чатов.
     */
    void clearAllFilePaths();

    /**
     * \brief Добавляет файлы в очередь на отправку.
     *
     * \param chatId Идентификатор чата.
     * \param paths Набор путей к файлам.
     */
    void addPendingFiles(unsigned long long chatId, const QSet<QString> &paths);

    /**
     * \brief Проверяет, есть ли файлы, ожидающие отправки в указанном чате.
     *
     * \param chatId Идентификатор чата.
     * \return true, если есть файлы.
     */
    bool hasPendingFiles(unsigned long long chatId) const;

    /**
     * \brief Удаляет конкретный файл из очереди на отправку.
     *
     * \param chatId Идентификатор чата.
     * \param filePath Путь к файлу.
     */
    void removeFileByPath(unsigned long long chatId, const QString &filePath);

protected:
    // --- События отрисовки и ввода ---

    /// \brief Переопределенный обработчик входа перетаскиваемого объекта.
    void dragEnterEvent(QDragEnterEvent *event) override;
    
    /// \brief Переопределенный обработчик перемещения перетаскиваемого объекта.
    void dragMoveEvent(QDragMoveEvent *event) override;
    
    /// \brief Переопределенный обработчик сброса перетаскиваемого объекта.
    void dropEvent(QDropEvent *event) override;
    
    /// \brief Переопределенный обработчик события отрисовки.
    void paintEvent(QPaintEvent *event) override;
    
    /// \brief Переопределенный обработчик перемещения мыши.
    void mouseMoveEvent(QMouseEvent *event) override;
    
    /// \brief Переопределенный обработчик покидания виджета.
    void leaveEvent(QEvent *event) override;

private:
    // --- Внутренние методы ---

    /**
     * \brief Обработчик остановки прокрутки списка (для вычисления прочитанных сообщений).
     */
    void on_scrollStop();

    /**
     * \brief Запоминает последнее прочитанное сообщение.
     *
     * \param chatId Идентификатор чата.
     * \param messageId Идентификатор сообщения.
     */
    void setLastReadMessage(const quint64 chatId, const quint64 messageId);

    // --- Внутренние переменные ---
    QHash<unsigned long long, QSet<QString>> filePathsByChat;                     //!< Хранилище путей файлов по чатам
    unsigned long long                       currentChatId = ULONG_LONG_MAX;      //!< ID текущего открытого чата

    QTimer                                  *scrollStopTimer;                     //!< Таймер для определения конца прокрутки
    inline static constexpr uint             SCROLL_STOP_TIMER_INTERVAL = 500;    //!< Интервал таймера остановки прокрутки (мс)

    std::pair<quint64, quint64>              lastReadMessage_;                    //!< Последнее прочитанное сообщение (ChatId, MessageId)
    std::pair<quint64, quint64>              lastSentReadMessage_{ULONG_LONG_MAX, ULONG_LONG_MAX}; //!< Последнее отправленное на сервер прочитанное сообщение

signals:
    // --- Сигналы ---

    /**
     * \brief Сигнал о том, что файлы были успешно перетащены в окно.
     */
    void gotDragNDropFiles();

    /**
     * \brief Сигнал запроса на отметку сообщения прочитанным.
     *
     * \param message Пара из ID чата и ID сообщения.
     */
    void needReadLastMessage(const std::pair<quint64, quint64> &message);
};

#endif // LISTVIEWDRAGNDROP_H
