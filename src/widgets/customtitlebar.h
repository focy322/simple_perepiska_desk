#ifndef CUSTOMTITLEBAR_H
#define CUSTOMTITLEBAR_H

#include <QWidget>
#include <QAbstractButton>

/**
 * \brief Кастомный заголовок окна (Title Bar).
 *
 * Заменяет стандартный заголовок ОС, содержит кнопки управления окном.
 */
class CustomTitleBar : public QWidget
{
    Q_OBJECT
public:
    explicit CustomTitleBar(QWidget *parent = nullptr);

private slots:
    // --- Внутренние обработчики ---

    /**
     * \brief Обработчик нажатия на кнопку "Свернуть"
     */
    void onMinimizeClicked();

    /**
     * \brief Обработчик нажатия на кнопку "Развернуть/Восстановить"
     */
    void onMaximizeRestoreClicked();

    /**
     * \brief Обработчик нажатия на кнопку "Закрыть"
     */
    void onCloseClicked();

private:
    // --- Элементы UI ---
    QAbstractButton *m_minimizeBtn; //!< Кнопка сворачивания окна
    QAbstractButton *m_maximizeBtn; //!< Кнопка разворачивания/восстановления окна
    QAbstractButton *m_closeBtn;    //!< Кнопка закрытия окна
};

#endif // CUSTOMTITLEBAR_H
