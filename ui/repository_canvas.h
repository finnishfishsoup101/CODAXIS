#ifndef REPOSITORY_CANVAS_H
#define REPOSITORY_CANVAS_H

#include <QWidget>
#include <QVector>
#include <QPointF>

class QTimer;

/**
 * @brief Subtle repository-inspired background canvas for the Overview page.
 * Renders an ornamental, understated vector Git branch graph using QPainter,
 * evoking commits, branches, and code topology behind the content.
 *
 * Additive ambient layer (non-breaking):
 *  - Animated data-packet particles that travel horizontally across the canvas
 *  - Gentle ambient depth vignette behind the hero card area
 *  - Dimensional CODAXIS wordmark accent (bottom-left, italic, gradient)
 * Animation pauses automatically when the widget is hidden.
 */
class RepositoryCanvas : public QWidget {
    Q_OBJECT
public:
    explicit RepositoryCanvas(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void advanceParticles();

private:
    // ---- Particle system ------------------------------------------------
    struct Particle {
        qreal x;          ///< current x position (pixels)
        qreal y;          ///< fixed y lane (pixels, relative to canvas height)
        qreal yFrac;      ///< y as fraction of height for responsive positioning
        qreal speed;      ///< pixels per tick
        qreal size;       ///< dot radius
        qreal opacity;    ///< 0.0 – 1.0
        int   colorType;  ///< 0 = cyan, 1 = violet, 2 = magenta
        qreal trailLen;   ///< trailing glow length in pixels
    };

    void initParticles();

    QTimer          *m_timer{nullptr};
    QVector<Particle> m_particles;
    bool              m_particlesReady{false};
};

#endif // REPOSITORY_CANVAS_H
