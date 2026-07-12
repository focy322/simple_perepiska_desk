#include "avatarhelper.h"
#include <QPainter>
#include <QFont>
#include <QCryptographicHash>
#include <QPainterPath>

QColor AvatarHelper::getColdColor(const QString &name) {
    // Генерируем хэш, чтобы выбрать постоянный цвет для одного и того же имени
    QByteArray hash = QCryptographicHash::hash(name.toUtf8(), QCryptographicHash::Md5);
    int h = static_cast<unsigned char>(hash[0]); // от 0 до 255
    // Оттенок холодных цветов примерно между 180 и 300 (от голубого до пурпурного/синего)
    int hue = 180 + (h % 120);
    return QColor::fromHsl(hue, 150, 120);
}

QPixmap AvatarHelper::generatePlaceholder(const QString &name, int size) {
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    QColor bgColor = getColdColor(name);
    painter.setBrush(bgColor);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(0, 0, size, size);

    QString firstLetter = name.isEmpty() ? "?" : name.left(1).toUpper();

    painter.setPen(Qt::white);
    QFont font = painter.font();
    font.setPixelSize(size * 0.5);
    font.setBold(true);
    painter.setFont(font);
    painter.drawText(0, 0, size, size, Qt::AlignCenter, firstLetter);

    return pixmap;
}

QPixmap AvatarHelper::makeRoundImage(const QPixmap &src, int size) {
    QPixmap target(size, size);
    target.fill(Qt::transparent);

    QPixmap scaled = src.scaled(size, size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);

    QPainter painter(&target);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    QPainterPath path;
    path.addEllipse(0, 0, size, size);
    painter.setClipPath(path);
    painter.drawPixmap(0, 0, scaled);

    return target;
}
