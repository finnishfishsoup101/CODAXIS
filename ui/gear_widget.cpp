/*
 * gear_widget.cpp
 *
 * Self-contained decorative gear widget for the CODAXIS sidebar.
 * Uses QPainter + QVariantAnimation only – no external dependencies.
 *
 * Visual specification
 * --------------------
 *  · 10 teeth, drawn as radially-placed rounded rectangles
 *  · Solid gear body disc behind the teeth
 *  · Central circular hub
 *  · Inner spoke ring (subtle detail ring)
 *  · Palette: CODAXIS Neon Pulse – violet, magenta, cyan
 *  · Rotation: clockwise, 12 s per revolution, linear easing
 *  · Pause/resume preserves current angle (no snap-to-zero)
 */

#include "gear_widget.h"

#include <QPainter>
#include <QPainterPath>
#include <QVariantAnimation>
#include <QShowEvent>
#include <QHideEvent>
#include <QtMath>

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

GearWidget::GearWidget(QWidget *parent)
    : QWidget(parent)
{
    // Transparent background – the sidebar background shows through
    setAttribute(Qt::WA_TranslucentBackground);
    setFocusPolicy(Qt::NoFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    // Build the infinite looping animation (0° → 360°, 12 s per revolution)
    m_animation = new QVariantAnimation(this);
    m_animation->setStartValue(0.0);
    m_animation->setEndValue(360.0);
    m_animation->setDuration(12000);          // ms
    m_animation->setLoopCount(-1);            // infinite
    m_animation->setEasingCurve(QEasingCurve::Linear);

    connect(m_animation, &QVariantAnimation::valueChanged,
            this, [this](const QVariant &val) {
                m_angle = val.toReal();
                update();
            });
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void GearWidget::setActive(bool active)
{
    if (m_active == active)
        return;

    m_active = active;

    if (active) {
        // Resume from current angle so the gear doesn't snap to 0°
        if (m_animation->state() != QVariantAnimation::Running) {
            // Map current angle back to a normalised time offset so the
            // animation starts seamlessly from where it left off.
            const qreal fraction = m_angle / 360.0;
            m_animation->start();
            // Advance the internal time to match the stored angle.
            // Qt's QVariantAnimation does not expose setCurrentTime natively
            // for looped animations while running, but we can stop, set the
            // start value to the current angle, reset end to 360, then
            // re-start.  Simpler approach: set currentTime on the paused
            // animation, then start.
            m_animation->pause();
            m_animation->setCurrentTime(static_cast<int>(fraction * m_animation->duration()));
            m_animation->resume();
        }
    } else {
        if (m_animation->state() == QVariantAnimation::Running) {
            m_animation->pause();
        }
    }
}

// ---------------------------------------------------------------------------
// Protected overrides
// ---------------------------------------------------------------------------

void GearWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    // Resume animation only if we were previously rotating
    if (m_active && m_animation->state() == QVariantAnimation::Paused) {
        m_animation->resume();
    }
}

void GearWidget::hideEvent(QHideEvent *event)
{
    QWidget::hideEvent(event);
    // Always pause when hidden to avoid unnecessary CPU usage
    if (m_animation->state() == QVariantAnimation::Running) {
        m_animation->pause();
    }
}

QSize GearWidget::sizeHint() const
{
    return {100, 100};
}

QSize GearWidget::minimumSizeHint() const
{
    return {60, 60};
}

// ---------------------------------------------------------------------------
// Painting
// ---------------------------------------------------------------------------

void GearWidget::paintEvent(QPaintEvent * /*event*/)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);

    const qreal w = static_cast<qreal>(width());
    const qreal h = static_cast<qreal>(height());

    // Available drawing area – leave breathing room inside the widget
    const qreal available = qMin(w, h) * 0.82;
    const qreal outerR    = available * 0.50;
    const qreal cx        = w * 0.50;
    const qreal cy        = h * 0.50;

    drawGear(p, cx, cy, outerR);
}

// ---------------------------------------------------------------------------
// Gear drawing
// ---------------------------------------------------------------------------

void GearWidget::drawGear(QPainter &p, qreal cx, qreal cy, qreal outerR) const
{
    // -----------------------------------------------------------------------
    // Palette (Neon Pulse / CODAXIS)
    // -----------------------------------------------------------------------
    const QColor col_violet   (0x8b, 0x5c, 0xf6);          // #8b5cf6
    const QColor col_magenta  (0xe0, 0x00, 0xff);          // #e000ff
    const QColor col_cyan     (0x22, 0xd3, 0xee);          // #22d3ee
    const QColor col_body     (0x1e, 0x19, 0x2c);          // #1e192c  CardSurface
    const QColor col_ring     (0x3d, 0x32, 0x57);          // #3d3257  BorderSubtle
    const QColor col_hub      (0x24, 0x1d, 0x34);          // #241d34  SubSurface

    const QColor col_toothFill(0x29, 0x1f, 0x3b);          // PlumSurface
    const QColor col_toothBorder = col_violet;

    // Radii (all relative to outerR)
    const qreal bodyR     = outerR * 0.72;
    const qreal toothW    = outerR * 0.22;   // half-width of each tooth rectangle
    const qreal toothH    = outerR * 0.30;   // height (radial extent) of each tooth
    const qreal innerRingR = bodyR  * 0.65;
    const qreal hubR      = bodyR  * 0.28;

    const int   numTeeth  = 10;

    // -----------------------------------------------------------------------
    // Rotate everything around (cx, cy) by m_angle
    // -----------------------------------------------------------------------
    p.save();
    p.translate(cx, cy);
    p.rotate(m_angle);

    // -----------------------------------------------------------------------
    // 1. Gear body disc (solid fill, subtle border)
    // -----------------------------------------------------------------------
    {
        QRadialGradient bodyGrad(0, 0, bodyR);
        bodyGrad.setColorAt(0.0, col_body.lighter(120));
        bodyGrad.setColorAt(1.0, col_body);
        p.setBrush(bodyGrad);
        p.setPen(QPen(col_ring, 1.4));
        p.drawEllipse(QPointF(0, 0), bodyR, bodyR);
    }

    // -----------------------------------------------------------------------
    // 2. Teeth — drawn as rounded rectangles at each tooth angle
    // -----------------------------------------------------------------------
    {
        const qreal cornerRadius = toothW * 0.55;
        p.setPen(QPen(col_toothBorder, 1.2));
        p.setBrush(col_toothFill);

        for (int i = 0; i < numTeeth; ++i) {
            const qreal angleDeg  = (360.0 / numTeeth) * i;
            const qreal angleRad  = qDegreesToRadians(angleDeg);

            p.save();
            p.rotate(angleDeg);

            // The tooth rect: centred on +Y axis, base at bodyR
            const qreal baseY = bodyR - toothH * 0.08;   // slight overlap with body
            const QRectF toothRect(
                -toothW,
                -(baseY + toothH),
                toothW * 2.0,
                toothH
            );
            p.drawRoundedRect(toothRect, cornerRadius, cornerRadius);

            p.restore();

            Q_UNUSED(angleRad)
        }
    }

    // -----------------------------------------------------------------------
    // 3. Inner accent ring (subtle detail)
    // -----------------------------------------------------------------------
    {
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(col_cyan.darker(140), 1.0));
        p.drawEllipse(QPointF(0, 0), innerRingR, innerRingR);
    }

    // -----------------------------------------------------------------------
    // 4. Spokes (4 spokes, drawn before the hub to be clipped naturally)
    // -----------------------------------------------------------------------
    {
        const int   numSpokes = 4;
        const qreal spokeW    = bodyR * 0.09;
        const qreal spokeLen  = innerRingR - hubR - bodyR * 0.02;

        QLinearGradient spokeGrad(0, -hubR, 0, -innerRingR);
        spokeGrad.setColorAt(0.0, col_violet.darker(120));
        spokeGrad.setColorAt(1.0, col_violet);

        p.setBrush(spokeGrad);
        p.setPen(Qt::NoPen);

        for (int s = 0; s < numSpokes; ++s) {
            p.save();
            p.rotate((360.0 / numSpokes) * s);
            const QRectF spokeRect(
                -spokeW * 0.5,
                -(hubR + spokeLen),
                spokeW,
                spokeLen
            );
            p.drawRoundedRect(spokeRect, spokeW * 0.5, spokeW * 0.5);
            p.restore();
        }
    }

    // -----------------------------------------------------------------------
    // 5. Central hub
    // -----------------------------------------------------------------------
    {
        QRadialGradient hubGrad(0, 0, hubR);
        hubGrad.setColorAt(0.0, col_magenta.darker(200));
        hubGrad.setColorAt(0.6, col_hub);
        hubGrad.setColorAt(1.0, col_violet.darker(160));

        p.setBrush(hubGrad);
        p.setPen(QPen(col_magenta.darker(130), 1.6));
        p.drawEllipse(QPointF(0, 0), hubR, hubR);

        // Hub highlight dot
        p.setBrush(col_cyan);
        p.setPen(Qt::NoPen);
        const qreal dotR = hubR * 0.22;
        p.drawEllipse(QPointF(0, 0), dotR, dotR);
    }

    p.restore();

    // -----------------------------------------------------------------------
    // 6. Outer glow ring (drawn outside the rotation transform so it stays
    //    perfectly centred; it is cosmetic only)
    // -----------------------------------------------------------------------
    {
        const qreal glowR = outerR + toothH * 0.60;
        QRadialGradient glow(cx, cy, glowR);
        glow.setColorAt(0.72, Qt::transparent);
        glow.setColorAt(0.85, QColor(0x8b, 0x5c, 0xf6, 28));  // violet tint
        glow.setColorAt(1.0,  Qt::transparent);

        p.setPen(Qt::NoPen);
        p.setBrush(glow);
        p.drawEllipse(QPointF(cx, cy), glowR, glowR);
    }
}
