/*
 * repository_canvas.cpp
 *
 * Background canvas for the CODAXIS Overview page.
 *
 * EXISTING SECTION (lines 12-174 of original): preserved byte-for-byte.
 *   - Static ambient radial glow
 *   - Dot grid
 *   - Left-margin Git branch motif
 *   - Right-margin main trunk + feature/hotfix branches + commit nodes + labels
 *
 * ADDITIVE SECTION (appended, non-destructive):
 *   A. Animated data-packet particles  -- soft glowing dots traveling L→R
 *   B. Ambient depth vignette          -- gentle radial behind hero-card area
 *   C. Dimensional wordmark accent     -- italic "CODAXIS" gradient text, lower-left
 *
 * Animation is driven by a 33 ms QTimer (~30 fps).
 * Timer starts in showEvent, pauses in hideEvent → no CPU waste when hidden.
 */

#include "repository_canvas.h"
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QShowEvent>
#include <QHideEvent>
#include <QResizeEvent>
#include <QTimer>
#include <QtMath>
#include <QRandomGenerator>
#include <QFontDatabase>

// ============================================================================
// Construction
// ============================================================================

RepositoryCanvas::RepositoryCanvas(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_StyledBackground, true);

    m_timer = new QTimer(this);
    m_timer->setInterval(33); // ~30 fps
    connect(m_timer, &QTimer::timeout, this, &RepositoryCanvas::advanceParticles);
}

// ============================================================================
// Show / Hide — pause animation when not visible
// ============================================================================

void RepositoryCanvas::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    if (!m_particlesReady) {
        initParticles();
    }
    m_timer->start();
}

void RepositoryCanvas::hideEvent(QHideEvent *event)
{
    QWidget::hideEvent(event);
    m_timer->stop();
}

void RepositoryCanvas::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    // Re-anchor particle Y lanes to the new height.
    // X positions are kept so motion is not interrupted.
    const int h = height();
    if (h > 0) {
        for (auto &p : m_particles) {
            p.y = p.yFrac * h;
        }
    }
}

// ============================================================================
// Particle initialisation
// ============================================================================

void RepositoryCanvas::initParticles()
{
    m_particles.clear();

    // 24 particles distributed organically across the canvas
    const int count = 24;
    auto *rng = QRandomGenerator::global();

    for (int i = 0; i < count; ++i) {
        Particle pk;

        // Y lane: spread evenly across 8%–92% of canvas height
        pk.yFrac = 0.08 + (static_cast<qreal>(i) / (count - 1)) * 0.84;
        // Organic lane jitter (+- 2.5%)
        pk.yFrac += (rng->bounded(50) * 0.001 - 0.025);
        pk.yFrac = qBound(0.06, pk.yFrac, 0.94);
        pk.y     = pk.yFrac * height();

        // X position: distributed across the canvas width
        pk.x = static_cast<qreal>(rng->bounded(qMax(width(), 400)));

        // Speed & Direction:
        // ~25% travel right-to-left (negative), 75% travel left-to-right (positive)
        const bool reverseDir = (i % 4 == 0);
        if (reverseDir) {
            pk.speed = -(0.7 + rng->bounded(110) * 0.01); // -0.7 to -1.8 px/tick
        } else {
            pk.speed = 0.8 + rng->bounded(160) * 0.01;   // +0.8 to +2.4 px/tick
        }

        // Size: 1.6 to 2.9 px radius
        pk.size = 1.6 + rng->bounded(14) * 0.1;

        // Opacity: 0.50 to 0.85 (clearly visible against dark background)
        pk.opacity = 0.50 + rng->bounded(36) * 0.01;

        // Color: cycle cyan(0), violet(1), magenta(2)
        pk.colorType = i % 3;

        // Trail length: 14 to 32 px
        pk.trailLen = 14.0 + rng->bounded(19);

        m_particles.append(pk);
    }

    m_particlesReady = true;
}

// ============================================================================
// Particle advance slot (called by timer)
// ============================================================================

void RepositoryCanvas::advanceParticles()
{
    const qreal w = static_cast<qreal>(width());
    for (auto &pk : m_particles) {
        pk.x += pk.speed;
        if (pk.speed > 0 && (pk.x - pk.trailLen > w + 20.0)) {
            pk.x = -pk.trailLen - 10.0;
        } else if (pk.speed < 0 && (pk.x + pk.trailLen < -20.0)) {
            pk.x = w + pk.trailLen + 10.0;
        }
    }
    update();
}

// ============================================================================
// Paint event
// ============================================================================

void RepositoryCanvas::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const int w = width();
    const int h = height();

    // ========================================================================
    // EXISTING SECTION — preserved verbatim
    // ========================================================================

    // 1. Fill base canvas with deep smoky aubergine
    painter.fillRect(rect(), QColor(QStringLiteral("#13111c")));

    // 1b. Subtle ambient gradient aura in background center
    QRadialGradient ambientGlow(w * 0.38, h * 0.42, std::max(w, h) * 0.55);
    ambientGlow.setColorAt(0.0, QColor(139, 92, 246, 26));
    ambientGlow.setColorAt(0.5, QColor(224, 0, 255, 10));
    ambientGlow.setColorAt(1.0, QColor(19, 17, 28, 0));
    painter.fillRect(rect(), ambientGlow);

    // 1c. Gentle depth texture: subtle technical dot grid in background space
    const QColor gridDotColor(139, 92, 246, 18);
    for (int gx = 36; gx < w - 20; gx += 44) {
        for (int gy = 36; gy < h - 20; gy += 44) {
            painter.fillRect(gx, gy, 1, 1, gridDotColor);
        }
    }

    // 2. Only render ornamental Git graph if sufficient width exists
    if (w < 560) {
        // Still paint the additive layers even on narrow canvases
        goto additive_layers;
    }

    {
        // Subtle decorative ambient track on left margin
        const qreal leftTrackX = 42.0;
        QPen faintTrackPen(QColor(139, 92, 246, 50), 1.5, Qt::DashLine);
        painter.setPen(faintTrackPen);
        painter.drawLine(QPointF(leftTrackX, 75.0), QPointF(leftTrackX, std::min(h - 75.0, 360.0)));

        // Left track branch motif (sprouts release tag branch)
        QPainterPath leftBranchPath;
        leftBranchPath.moveTo(leftTrackX, 145.0);
        leftBranchPath.cubicTo(leftTrackX + 6.0, 160.0, leftTrackX + 22.0, 165.0, leftTrackX + 22.0, 180.0);
        leftBranchPath.lineTo(leftTrackX + 22.0, 205.0);
        leftBranchPath.cubicTo(leftTrackX + 22.0, 220.0, leftTrackX + 6.0, 225.0, leftTrackX, 240.0);

        QPen leftBranchPen(QColor(34, 211, 238, 75), 1.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        painter.setPen(leftBranchPen);
        painter.drawPath(leftBranchPath);

        painter.setPen(QPen(QColor(34, 211, 238, 90), 1.5));
        painter.setBrush(QColor(QStringLiteral("#13111c")));
        painter.drawEllipse(QPointF(leftTrackX, 115.0), 3.0, 3.0);
        painter.drawEllipse(QPointF(leftTrackX + 22.0, 192.0), 3.0, 3.0);
        painter.drawEllipse(QPointF(leftTrackX, 270.0), 3.0, 3.0);

        QFont leftTagFont = painter.font();
        leftTagFont.setPointSize(7);
        leftTagFont.setWeight(QFont::DemiBold);
        painter.setFont(leftTagFont);
        painter.setPen(QColor(148, 163, 184, 80));
        painter.drawText(QPointF(leftTrackX + 8.0, 119.0), QStringLiteral("origin/main"));
        painter.drawText(QPointF(leftTrackX + 28.0, 195.0), QStringLiteral("v1.2.0"));

        // Restrained code-structure architectural watermark in lower left
        if (h > 420) {
            painter.setPen(QColor(148, 163, 184, 50));
            painter.drawText(QPointF(leftTrackX - 12.0, std::min(h - 36.0, 470.0)),
                             QStringLiteral("git::rev-parse --verify HEAD"));
        }

        // Position motif on the right margin of the canvas
        const qreal trunkX = w - 90.0;
        const qreal branchX = trunkX - 55.0;

        // Neon Pulse palette: cyan, violet, and hot pink glowing strokes
        const QColor branchLineColor(139, 92, 246, 120);     // Violet glow
        const QColor pinkLineColor(255, 45, 149, 140);        // Hot pink branch
        const QColor cyanLineColor(34, 211, 238, 120);        // Cyan hotfix branch
        const QColor nodeFillColor(QStringLiteral("#13111c")); // Matches dark canvas
        const QColor nodeStrokePink(255, 45, 149, 220);       // Hot pink node
        const QColor nodeStrokeCyan(34, 211, 238, 220);       // Cyan node
        const QColor textMuted(148, 163, 184, 140);           // Slate muted tags
        const QColor textFaint(148, 163, 184, 85);            // Slate faint hash tags

        // --- Main Trunk Line ---
        QPen trunkPen(branchLineColor, 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        painter.setPen(trunkPen);
        painter.drawLine(QPointF(trunkX, 35.0), QPointF(trunkX, h - 35.0));

        // --- Secondary Feature Branch (sprouts, runs parallel, merges back) ---
        QPainterPath branchPath;
        branchPath.moveTo(trunkX, 110.0);
        branchPath.cubicTo(trunkX - 10.0, 135.0, branchX + 10.0, 145.0, branchX, 170.0);
        branchPath.lineTo(branchX, 290.0);
        branchPath.cubicTo(branchX + 10.0, 315.0, trunkX - 10.0, 325.0, trunkX, 350.0);

        QPen branchPen(pinkLineColor, 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        painter.setPen(branchPen);
        painter.drawPath(branchPath);

        // --- Tertiary Micro-Branch (Hotfix/patch branch off trunk) ---
        const qreal hotfixX = trunkX + 28.0;
        QPainterPath hotfixPath;
        hotfixPath.moveTo(trunkX, 220.0);
        hotfixPath.cubicTo(trunkX + 8.0, 232.0, hotfixX, 238.0, hotfixX, 252.0);
        hotfixPath.lineTo(hotfixX, 268.0);
        hotfixPath.cubicTo(hotfixX, 282.0, trunkX + 8.0, 288.0, trunkX, 300.0);

        QPen hotfixPen(cyanLineColor, 1.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        painter.setPen(hotfixPen);
        painter.drawPath(hotfixPath);

        // Subtle cross-branch reference dashed line
        QPen crossPen(QColor(139, 92, 246, 60), 1.0, Qt::DashLine);
        painter.setPen(crossPen);
        painter.drawLine(QPointF(branchX, 220.0), QPointF(trunkX, 220.0));

        // --- Helper for drawing commit nodes ---
        auto drawCommitNode = [&](qreal x, qreal y, qreal radius, const QColor &strokeColor, const QColor &fillColor) {
            painter.setPen(QPen(strokeColor, 2.0));
            painter.setBrush(fillColor);
            painter.drawEllipse(QPointF(x, y), radius, radius);
        };

        // Commit nodes on main trunk
        drawCommitNode(trunkX, 65.0, 4.0, nodeStrokeCyan, nodeFillColor);
        drawCommitNode(trunkX, 110.0, 5.0, nodeStrokePink, nodeFillColor);   // Branch origin
        drawCommitNode(trunkX, 220.0, 4.0, nodeStrokeCyan, nodeFillColor);   // Hotfix origin
        drawCommitNode(trunkX, 300.0, 4.0, nodeStrokeCyan, nodeFillColor);   // Hotfix merge
        drawCommitNode(trunkX, 350.0, 5.5, nodeStrokePink, QColor(255, 45, 149, 50)); // Merge node
        if (h > 460) {
            drawCommitNode(trunkX, 430.0, 4.5, nodeStrokeCyan, nodeFillColor);
        }

        // Commit nodes on feature branch
        drawCommitNode(branchX, 195.0, 5.0, nodeStrokePink, nodeFillColor);
        drawCommitNode(branchX, 260.0, 5.0, nodeStrokePink, nodeFillColor);

        // Commit node on hotfix branch
        drawCommitNode(hotfixX, 260.0, 3.5, nodeStrokeCyan, nodeFillColor);

        // Micro typography labels & commit hash tags
        QFont tagFont = painter.font();
        tagFont.setPointSize(7);
        tagFont.setWeight(QFont::DemiBold);
        painter.setFont(tagFont);

        painter.setPen(textMuted);
        painter.drawText(QPointF(trunkX + 9.0, 69.0), QStringLiteral("main"));
        painter.drawText(QPointF(branchX - 28.0, 199.0), QStringLiteral("feat"));
        painter.drawText(QPointF(trunkX + 9.0, 354.0), QStringLiteral("merge"));
        painter.drawText(QPointF(hotfixX + 6.0, 264.0), QStringLiteral("patch"));

        if (h > 460) {
            painter.drawText(QPointF(trunkX + 9.0, 434.0), QStringLiteral("HEAD"));
        }

        // Subtle commit hashes for authentic Git visual structure
        painter.setPen(textFaint);
        painter.drawText(QPointF(trunkX + 9.0, 114.0), QStringLiteral("8f2a1"));
        painter.drawText(QPointF(branchX - 38.0, 264.0), QStringLiteral("b41c0"));
        painter.drawText(QPointF(trunkX + 9.0, 304.0), QStringLiteral("c93e2"));
    }

    // ========================================================================
    // ADDITIVE SECTION — drawn after existing static layer, never modifying it
    // ========================================================================

additive_layers:

    // ---- A. Ambient depth vignette behind the approximate hero-card zone ----
    // The hero card is horizontally centred within the content area.
    // We add a very soft radial highlight behind it so it appears to lift gently.
    {
        const qreal centerX = w * 0.50;
        const qreal centerY = h * 0.48;
        const qreal glowR   = qMin(w, h) * 0.55;

        QRadialGradient heroGlow(centerX, centerY, glowR);
        heroGlow.setColorAt(0.00, QColor(139, 92, 246, 16));   // violet centre, very faint
        heroGlow.setColorAt(0.35, QColor(224, 0,  255,  8));   // magenta mid
        heroGlow.setColorAt(0.70, QColor( 34,211, 238,  4));   // cyan outer
        heroGlow.setColorAt(1.00, QColor( 19, 17,  28,  0));   // transparent edge

        painter.setPen(Qt::NoPen);
        painter.setBrush(heroGlow);
        painter.drawEllipse(QPointF(centerX, centerY), glowR, glowR);
    }

    // ---- B. Animated data-packet particles --------------------------------
    if (m_particlesReady && !m_particles.isEmpty()) {
        painter.save();
        painter.setRenderHint(QPainter::Antialiasing, true);

        // Core palette for packets: Electric Cyan, Radiant Violet, Vivid Hot Pink
        const QColor baseColors[3] = {
            QColor(34, 211, 238),   // 0 cyan
            QColor(168, 85, 247),   // 1 violet
            QColor(244, 63, 94),    // 2 vibrant magenta/pink
        };

        for (const auto &pk : m_particles) {
            const QColor &base = baseColors[pk.colorType];

            // 1. Motion trail (tapered comet tail with smooth RoundCap)
            const qreal tailX = (pk.speed >= 0) ? (pk.x - pk.trailLen) : (pk.x + pk.trailLen);
            const qreal headX = pk.x;

            QLinearGradient trailGrad(tailX, pk.y, headX, pk.y);
            trailGrad.setColorAt(0.0, Qt::transparent);
            QColor trailHead = base;
            trailHead.setAlphaF(pk.opacity * 0.55);
            trailGrad.setColorAt(1.0, trailHead);

            QPen trailPen(QBrush(trailGrad), pk.size * 1.5, Qt::SolidLine, Qt::RoundCap);
            painter.setPen(trailPen);
            painter.drawLine(QPointF(tailX, pk.y), QPointF(headX, pk.y));

            // 2. Soft outer radial aura
            QRadialGradient aura(pk.x, pk.y, pk.size * 3.0);
            QColor auraCore = base;
            auraCore.setAlphaF(pk.opacity * 0.35);
            aura.setColorAt(0.0, auraCore);
            aura.setColorAt(0.5, QColor(base.red(), base.green(), base.blue(), static_cast<int>(pk.opacity * 30)));
            aura.setColorAt(1.0, Qt::transparent);

            painter.setPen(Qt::NoPen);
            painter.setBrush(aura);
            painter.drawEllipse(QPointF(pk.x, pk.y), pk.size * 3.0, pk.size * 3.0);

            // 3. Crisp luminous core dot with white-hot center
            QRadialGradient coreDot(pk.x, pk.y, pk.size);
            QColor centerWhite(255, 255, 255, static_cast<int>(pk.opacity * 255));
            QColor edgeColor = base;
            edgeColor.setAlphaF(pk.opacity * 0.95);
            coreDot.setColorAt(0.0, centerWhite);
            coreDot.setColorAt(0.65, edgeColor);
            coreDot.setColorAt(1.0, base);

            painter.setBrush(coreDot);
            painter.drawEllipse(QPointF(pk.x, pk.y), pk.size, pk.size);
        }

        painter.restore();
    }

    // ---- C. Premium CODAXIS wordmark — perfectly positioned ABOVE the hero card ----
    //
    // Placement Logic:
    // 1. Queries the runtime geometry of the actual hero card child widget (welcomeHeroCard).
    // 2. Wordmark is horizontally centered strictly over the hero card's center (cardCenterX).
    // 3. Wordmark sits completely within the vertical zone [topCeiling, cardTop - bottomGap].
    //    It is mathematically impossible to overlap the hero card.
    // 4. Wordmark scales down gracefully if available vertical space or card width is constrained.
    //
    // Typography:
    // Segoe UI Variable Display (Windows 11 modern display face, matching Apple SF Pro Display)
    // with fallbacks to Candara (refined cursive humanist strokes) and Segoe UI.
    //
    // Rendering:
    // Crisp vector QPainterPath with vertical linear gradient (white-violet specular top
    // -> electric magenta -> deep violet base) + soft subtle drop shadow. No muddy 3D bevels.
    {
        const QWidget *heroCard = findChild<QWidget*>(QStringLiteral("welcomeHeroCard"));
        QRect cardRect;
        if (heroCard && heroCard->height() > 20) {
            cardRect = heroCard->geometry();
        } else {
            // Safe fallback before initial layout pass
            const int cardW = qMin(w - 80, 640);
            cardRect = QRect((w - cardW) / 2, 130, cardW, 430);
        }

        const qreal cardCenterX = cardRect.center().x();
        const qreal cardTop = static_cast<qreal>(cardRect.top());
        const qreal cardWidth = static_cast<qreal>(cardRect.width());

        // Available vertical envelope strictly ABOVE the hero card:
        const qreal topCeiling = 20.0;
        // Deliberate, clearly visible gap between wordmark bottom and card top
        const qreal bottomGap = qBound(14.0, (cardTop - topCeiling) * 0.18, 28.0);
        const qreal slotTop = topCeiling;
        const qreal slotBottom = cardTop - bottomGap;
        const qreal availSlotH = slotBottom - slotTop;

        // Render when adequate space exists above card and canvas has sufficient width
        if (availSlotH >= 24.0 && w >= 400) {
            painter.save();
            painter.setRenderHint(QPainter::Antialiasing, true);
            painter.setRenderHint(QPainter::TextAntialiasing, true);

            // Responsive typographic sizing:
            // Sized to be prominent and balanced with hero card
            int ptSize = qBound(18, static_cast<int>(availSlotH * 0.44), 46);

            QFont wmFont;
            // Primary font: Segoe UI Variable Display / Candara / Segoe UI
            // Ultra-modern editorial typography, sleek italic with Apple-like elegance
            wmFont.setFamilies({
                QStringLiteral("Segoe UI Variable Display"),
                QStringLiteral("Candara"),
                QStringLiteral("Segoe UI"),
                QStringLiteral("Century Gothic")
            });
            wmFont.setPointSize(ptSize);
            wmFont.setWeight(QFont::Bold);
            wmFont.setItalic(true);
            wmFont.setLetterSpacing(QFont::PercentageSpacing, 108);

            QFontMetricsF fm(wmFont);
            const QString wmText = QStringLiteral("CODAXIS");

            // Proportionally clamp so wordmark width <= 72% of card width
            // and typographic height <= 80% of available slot height
            while (ptSize > 14 && (fm.horizontalAdvance(wmText) > cardWidth * 0.72 || fm.height() > availSlotH * 0.80)) {
                ptSize -= 2;
                wmFont.setPointSize(ptSize);
                fm = QFontMetricsF(wmFont);
            }

            const qreal textW = fm.horizontalAdvance(wmText);
            const qreal textH = fm.height();

            // Horizontally centered strictly over the hero card
            const qreal baselineX = cardCenterX - (textW * 0.5);

            // Vertically centered within the slot strictly above the hero card
            const qreal slotCenterY = slotTop + (availSlotH * 0.5);
            const qreal baselineY = slotCenterY + (fm.ascent() - fm.descent()) * 0.5;

            // 1. Soft atmospheric back-glow
            {
                const qreal auraW = textW * 0.85;
                const qreal auraH = textH * 2.2;
                QRadialGradient aura(cardCenterX, slotCenterY, auraW * 0.55);
                aura.setColorAt(0.00, QColor(168,  85, 247, 28)); // refined violet
                aura.setColorAt(0.45, QColor(236,  72, 153, 14)); // subtle magenta
                aura.setColorAt(1.00, QColor( 19,  17,  28,  0)); // transparent

                painter.setPen(Qt::NoPen);
                painter.setBrush(aura);
                painter.drawEllipse(QPointF(cardCenterX, slotCenterY), auraW * 0.55, auraH * 0.55);
            }

            // Vector path of the text for razor-sharp rendering
            QPainterPath textPath;
            textPath.addText(QPointF(baselineX, baselineY), wmFont, wmText);

            // 2. Single soft, restrained drop shadow underneath
            painter.save();
            painter.translate(0.0, 3.0);
            painter.fillPath(textPath, QColor(6, 4, 14, 120));
            painter.restore();

            // 3. High-contrast, radiant modern gradient fill (White-violet specular top -> Electric magenta -> Violet base)
            const qreal glyphTop = baselineY - fm.ascent();
            const qreal glyphBottom = baselineY + fm.descent();

            QLinearGradient wordmarkGrad(baselineX, glyphTop, baselineX, glyphBottom);
            wordmarkGrad.setColorAt(0.00, QColor(255, 255, 255));        // Crisp white specular highlight
            wordmarkGrad.setColorAt(0.20, QColor(245, 238, 255));        // Luminous pale lilac
            wordmarkGrad.setColorAt(0.50, QColor(216, 180, 254));        // Electric radiant violet (#d8b4fe)
            wordmarkGrad.setColorAt(0.78, QColor(244, 114, 182));        // Vibrant neon magenta (#f472b6)
            wordmarkGrad.setColorAt(1.00, QColor(139,  92, 246));        // Grounding deep violet (#8b5cf6)

            painter.fillPath(textPath, wordmarkGrad);

            painter.restore();
        }
    }
}
