#ifndef CHATLISTITEMDELEGATE_H
#define CHATLISTITEMDELEGATE_H

#include <QStyledItemDelegate>

#include <QMap>
#include <QTimer>

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

private:
    mutable QMap<qulonglong, qreal> m_avatarOpacities;
    mutable QMap<qulonglong, qreal> m_targetOpacities;
    mutable QTimer *m_animationTimer = nullptr;
};

#endif // CHATLISTITEMDELEGATE_H
