#include "repository_canvas.h"
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>

RepositoryCanvas::RepositoryCanvas(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_StyledBackground, true);
}

void RepositoryCanvas::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const int w = width();
    const int h = height();

    // 1. Fill base canvas with warm ivory
    painter.fillRect(rect(), QColor(QStringLiteral("#faf8f5")));

    // 2. Only render ornamental Git graph if sufficient width exists
    if (w < 560) {
        return;
    }

    // Position motif on the right margin of the canvas
    const qreal trunkX = w - 90.0;
    const qreal branchX = trunkX - 55.0;

    // Palette: restrained neutral-warm lines and faint olive accents
    const QColor branchLineColor(195, 189, 178, 90);       // Soft warm taupe
    const QColor oliveLineColor(82, 102, 73, 85);           // Faint olive-green
    const QColor nodeFillColor(QStringLiteral("#faf8f5"));  // Matches canvas
    const QColor nodeStrokeOlive(82, 102, 73, 140);        // Olive stroke
    const QColor nodeStrokeNeutral(180, 174, 163, 140);     // Neutral stroke
    const QColor textMuted(120, 122, 116, 90);              // Whisper-quiet tags

    // --- Main Trunk Line ---
    QPen trunkPen(branchLineColor, 1.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter.setPen(trunkPen);
    painter.drawLine(QPointF(trunkX, 35.0), QPointF(trunkX, h - 35.0));

    // --- Secondary Feature Branch (sprouts, runs parallel, merges back) ---
    QPainterPath branchPath;
    branchPath.moveTo(trunkX, 110.0);
    branchPath.cubicTo(trunkX - 10.0, 135.0, branchX + 10.0, 145.0, branchX, 170.0);
    branchPath.lineTo(branchX, 290.0);
    branchPath.cubicTo(branchX + 10.0, 315.0, trunkX - 10.0, 325.0, trunkX, 350.0);

    QPen branchPen(oliveLineColor, 1.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter.setPen(branchPen);
    painter.drawPath(branchPath);

    // --- Helper for drawing commit nodes ---
    auto drawCommitNode = [&](qreal x, qreal y, qreal radius, const QColor &strokeColor, const QColor &fillColor) {
        painter.setPen(QPen(strokeColor, 1.5));
        painter.setBrush(fillColor);
        painter.drawEllipse(QPointF(x, y), radius, radius);
    };

    // Commit nodes on main trunk
    drawCommitNode(trunkX, 65.0, 4.0, nodeStrokeNeutral, nodeFillColor);
    drawCommitNode(trunkX, 110.0, 4.5, nodeStrokeOlive, nodeFillColor);   // Branch origin
    drawCommitNode(trunkX, 230.0, 4.0, nodeStrokeNeutral, nodeFillColor);
    drawCommitNode(trunkX, 350.0, 5.0, nodeStrokeOlive, QColor(82, 102, 73, 40)); // Merge node
    if (h > 460) {
        drawCommitNode(trunkX, 430.0, 4.0, nodeStrokeNeutral, nodeFillColor);
    }

    // Commit nodes on feature branch
    drawCommitNode(branchX, 195.0, 4.5, nodeStrokeOlive, nodeFillColor);
    drawCommitNode(branchX, 260.0, 4.5, nodeStrokeOlive, nodeFillColor);

    // Micro typography labels (ornamental metadata)
    QFont tagFont = painter.font();
    tagFont.setPointSize(7);
    tagFont.setWeight(QFont::DemiBold);
    painter.setFont(tagFont);
    painter.setPen(textMuted);

    painter.drawText(QPointF(trunkX + 9.0, 69.0), QStringLiteral("main"));
    painter.drawText(QPointF(branchX - 28.0, 199.0), QStringLiteral("feat"));
    painter.drawText(QPointF(trunkX + 9.0, 354.0), QStringLiteral("merge"));
}
