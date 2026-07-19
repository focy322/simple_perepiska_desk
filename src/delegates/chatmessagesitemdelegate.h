#ifndef CHATMESSAGESITEMDELEGATE_H
#define CHATMESSAGESITEMDELEGATE_H

#include <QStyledItemDelegate>

/**
 * Делегат для пользовательской отрисовки элементов списка сообщений чата.
 * Отвечает за выравнивание своих и чужих сообщений, отображение вложений, времени, статуса прочтения и кнопок действий (редактировать/удалить).
 */
class ChatMessagesItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit ChatMessagesItemDelegate(QObject *parent = nullptr);

    /**
     * Устанавливает идентификатор текущего авторизованного пользователя.
     * Используется для определения выравнивания (свои сообщения справа, чужие слева).
     * \param userId идентификатор текущего пользователя
     */
    void setCurrentUserId(unsigned long long userId);

    /**
     * Отрисовывает элемент сообщения.
     * \param painter объект для рисования
     * \param option параметры стиля
     * \param index индекс элемента в модели данных
     */
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    /**
     * Возвращает рекомендуемый размер элемента сообщения, учитывая текст и вложения.
     * \param option параметры стиля
     * \param index индекс элемента в модели данных
     * \return размер элемента
     */
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    /**
     * Обрабатывает события мыши в области элемента списка (например, клики по кнопкам или вложениям).
     * \param event событие ввода (клик, наведение и т.д.)
     * \param model модель данных
     * \param option параметры стиля
     * \param index индекс элемента
     * \return true, если событие обработано
     */
    bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index) override;

    /**
     * Возвращает информацию о последнем прочитанном сообщении (пара chatId и messageId).
     * \return пара идентификаторов
     */
    const std::pair<quint64, quint64>& getLastReadMessage() { return lastReadMessage; };

    /**
     * Сбрасывает состояние последнего прочитанного сообщения.
     */
    void resetLastReadMessage() const { lastReadMessage = {ULONG_LONG_MAX, ULONG_LONG_MAX}; }

signals:
    // --- Сигналы действий пользователя ---

    /**
     * Сигнал запроса на редактирование сообщения (после клика по кнопке "Изменить").
     * \param messageId идентификатор сообщения
     * \param currentText текущий текст сообщения для подстановки в поле ввода
     */
    void editMessageRequested(quint64 messageId, const QString &currentText);

    /**
     * Сигнал запроса на удаление сообщения (после клика по кнопке "Удалить").
     * \param messageId идентификатор сообщения
     */
    void deleteMessageRequested(quint64 messageId);

private:
    // --- Внутренние методы ---

    /**
     * Обновляет состояние последнего прочитанного сообщения во время отрисовки.
     * \param chatId идентификатор чата
     * \param MessageId идентификатор сообщения
     */
    void setLastReadMessage(const quint64 chatId, const quint64 MessageId) const;

    // --- Внутренние переменные и состояние ---
    unsigned long long m_currentUserId;                   //!< Идентификатор текущего пользователя
    QString appDownloadsDir;                              //!< Путь к локальной папке загрузок приложения

    mutable std::pair<quint64, quint64> lastReadMessage;  //!< Пара (ChatId, MessageId) последнего прочитанного сообщения
    mutable QHash<quint64, QRect> m_editBtnRects;         //!< Хранилище областей кнопок "Редактировать" по messageId
    mutable QHash<quint64, QRect> m_deleteBtnRects;       //!< Хранилище областей кнопок "Удалить" по messageId
};

#endif // CHATMESSAGESITEMDELEGATE_H
