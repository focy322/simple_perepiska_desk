#ifndef SEARCHITEMDELEGATE_H
#define SEARCHITEMDELEGATE_H

#include <QStyledItemDelegate>

/**
 * Делегат для пользовательской отрисовки элементов списка при поиске пользователей.
 * Отвечает за внешний вид найденных пользователей (аватарка, имя пользователя, логин).
 */
class SearchItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit SearchItemDelegate(QObject *parent = nullptr);

    /**
     * Отрисовывает элемент списка результатов поиска.
     * \param painter объект для рисования
     * \param option параметры стиля и состояния элемента
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

#endif // SEARCHITEMDELEGATE_H