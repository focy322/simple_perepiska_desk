#ifndef AVATARHELPER_H
#define AVATARHELPER_H

#include <QPixmap>
#include <QString>
#include <QColor>

class AvatarHelper {
public:
    static QPixmap generatePlaceholder(const QString &name, int size);
    static QPixmap makeRoundImage(const QPixmap &src, int size);
    static QColor getColdColor(const QString &name);
};

#endif // AVATARHELPER_H
