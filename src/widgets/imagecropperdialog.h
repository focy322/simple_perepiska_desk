#ifndef IMAGECROPPERDIALOG_H
#define IMAGECROPPERDIALOG_H

#include <QDialog>
#include <QPixmap>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QWheelEvent>

/**
 * \brief Диалоговое окно для кадрирования (обрезки) изображения.
 *
 * Позволяет масштабировать и перемещать изображение для получения квадратного аватара.
 */
class ImageCropperDialog : public QDialog {
    Q_OBJECT
public:
    /**
     * \brief Конструктор окна кадрирования.
     *
     * \param image Исходное изображение.
     * \param parent Родительский виджет.
     */
    explicit ImageCropperDialog(const QPixmap &image, QWidget *parent = nullptr);

    /**
     * \brief Возвращает обрезанное изображение.
     *
     * \return QPixmap квадратного размера.
     */
    QPixmap getCroppedImage() const;

protected:
    // --- События отрисовки и ввода ---
    
    /// \brief Переопределенный обработчик события отрисовки.
    void paintEvent(QPaintEvent *event) override;
    
    /// \brief Переопределенный обработчик нажатия кнопки мыши.
    void mousePressEvent(QMouseEvent *event) override;
    
    /// \brief Переопределенный обработчик перемещения мыши.
    void mouseMoveEvent(QMouseEvent *event) override;
    
    /// \brief Переопределенный обработчик отпускания кнопки мыши.
    void mouseReleaseEvent(QMouseEvent *event) override;
    
    /// \brief Переопределенный обработчик прокрутки колесика мыши.
    void wheelEvent(QWheelEvent *event) override;

private:
    // --- Внутреннее состояние ---
    QPixmap m_image;        //!< Исходное изображение
    double  m_scale;        //!< Текущий масштаб
    QPointF m_offset;       //!< Смещение изображения
    QPoint  m_lastMousePos; //!< Последняя позиция мыши (для перетаскивания)
    bool    m_isDragging;   //!< Флаг перетаскивания мышью
    int     m_cropSize;     //!< Размер рамки кадрирования
};

#endif // IMAGECROPPERDIALOG_H
