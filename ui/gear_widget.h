#ifndef GEAR_WIDGET_H
#define GEAR_WIDGET_H

#include <QWidget>

class QVariantAnimation;

/**
 * @brief Decorative mechanical-gear widget for the CODAXIS sidebar.
 *
 * Draws a recognisable gear using the Neon Pulse palette (dark-purple
 * background, violet/magenta/cyan accents).  The gear rotates smoothly only
 * when the repository is loaded AND the Dashboard page is active; it is
 * completely stationary otherwise.
 *
 * This is a strictly additive, self-contained widget.  It owns its own
 * QVariantAnimation and pauses it automatically when the widget is hidden
 * to avoid wasting CPU cycles.
 *
 * Public API (called by MainWindow):
 *   setActive(bool) - true  -> start / resume rotation
 *                   - false -> stop  / freeze rotation
 */
class GearWidget : public QWidget
{
    Q_OBJECT

public:
    explicit GearWidget(QWidget *parent = nullptr);
    ~GearWidget() override = default;

    /** Enable or disable rotation without resetting the current angle. */
    void setActive(bool active);

protected:
    void paintEvent(QPaintEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    QSize sizeHint()  const override;
    QSize minimumSizeHint() const override;

private:
    QVariantAnimation *m_animation{nullptr};
    qreal              m_angle{0.0};    ///< current rotation angle in degrees
    bool               m_active{false}; ///< whether the gear should rotate

    void drawGear(class QPainter &p, qreal cx, qreal cy, qreal outerR) const;
};

#endif // GEAR_WIDGET_H
