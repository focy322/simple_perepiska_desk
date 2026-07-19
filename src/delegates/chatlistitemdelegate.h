#ifndef CHATLISTITEMDELEGATE_H
#define CHATLISTITEMDELEGATE_H

#include <QStyledItemDelegate>

/**
 * Делегат для пользовательской отрисовки элементов списка чатов.
 * Отвечает за внешний вид каждого отдельного чата в списке (аватарки, текст, непрочитанные сообщения).
 */
class ChatListItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit ChatListItemDelegate(QObject *parent = nullptr);

    /**
     * Отрисовывает элемент списка чатов.
     * \param painter объект для рисования
     * \param option параметры стиля и состояния элемента (например, выделен ли он)
     * \param index индекс элемента в модели данных
     */
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    /**
     * Возвращает рекомендуемый размер элемента списка.
     * \param option параметры стиля
     * \param index индекс элемента в модели данных
     * \return размер элемента
     */
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

#endif // CHATLISTITEMDELEGATE_H
