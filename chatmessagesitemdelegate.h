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
    const std::pair<quint64, quint64>& getLastReadMessage() { return lastReadMessage; };

private:
    void setLastReadMessage(const quint64 chatId, const quint64 MessageId) const;
    unsigned long long m_currentUserId;
    mutable std::pair<quint64, quint64> lastReadMessage;  //!< Последнее прочитанное сообщение ChatId | MessageId
};

#endif // CHATMESSAGESITEMDELEGATE_H
