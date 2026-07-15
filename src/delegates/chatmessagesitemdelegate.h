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
    bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index) override;

    const std::pair<quint64, quint64>& getLastReadMessage() { return lastReadMessage; };

signals:
    void editMessageRequested(quint64 messageId, const QString &currentText);
    void deleteMessageRequested(quint64 messageId);

private:
    void setLastReadMessage(const quint64 chatId, const quint64 MessageId) const;
    unsigned long long m_currentUserId;
    QString appDownloadsDir;
    mutable std::pair<quint64, quint64> lastReadMessage;  //!< Последнее прочитанное сообщение ChatId | MessageId
    mutable QHash<quint64, QRect> m_editBtnRects;
    mutable QHash<quint64, QRect> m_deleteBtnRects;
};

#endif // CHATMESSAGESITEMDELEGATE_H
