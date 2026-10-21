#ifndef REPOSITORY_CANVAS_H
#define REPOSITORY_CANVAS_H

#include <QWidget>

/**
 * @brief Subtle repository-inspired background canvas for the Overview page.
 * Renders an ornamental, understated vector Git branch graph using QPainter,
 * evoking commits, branches, and code topology behind the content.
 */
class RepositoryCanvas : public QWidget {
    Q_OBJECT
public:
    explicit RepositoryCanvas(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
};

#endif // REPOSITORY_CANVAS_H
