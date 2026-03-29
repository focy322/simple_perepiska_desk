#ifndef CHATMESSAGESITEMDELEGATE_H
#define CHATMESSAGESITEMDELEGATE_H

#include <QStyledItemDelegate>

class ChatMessagesItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit ChatMessagesItemDelegate(QObject *parent = nullptr);

    void setCurrentUserId(unsigned long long userId);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    unsigned long long m_currentUserId;
};

#endif // CHATMESSAGESITEMDELEGATE_H
