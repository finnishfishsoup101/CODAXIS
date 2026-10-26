#ifndef REPOSITORY_CANVAS_H
#define REPOSITORY_CANVAS_H

#include <QWidget>
#include <QVector>
#include <QPointF>

class QTimer;

/**
 * @brief Repository-inspired background canvas for CODAXIS pages.
 *
 * Two operation modes:
 *
 *  - Mode::Overview (default): Vivid full-intensity version used as the Overview
 *    page widget itself. 72 packets, high opacity, wide glow, static branch
 *    motifs, CODAXIS wordmark. Unchanged from the previous implementation.
 *
 *  - Mode::ContentPage: Subtle low-intensity version for Dashboard, Commits,
 *    Contributors, Hotspots, and Repository Health. 14 packets, low opacity,
 *    short trails, minimal glow. Fills the background of its parent page via
 *    Qt's StackAll stacking layout — content widgets are placed on top and
 *    remain fully interactive.
 *
 * In both modes, the animation timer starts on showEvent and pauses on
 * hideEvent, so there is zero CPU cost while the widget is hidden.
 */
class RepositoryCanvas : public QWidget {
    Q_OBJECT
public:
    enum class Mode { Overview, ContentPage };

    explicit RepositoryCanvas(QWidget *parent = nullptr,
                              Mode mode = Mode::Overview);

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
        int   colorType;  ///< 0 = cyan, 1 = violet, 2 = magenta, 3 = fuchsia
        qreal trailLen;   ///< trailing glow length in pixels
    };

    void initParticles();
    void paintContentPageBackground(QPainter &painter, int w, int h);

    QTimer          *m_timer{nullptr};
    QVector<Particle> m_particles;
    bool              m_particlesReady{false};
    Mode              m_mode{Mode::Overview};
};

#endif // REPOSITORY_CANVAS_H
