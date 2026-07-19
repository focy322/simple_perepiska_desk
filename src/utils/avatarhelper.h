#ifndef AVATARHELPER_H
#define AVATARHELPER_H

#include <QPixmap>
#include <QString>
#include <QColor>

/**
 * @brief Вспомогательный класс для работы с аватарами пользователей
 */
class AvatarHelper {
public:
    /**
     * Генерирует круглый аватар-заглушку с первой буквой имени
     * \param name Имя пользователя (для выбора цвета и буквы)
     * \param size Размер итогового изображения в пикселях (ширина и высота)
     * \return Сгенерированное изображение аватара
     */
    static QPixmap generatePlaceholder(const QString &name, int size);

    /**
     * Скругляет переданное изображение (обрезает по кругу)
     * \param src Исходное изображение
     * \param size Требуемый размер (ширина и высота)
     * \return Скругленное изображение
     */
    static QPixmap makeRoundImage(const QPixmap &src, int size);

    /**
     * Генерирует стабильный "холодный" цвет на основе имени пользователя
     * \param name Имя пользователя (используется MD5 хэш)
     * \return Цвет в формате QColor
     */
    static QColor getColdColor(const QString &name);
};

#endif // AVATARHELPER_H
