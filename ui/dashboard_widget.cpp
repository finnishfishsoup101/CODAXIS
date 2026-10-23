#include "dashboard_widget.h"
#include "core/git_analytics_service.h"
#include "ui/theme.h"

#include <QDir>
#include <QFileInfo>
#include <QFont>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMap>
#include <QMouseEvent>
#include <QPainter>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QSplineSeries>
#include <QTableWidget>
#include <QToolTip>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>

// ============================================================================
// ActivityChartView (P1 fix: pixel-space snapping tooltip)
// ============================================================================
ActivityChartView::ActivityChartView(QWidget *parent) : QChartView(parent) {
  setRenderHint(QPainter::Antialiasing);
  setMouseTracking(true);
}

void ActivityChartView::setActivityData(const CommitActivityData &data,
                                        QLineSeries *lineSeries) {
  m_data = data;
  m_lineSeries = lineSeries;
  m_highlightSeries = nullptr;
}

void ActivityChartView::mouseMoveEvent(QMouseEvent *event) {
  QChartView::mouseMoveEvent(event);
  if (!chart() || m_data.buckets.isEmpty() || !m_lineSeries)
    return;

  const QPointF mousePos = event->position();
  if (!chart()->plotArea().contains(mousePos)) {
    if (m_highlightSeries)
      m_highlightSeries->clear();
    QToolTip::hideText();
    return;
  }

  const int n = m_data.buckets.size();
  const QPointF chartVal = chart()->mapToValue(mousePos, m_lineSeries);
  int idx = qRound(chartVal.x());
  if (idx < 0)
    idx = 0;
  if (idx >= n)
    idx = n - 1;

  const auto &bucket = m_data.buckets[idx];
  if (!m_highlightSeries) {
    m_highlightSeries = new QScatterSeries();
    m_highlightSeries->setMarkerSize(12.0);
    m_highlightSeries->setColor(QColor(QStringLiteral("#22d3ee")));
    m_highlightSeries->setBorderColor(QColor(QStringLiteral("#ffffff")));
    chart()->addSeries(m_highlightSeries);
    for (auto *ax : chart()->axes())
      m_highlightSeries->attachAxis(ax);
  }
  m_highlightSeries->clear();
  m_highlightSeries->append(idx, bucket.commitCount);

  const QString tip =
      QStringLiteral(
          "<div style='background-color:#1e182e; color:#f8fafc; "
          "font-family:Segoe UI; padding:6px 10px; border-radius:6px; "
          "border:1px solid #5a4c8a;'>"
          "<span style='color:#a59abf; font-size:11px;'>%1</span><br/>"
          "<b style='color:#ff2d95; font-size:15px;'>%2 %3</b></div>")
          .arg(bucket.date.toString(QStringLiteral("d MMM yyyy")))
          .arg(bucket.commitCount)
          .arg(bucket.commitCount == 1 ? QStringLiteral("commit")
                                       : QStringLiteral("commits"));
  QToolTip::showText(event->globalPosition().toPoint(), tip, this);
}

void ActivityChartView::leaveEvent(QEvent *event) {
  QChartView::leaveEvent(event);
  if (m_highlightSeries)
    m_highlightSeries->clear();
  QToolTip::hideText();
}

// ============================================================================
// KpiCard
// ============================================================================
KpiCard::KpiCard(const QString &icon, const QString &title,
                 const QColor &accentColor, QWidget *parent)
    : QFrame(parent), m_accentColor(accentColor) {
  setObjectName(QStringLiteral("dashboardKpiCard"));
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  m_shadow = new QGraphicsDropShadowEffect(this);
  m_shadow->setBlurRadius(22);
  m_shadow->setColor(QColor(8, 4, 16, 150));
  m_shadow->setOffset(0, 4);
  setGraphicsEffect(m_shadow);
  leaveEvent(nullptr);

  // Root layout: metrics details on the left, enlarged icon on the right
  auto *rootLay = new QHBoxLayout(this);
  rootLay->setContentsMargins(20, 16, 20, 16);
  rootLay->setSpacing(12);

  auto *leftCol = new QVBoxLayout();
  leftCol->setContentsMargins(0, 0, 0, 0);
  leftCol->setSpacing(4);

  auto *tLbl = new QLabel(title, this);
  tLbl->setStyleSheet(
      QStringLiteral("color:#a59abf;font-size:10px;font-weight:700;letter-"
                     "spacing:1.1px;text-transform:uppercase;"));
  leftCol->addWidget(tLbl);

  m_valLabel = new QLabel(QString::fromUtf8("\xe2\x80\x94"), this);
  m_valLabel->setStyleSheet(QStringLiteral(
      "color:#ffffff;font-size:30px;font-weight:800;letter-spacing:-0.5px;"));
  leftCol->addWidget(m_valLabel);

  m_subLabel = new QLabel(QStringLiteral("No repository open"), this);
  m_subLabel->setStyleSheet(
      QStringLiteral("color:#8c82a5;font-size:11px;font-weight:500;"));
  leftCol->addWidget(m_subLabel);

  rootLay->addLayout(leftCol, 1);

  // Enlarged KPI icon neatly positioned on the right side of the card,
  // vertically centered
  auto *iconLbl = new QLabel(icon, this);
  iconLbl->setAlignment(Qt::AlignCenter);
  iconLbl->setStyleSheet(
      QStringLiteral("font-size:32px; color:%1; background:transparent;")
          .arg(m_accentColor.name()));
  iconLbl->setFixedWidth(46);
  rootLay->addWidget(iconLbl, 0, Qt::AlignRight | Qt::AlignVCenter);
}

void KpiCard::setValue(const QString &val) {
  if (m_valLabel)
    m_valLabel->setText(val);
}
void KpiCard::setSubText(const QString &sub) {
  if (m_subLabel)
    m_subLabel->setText(sub);
}

void KpiCard::enterEvent(QEnterEvent *event) {
  QFrame::enterEvent(event);
  setStyleSheet(QStringLiteral("QFrame#dashboardKpiCard{"
                               "background:qlineargradient(x1:0,y1:0,x2:0,y2:1,"
                               "stop:0 rgba(52,40,75,0.92),stop:0.08 "
                               "rgba(68,52,98,0.8),stop:1 rgba(32,24,48,0.95));"
                               "border:1.5px solid %1;border-top:3.5px solid "
                               "%1;border-radius:14px;}")
                    .arg(m_accentColor.name()));
  m_shadow->setBlurRadius(28);
  m_shadow->setColor(QColor(m_accentColor.red(), m_accentColor.green(),
                            m_accentColor.blue(), 85));
  m_shadow->setOffset(0, 6);
}

void KpiCard::leaveEvent(QEvent *event) {
  if (event)
    QFrame::leaveEvent(event);
  setStyleSheet(
      QStringLiteral("QFrame#dashboardKpiCard{"
                     "background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 "
                     "rgba(42,32,60,0.8),stop:0.08 rgba(54,42,78,0.7),stop:1 "
                     "rgba(24,18,36,0.85));"
                     "border:1px solid rgba(160,135,215,0.22);border-top:3.5px "
                     "solid %1;border-radius:14px;}")
          .arg(m_accentColor.name()));
  m_shadow->setBlurRadius(20);
  m_shadow->setColor(QColor(8, 4, 16, 140));
  m_shadow->setOffset(0, 4);
}

// ============================================================================
// HotspotRowWidget
// ============================================================================
HotspotRowWidget::HotspotRowWidget(int rank, const HotspotRecord &hotspot,
                                   int maxChurn, QWidget *parent)
    : QFrame(parent) {
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  setCursor(Qt::PointingHandCursor);
  leaveEvent(nullptr);

  auto *lay = new QHBoxLayout(this);
  lay->setContentsMargins(14, 9, 14, 9);
  lay->setSpacing(12);

  auto *rankBadge = new QLabel(QStringLiteral("#%1").arg(rank), this);
  rankBadge->setFixedWidth(28);
  rankBadge->setStyleSheet(
      QStringLiteral("color:#ffffff;background:qlineargradient(x1:0,y1:0,x2:1,"
                     "y2:1,stop:0 #ff2d95,stop:1 #8b5cf6);"
                     "border-radius:5px;font-size:10px;font-weight:800;padding:"
                     "2px 4px;text-align:center;"));
  rankBadge->setAlignment(Qt::AlignCenter);
  lay->addWidget(rankBadge);

  QFont monoFont(QStringLiteral("Consolas"));
  monoFont.setPointSize(9);
  auto *pathLabel = new QLabel(hotspot.filePath, this);
  pathLabel->setFont(monoFont);
  pathLabel->setStyleSheet(QStringLiteral("color:#f1f5f9;font-weight:600;"));
  pathLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  lay->addWidget(pathLabel, 1);

  auto *meter = new QProgressBar(this);
  meter->setRange(0, std::max(1, maxChurn));
  meter->setValue(hotspot.commits);
  meter->setTextVisible(false);
  meter->setFixedWidth(80);
  meter->setFixedHeight(7);
  meter->setStyleSheet(QStringLiteral(
      "QProgressBar{background:rgba(20,16,30,0.7);border-radius:3px;border:1px "
      "solid rgba(120,100,160,0.25);}"
      "QProgressBar::chunk{background:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:"
      "0 #ff2d95,stop:0.5 #e000ff,stop:1 #22d3ee);border-radius:3px;}"));
  lay->addWidget(meter);

  auto *countLabel =
      new QLabel(QStringLiteral("%1 commits").arg(hotspot.commits), this);
  countLabel->setFixedWidth(80);
  countLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  countLabel->setStyleSheet(
      QStringLiteral("color:#22d3ee;font-weight:700;font-size:11px;"));
  lay->addWidget(countLabel);

  setToolTip(QStringLiteral("File: %1\nChurn: %2 commits\nLatest: %3")
                 .arg(hotspot.filePath)
                 .arg(hotspot.commits)
                 .arg(hotspot.latestDate.isEmpty() ? QStringLiteral("N/A")
                                                   : hotspot.latestDate));
}

void HotspotRowWidget::enterEvent(QEnterEvent *event) {
  QFrame::enterEvent(event);
  setStyleSheet(
      QStringLiteral("QFrame{background:rgba(52,40,75,0.85);border:1.5px solid "
                     "rgba(224,0,255,0.5);border-radius:9px;}"));
}

void HotspotRowWidget::leaveEvent(QEvent *event) {
  if (event)
    QFrame::leaveEvent(event);
  setStyleSheet(
      QStringLiteral("QFrame{background:rgba(32,25,46,0.7);border:1px solid "
                     "rgba(150,125,200,0.18);border-radius:9px;}"));
}

// ============================================================================
// Helpers
// ============================================================================
namespace {

QFrame *makeGlassCard(QWidget *parent) {
  auto *card = new QFrame(parent);
  card->setObjectName(QStringLiteral("glassCard"));
  card->setStyleSheet(
      QStringLiteral("QFrame#glassCard{"
                     "background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 "
                     "rgba(44,34,64,0.78),stop:0.06 rgba(58,46,84,0.65),stop:1 "
                     "rgba(24,18,36,0.84));"
                     "border:1px solid rgba(170,145,225,0.22);border-top:1px "
                     "solid rgba(225,200,255,0.38);border-radius:14px;}"));
  card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  auto *shadow = new QGraphicsDropShadowEffect(card);
  shadow->setBlurRadius(24);
  shadow->setColor(QColor(8, 4, 16, 160));
  shadow->setOffset(0, 5);
  card->setGraphicsEffect(shadow);
  return card;
}

QLabel *makeSectionTitle(const QString &text, QWidget *parent) {
  auto *lbl = new QLabel(text, parent);
  lbl->setStyleSheet(QStringLiteral(
      "color:#ffffff;font-size:14px;font-weight:700;letter-spacing:0.2px;"));
  return lbl;
}

QLabel *makeSectionSub(const QString &text, QWidget *parent) {
  auto *lbl = new QLabel(text, parent);
  lbl->setStyleSheet(QStringLiteral("color:#9d93b8;font-size:11px;"));
  return lbl;
}

const QList<QColor> &contribPalette() {
  static const QList<QColor> pal = {
      QColor(QStringLiteral("#ff2d95")), QColor(QStringLiteral("#22d3ee")),
      QColor(QStringLiteral("#e000ff")), QColor(QStringLiteral("#8b5cf6")),
      QColor(QStringLiteral("#14f1d9")), QColor(QStringLiteral("#ffb547")),
      QColor(QStringLiteral("#ff6b6b")), QColor(QStringLiteral("#34d399")),
  };
  return pal;
}

QString pillStyle(const QString &accent) {
  return QStringLiteral("QPushButton{background:rgba(42,32,60,0.6);border:1px "
                        "solid rgba(160,135,215,0.25);"
                        "border-radius:14px;padding:5px "
                        "16px;font-size:11px;color:#c4bcd6;font-weight:600;}"
                        "QPushButton:hover{background:rgba(60,46,86,0.8);"
                        "border-color:%1;color:#ffffff;}"
                        "QPushButton:checked{background:qlineargradient(x1:0,"
                        "y1:0,x2:1,y2:0,stop:0 %1,stop:1 #8b5cf6);"
                        "border:1px solid %1;color:#ffffff;font-weight:700;}")
      .arg(accent);
}

void applyChartBg(QChart *chart) {
  chart->setBackgroundVisible(false);
  chart->setPlotAreaBackgroundVisible(false);
  chart->setMargins(QMargins(6, 6, 6, 6));
}

void styleAxis(QAbstractAxis *ax) {
  ax->setLabelsFont(QFont(QStringLiteral("Segoe UI"), 8));
  ax->setLabelsColor(QColor(QStringLiteral("#9d93b8")));
  ax->setLinePen(QPen(QColor(160, 140, 215, 60), 1));
  ax->setGridLinePen(QPen(QColor(255, 255, 255, 20), 1, Qt::DashLine));
}

} // namespace

// ============================================================================
// DashboardWidget::DashboardWidget & setupUi
// ============================================================================
DashboardWidget::DashboardWidget(QWidget *parent) : QWidget(parent) {
  setupUi();
}

void DashboardWidget::setupUi() {
  setStyleSheet(QStringLiteral("background-color:#13111c;"));

  auto *scroll = new QScrollArea(this);
  scroll->setWidgetResizable(true);
  scroll->setFrameShape(QFrame::NoFrame);
  scroll->setStyleSheet(QStringLiteral(
      "QScrollArea{background:transparent;border:none;}"
      "QScrollBar:vertical{background:#171422;width:8px;border:none;margin:0;}"
      "QScrollBar::handle:vertical{background:#3d3257;border-radius:4px;min-"
      "height:24px;}"
      "QScrollBar::handle:vertical:hover{background:#8b5cf6;}"
      "QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{height:0;"
      "}"));

  auto *container = new QWidget(scroll);
  container->setStyleSheet(QStringLiteral("background:transparent;"));
  scroll->setWidget(container);

  auto *outerLay = new QVBoxLayout(this);
  outerLay->setContentsMargins(0, 0, 0, 0);
  outerLay->setSpacing(0);
  outerLay->addWidget(scroll);

  auto *mainLay = new QVBoxLayout(container);
  mainLay->setContentsMargins(28, 24, 28, 28);
  mainLay->setSpacing(20);

  // --- Header ---
  auto *headerCard = makeGlassCard(container);
  headerCard->setStyleSheet(
      QStringLiteral("QFrame#glassCard{"
                     "background:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 "
                     "rgba(52,38,78,0.85),stop:1 rgba(34,25,52,0.85));"
                     "border:1px solid rgba(170,145,225,0.25);border-left:4px "
                     "solid #ff2d95;border-radius:14px;}"));
  auto *headerLay = new QHBoxLayout(headerCard);
  headerLay->setContentsMargins(20, 14, 20, 14);
  headerLay->setSpacing(14);

  auto *badgeLbl =
      new QLabel(QStringLiteral("◈ REPOSITORY DASHBOARD"), headerCard);
  badgeLbl->setStyleSheet(QStringLiteral(
      "color:#ff2d95;background:rgba(255,45,149,0.15);border:1px solid "
      "rgba(255,45,149,0.5);"
      "border-radius:10px;padding:3px "
      "10px;font-size:10px;font-weight:700;letter-spacing:0.8px;"));
  headerLay->addWidget(badgeLbl);

  m_repoPathLabel =
      new QLabel(QStringLiteral("No repository loaded"), headerCard);
  m_repoPathLabel->setStyleSheet(
      QStringLiteral("color:#ffffff;font-size:15px;font-weight:700;"));
  headerLay->addWidget(m_repoPathLabel, 1);

  m_branchBadge = new QLabel(QString::fromUtf8("\xe2\x80\x94"), headerCard);
  m_branchBadge->setStyleSheet(QStringLiteral(
      "color:#22d3ee;background:rgba(34,211,238,0.12);border:1px solid "
      "rgba(34,211,238,0.45);"
      "border-radius:10px;padding:3px "
      "10px;font-size:11px;font-family:Consolas,monospace;font-weight:700;"));
  headerLay->addWidget(m_branchBadge);

  m_statusPill = new QLabel(QStringLiteral("Unloaded"), headerCard);
  m_statusPill->setStyleSheet(
      QStringLiteral("color:#8c82a5;background:rgba(140,130,165,0.12);border:"
                     "1px solid rgba(140,130,165,0.25);border-radius:10px;"
                     "padding:3px 10px;font-size:11px;font-weight:600;"));
  headerLay->addWidget(m_statusPill);
  mainLay->addWidget(headerCard);

  // --- KPI Row ---
  auto *kpiGrid = new QHBoxLayout();
  kpiGrid->setContentsMargins(0, 0, 0, 0);
  kpiGrid->setSpacing(16);
  m_commitsCard =
      new KpiCard(QStringLiteral("⎇"), QStringLiteral("TOTAL COMMITS"),
                  QColor(QStringLiteral("#ff2d95")), container);
  m_contributorsCard =
      new KpiCard(QStringLiteral("👥"), QStringLiteral("CONTRIBUTORS"),
                  QColor(QStringLiteral("#22d3ee")), container);
  m_filesCard =
      new KpiCard(QStringLiteral("📁"), QStringLiteral("TRACKED FILES"),
                  QColor(QStringLiteral("#8b5cf6")), container);
  m_workingTreeCard =
      new KpiCard(QStringLiteral("🛡️"), QStringLiteral("WORKING TREE"),
                  QColor(QStringLiteral("#34d399")), container);
  kpiGrid->addWidget(m_commitsCard);
  kpiGrid->addWidget(m_contributorsCard);
  kpiGrid->addWidget(m_filesCard);
  kpiGrid->addWidget(m_workingTreeCard);
  mainLay->addLayout(kpiGrid);

  // --- Commit Activity (full width) ---
  auto *commitCard = makeGlassCard(container);
  auto *commitLay = new QVBoxLayout(commitCard);
  commitLay->setContentsMargins(22, 18, 22, 18);
  commitLay->setSpacing(10);

  auto *cHeaderRow = new QHBoxLayout();
  auto *cTitleCol = new QVBoxLayout();
  cTitleCol->setSpacing(2);
  cTitleCol->addWidget(makeSectionTitle(
      QStringLiteral("📈 Commit Activity Timeline"), commitCard));
  m_commitChartSubLabel = makeSectionSub(
      QStringLiteral("Daily commit frequency across selected time window"),
      commitCard);
  cTitleCol->addWidget(m_commitChartSubLabel);
  cHeaderRow->addLayout(cTitleCol, 1);

  const QString tStyle = pillStyle(QStringLiteral("#ff2d95"));
  m_btnWindow30 = new QPushButton(QStringLiteral("30 Days"), commitCard);
  m_btnWindow30->setCheckable(true);
  m_btnWindow30->setChecked(true);
  m_btnWindow30->setCursor(Qt::PointingHandCursor);
  m_btnWindow30->setStyleSheet(tStyle);
  connect(m_btnWindow30, &QPushButton::clicked, this,
          &DashboardWidget::onWindow30Clicked);

  m_btnWindow90 = new QPushButton(QStringLiteral("90 Days"), commitCard);
  m_btnWindow90->setCheckable(true);
  m_btnWindow90->setChecked(false);
  m_btnWindow90->setCursor(Qt::PointingHandCursor);
  m_btnWindow90->setStyleSheet(tStyle);
  connect(m_btnWindow90, &QPushButton::clicked, this,
          &DashboardWidget::onWindow90Clicked);

  cHeaderRow->addWidget(m_btnWindow30);
  cHeaderRow->addWidget(m_btnWindow90);
  commitLay->addLayout(cHeaderRow);

  m_commitChartView = new ActivityChartView(commitCard);
  m_commitChartView->setMinimumHeight(220);
  m_commitChartView->setSizePolicy(QSizePolicy::Expanding,
                                   QSizePolicy::Expanding);
  m_commitChartView->setStyleSheet(
      QStringLiteral("background:transparent;border:none;"));
  commitLay->addWidget(m_commitChartView);

  m_commitChartEmptyLabel = new QLabel(
      QStringLiteral("Open a Git repository to view commit activity."),
      commitCard);
  m_commitChartEmptyLabel->setAlignment(Qt::AlignCenter);
  m_commitChartEmptyLabel->setStyleSheet(
      QStringLiteral("color:#8c82a5;font-size:12px;font-style:italic;"));
  m_commitChartEmptyLabel->setMinimumHeight(220);
  commitLay->addWidget(m_commitChartEmptyLabel);
  mainLay->addWidget(commitCard);

  // --- Middle Row: Contributor Distribution + Activity Over Time ---
  auto *middleRow = new QHBoxLayout();
  middleRow->setContentsMargins(0, 0, 0, 0);
  middleRow->setSpacing(16);

  // Contributor Distribution Card
  auto *contribCard = makeGlassCard(container);
  auto *contribLay = new QVBoxLayout(contribCard);
  contribLay->setContentsMargins(22, 18, 22, 18);
  contribLay->setSpacing(10);
  auto *ctCol = new QVBoxLayout();
  ctCol->setSpacing(2);
  ctCol->addWidget(makeSectionTitle(
      QStringLiteral("📊 Contributor Distribution"), contribCard));
  ctCol->addWidget(makeSectionSub(
      QStringLiteral("Commit volume by verified contributor identity"),
      contribCard));
  contribLay->addLayout(ctCol);

  m_contribChartsWidget = new QWidget(contribCard);
  m_contribChartsWidget->setSizePolicy(QSizePolicy::Expanding,
                                       QSizePolicy::Preferred);
  auto *chartsHLay = new QHBoxLayout(m_contribChartsWidget);
  chartsHLay->setContentsMargins(0, 0, 0, 0);
  chartsHLay->setSpacing(8);

  m_contribDonutView = new QChartView(m_contribChartsWidget);
  m_contribDonutView->setRenderHint(QPainter::Antialiasing);
  m_contribDonutView->setMinimumHeight(180);
  m_contribDonutView->setStyleSheet(
      QStringLiteral("background:transparent;border:none;"));
  chartsHLay->addWidget(m_contribDonutView, 1);

  m_contribBarView = new QChartView(m_contribChartsWidget);
  m_contribBarView->setRenderHint(QPainter::Antialiasing);
  m_contribBarView->setMinimumHeight(180);
  m_contribBarView->setStyleSheet(
      QStringLiteral("background:transparent;border:none;"));
  chartsHLay->addWidget(m_contribBarView, 1);

  contribLay->addWidget(m_contribChartsWidget);

  m_contribLegendWidget = new QWidget(contribCard);
  m_contribLegendLayout = new QVBoxLayout(m_contribLegendWidget);
  m_contribLegendLayout->setContentsMargins(0, 4, 0, 0);
  m_contribLegendLayout->setSpacing(4);
  contribLay->addWidget(m_contribLegendWidget);

  m_contributorChartEmptyLabel =
      new QLabel(QStringLiteral("No contributor data available."), contribCard);
  m_contributorChartEmptyLabel->setAlignment(Qt::AlignCenter);
  m_contributorChartEmptyLabel->setStyleSheet(
      QStringLiteral("color:#8c82a5;font-size:12px;font-style:italic;"));
  m_contributorChartEmptyLabel->setMinimumHeight(180);
  contribLay->addWidget(m_contributorChartEmptyLabel);
  middleRow->addWidget(contribCard, 5);

  // Contributor Activity Over Time Card
  auto *actCard = makeGlassCard(container);
  auto *actLay = new QVBoxLayout(actCard);
  actLay->setContentsMargins(22, 18, 22, 18);
  actLay->setSpacing(10);
  auto *atCol = new QVBoxLayout();
  atCol->setSpacing(2);
  atCol->addWidget(makeSectionTitle(
      QStringLiteral("📉 Contributor Activity Over Time"), actCard));
  atCol->addWidget(makeSectionSub(
      QStringLiteral("Daily commits per contributor across selected window"),
      actCard));
  actLay->addLayout(atCol);

  m_contribActivityView = new QChartView(actCard);
  m_contribActivityView->setRenderHint(QPainter::Antialiasing);
  m_contribActivityView->setMinimumHeight(200);
  m_contribActivityView->setSizePolicy(QSizePolicy::Expanding,
                                       QSizePolicy::Expanding);
  m_contribActivityView->setStyleSheet(
      QStringLiteral("background:transparent;border:none;"));
  actLay->addWidget(m_contribActivityView);

  m_contribActivityEmptyLabel =
      new QLabel(QStringLiteral("No activity data available."), actCard);
  m_contribActivityEmptyLabel->setAlignment(Qt::AlignCenter);
  m_contribActivityEmptyLabel->setStyleSheet(
      QStringLiteral("color:#8c82a5;font-size:12px;font-style:italic;"));
  m_contribActivityEmptyLabel->setMinimumHeight(200);
  actLay->addWidget(m_contribActivityEmptyLabel);
  middleRow->addWidget(actCard, 5);
  mainLay->addLayout(middleRow);

  // --- Bottom Row: Hotspots + Recent Commits ---
  auto *bottomRow = new QHBoxLayout();
  bottomRow->setContentsMargins(0, 0, 0, 0);
  bottomRow->setSpacing(16);

  auto *hotCard = makeGlassCard(container);
  auto *hotLay = new QVBoxLayout(hotCard);
  hotLay->setContentsMargins(22, 18, 22, 18);
  hotLay->setSpacing(12);
  auto *hotTitleCol = new QVBoxLayout();
  hotTitleCol->setSpacing(2);
  hotTitleCol->addWidget(makeSectionTitle(
      QStringLiteral("🔥 Top File Hotspots (Churn)"), hotCard));
  hotTitleCol->addWidget(makeSectionSub(
      QStringLiteral("Files modified most frequently across history"),
      hotCard));
  hotLay->addLayout(hotTitleCol);

  m_hotspotsContainerLayout = new QVBoxLayout();
  m_hotspotsContainerLayout->setContentsMargins(0, 0, 0, 0);
  m_hotspotsContainerLayout->setSpacing(6);
  m_hotspotsEmptyLabel =
      new QLabel(QStringLiteral("No file churn data available."), hotCard);
  m_hotspotsEmptyLabel->setAlignment(Qt::AlignCenter);
  m_hotspotsEmptyLabel->setStyleSheet(
      QStringLiteral("color:#8c82a5;font-size:12px;font-style:italic;"));
  m_hotspotsContainerLayout->addWidget(m_hotspotsEmptyLabel);
  hotLay->addLayout(m_hotspotsContainerLayout);
  hotLay->addStretch(1);
  bottomRow->addWidget(hotCard, 1);

  auto *recentCard = makeGlassCard(container);
  auto *recentLay = new QVBoxLayout(recentCard);
  recentLay->setContentsMargins(22, 18, 22, 18);
  recentLay->setSpacing(12);
  auto *recTitleCol = new QVBoxLayout();
  recTitleCol->setSpacing(2);
  recTitleCol->addWidget(
      makeSectionTitle(QStringLiteral("🕒 Recent Revisions"), recentCard));
  recTitleCol->addWidget(makeSectionSub(
      QStringLiteral("Latest committed revisions with verified identities"),
      recentCard));
  recentLay->addLayout(recTitleCol);

  m_recentCommitsTable = new QTableWidget(0, 4, recentCard);
  m_recentCommitsTable->setObjectName(QStringLiteral("commitsTablePrism"));
  m_recentCommitsTable->setHorizontalHeaderLabels(
      {QStringLiteral("HASH"), QStringLiteral("MESSAGE"),
       QStringLiteral("AUTHOR"), QStringLiteral("DATE")});
  m_recentCommitsTable->horizontalHeader()->setSectionResizeMode(
      0, QHeaderView::ResizeToContents);
  m_recentCommitsTable->horizontalHeader()->setSectionResizeMode(
      1, QHeaderView::Stretch);
  m_recentCommitsTable->horizontalHeader()->setSectionResizeMode(
      2, QHeaderView::ResizeToContents);
  m_recentCommitsTable->horizontalHeader()->setSectionResizeMode(
      3, QHeaderView::ResizeToContents);
  m_recentCommitsTable->verticalHeader()->setVisible(false);
  m_recentCommitsTable->verticalHeader()->setDefaultSectionSize(32);
  m_recentCommitsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_recentCommitsTable->setSelectionMode(QAbstractItemView::SingleSelection);
  m_recentCommitsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
  m_recentCommitsTable->setShowGrid(false);
  m_recentCommitsTable->setAlternatingRowColors(true);
  m_recentCommitsTable->setMinimumHeight(180);
  m_recentCommitsTable->setStyleSheet(QStringLiteral(
      "QTableWidget{background:transparent;alternate-background-color:rgba(36,"
      "28,52,0.45);"
      "color:#f1f5f9;border:1px solid "
      "rgba(150,125,200,0.18);border-radius:10px;font-size:12px;"
      "gridline-color:transparent;selection-background-color:rgba(139,92,246,0."
      "32);selection-color:#ffffff;outline:none;}"
      "QTableWidget "
      "QHeaderView::section{background:rgba(32,25,48,0.85);color:#a89ec0;font-"
      "size:10px;font-weight:700;"
      "letter-spacing:0.5px;padding:9px 12px;border:none;border-right:1px "
      "solid rgba(150,125,200,0.15);border-bottom:2px solid #ff2d95;}"
      "QTableWidget::item{padding:8px 12px;border:none;}"
      "QTableWidget::item:selected{background:rgba(139,92,246,0.32);color:#"
      "ffffff;}"));
  recentLay->addWidget(m_recentCommitsTable);
  bottomRow->addWidget(recentCard, 1);
  mainLay->addLayout(bottomRow);
}

// ============================================================================
// Slots
// ============================================================================
void DashboardWidget::onWindow30Clicked() {
  m_currentWindowDays = 30;
  m_btnWindow30->setChecked(true);
  m_btnWindow90->setChecked(false);
  updateCommitChart();
  updateContributorActivityChart(m_cachedCommits, m_cachedContributors);
}

void DashboardWidget::onWindow90Clicked() {
  m_currentWindowDays = 90;
  m_btnWindow30->setChecked(false);
  m_btnWindow90->setChecked(true);
  updateCommitChart();
  updateContributorActivityChart(m_cachedCommits, m_cachedContributors);
}

// ============================================================================
// updateData
// ============================================================================
void DashboardWidget::updateData(const RepositoryHealthData &health,
                                 const GitAnalyticsService &analytics,
                                 const QString &repoPath,
                                 const QString &branch) {
  m_hasLoadedRepo = true;
  m_cachedCommits = analytics.commits();
  m_cachedContributors = analytics.contributors();

  const QString repoName = QFileInfo(repoPath).fileName();
  m_repoPathLabel->setText(
      repoName.isEmpty() ? QDir::toNativeSeparators(repoPath)
                         : QStringLiteral("%1 %2 %3")
                               .arg(repoName, QString::fromUtf8("\xe2\x80\x94"),
                                    QDir::toNativeSeparators(repoPath)));
  m_branchBadge->setText(branch.isEmpty() ? QStringLiteral("HEAD") : branch);

  if (health.workingTreeStatus.contains(QStringLiteral("Clean"),
                                        Qt::CaseInsensitive)) {
    m_statusPill->setText(QString::fromUtf8("\xe2\x97\x8f Clean Tree"));
    m_statusPill->setStyleSheet(
        QStringLiteral("color:#34d399;background:rgba(52,211,153,0.14);border:"
                       "1px solid rgba(52,211,153,0.45);border-radius:10px;"
                       "padding:3px 10px;font-size:11px;font-weight:700;"));
  } else {
    m_statusPill->setText(
        QStringLiteral("Changes: %1").arg(health.workingTreeStatus));
    m_statusPill->setStyleSheet(
        QStringLiteral("color:#fb923c;background:rgba(251,146,60,0.14);border:"
                       "1px solid rgba(251,146,60,0.45);border-radius:10px;"
                       "padding:3px 10px;font-size:11px;font-weight:700;"));
  }

  updateKpiCards(health, analytics);
  updateCommitChart();
  updateContributorChart(analytics.contributors());
  updateContributorActivityChart(m_cachedCommits, m_cachedContributors);
  updateHotspotsList(analytics.hotspots());
  updateRecentCommitsTable(analytics.commits());
}

// ============================================================================
// updateKpiCards
// ============================================================================
void DashboardWidget::updateKpiCards(const RepositoryHealthData &health,
                                     const GitAnalyticsService &analytics) {
  const int totalRevs = health.totalCommits;
  const int loadedRevs = analytics.commits().size();
  if (totalRevs > 0) {
    m_commitsCard->setValue(QString::number(totalRevs));
    m_commitsCard->setSubText(
        totalRevs > loadedRevs
            ? QStringLiteral("%1 commits analyzed (capped at 500)")
                  .arg(loadedRevs)
            : QStringLiteral("Full history analyzed (%1 commits)")
                  .arg(totalRevs));
  } else {
    m_commitsCard->setValue(QStringLiteral("0"));
    m_commitsCard->setSubText(QStringLiteral("No commits in repository"));
  }
  const int cnt = analytics.contributors().size();
  m_contributorsCard->setValue(QString::number(cnt));
  m_contributorsCard->setSubText(
      cnt == 1 ? QStringLiteral("1 distinct author")
               : QStringLiteral("%1 distinct authors").arg(cnt));
  m_filesCard->setValue(QString::number(health.trackedFiles));
  m_filesCard->setSubText(QStringLiteral("Versioned in active branch"));
  m_workingTreeCard->setValue(health.workingTreeStatus);
  m_workingTreeCard->setSubText(health.workingTreeDetail);
}

// ============================================================================
// updateCommitChart (P1+P2 fix)
// ============================================================================
void DashboardWidget::updateCommitChart() {
  if (!m_hasLoadedRepo || m_cachedCommits.isEmpty()) {
    m_commitChartView->hide();
    m_commitChartEmptyLabel->show();
    m_commitChartSubLabel->setText(
        QStringLiteral("No commit activity in selected time window"));
    return;
  }
  const CommitActivityData activity =
      GitAnalyticsService::aggregateCommitActivity(m_cachedCommits,
                                                   m_currentWindowDays);
  if (activity.buckets.isEmpty()) {
    m_commitChartView->hide();
    m_commitChartEmptyLabel->show();
    m_commitChartSubLabel->setText(
        QStringLiteral("No commit activity in selected time window"));
    return;
  }
  m_commitChartEmptyLabel->hide();
  m_commitChartView->show();

  const int n = activity.buckets.size();
  int maxCount = 0;

  auto *lineSeries = new QLineSeries();
  QPen lp(QColor(QStringLiteral("#ff2d95")), 2.6, Qt::SolidLine, Qt::RoundCap,
          Qt::RoundJoin);
  lineSeries->setPen(lp);
  for (int i = 0; i < n; ++i) {
    const int c = activity.buckets[i].commitCount;
    lineSeries->append(i, c);
    if (c > maxCount)
      maxCount = c;
  }

  auto *areaSeries = new QAreaSeries(lineSeries);
  areaSeries->setName(QStringLiteral("Commits"));
  QLinearGradient grad(QPointF(0, 0), QPointF(0, 1));
  grad.setCoordinateMode(QGradient::ObjectMode);
  grad.setColorAt(0.00, QColor(255, 45, 149, 180));
  grad.setColorAt(0.35, QColor(224, 0, 255, 115));
  grad.setColorAt(0.68, QColor(139, 92, 246, 55));
  grad.setColorAt(0.90, QColor(34, 211, 238, 20));
  grad.setColorAt(1.00, QColor(19, 17, 28, 0));
  areaSeries->setBrush(grad);
  areaSeries->setPen(lp);

  auto *chart = new QChart();
  chart->addSeries(areaSeries);
  applyChartBg(chart);
  chart->legend()->hide();

  auto *axisX = new QValueAxis();
  axisX->setRange(0, n - 1);
  axisX->setTickCount(std::min<int>(n, m_currentWindowDays <= 30 ? 7 : 10));
  axisX->setLabelFormat(QStringLiteral("%d"));
  styleAxis(axisX);
  if (n >= 2) {
    axisX->setTitleText(QStringLiteral("%1 %2 %3")
                            .arg(activity.buckets.first().date.toString(
                                     QStringLiteral("d MMM")),
                                 QString::fromUtf8("\xe2\x80\x93"),
                                 activity.buckets.last().date.toString(
                                     QStringLiteral("d MMM yyyy"))));
    axisX->setTitleFont(QFont(QStringLiteral("Segoe UI"), 8));
    axisX->setTitleBrush(QBrush(QColor(QStringLiteral("#a59abf"))));
  }
  chart->addAxis(axisX, Qt::AlignBottom);
  areaSeries->attachAxis(axisX);

  auto *axisY = new QValueAxis();
  axisY->setRange(0, std::max(1, maxCount + 1));
  axisY->setLabelFormat(QStringLiteral("%d"));
  axisY->setTickCount(std::min(maxCount + 2, 5));
  styleAxis(axisY);
  chart->addAxis(axisY, Qt::AlignLeft);
  areaSeries->attachAxis(axisY);

  m_commitChartView->setChart(chart);
  m_commitChartView->setActivityData(activity, lineSeries);

  QString sub = QStringLiteral("%1 commits across the last %2 days")
                    .arg(activity.totalCommits)
                    .arg(m_currentWindowDays);
  if (activity.isTruncated)
    sub += QStringLiteral(" (earlier days capped by 500-commit query limit)");
  m_commitChartSubLabel->setText(sub);
}

// ============================================================================
// updateContributorChart (P3: donut + bar simultaneously)
// ============================================================================
void DashboardWidget::updateContributorChart(
    const QList<ContributorRecord> &contributors) {
  QLayoutItem *li;
  while ((li = m_contribLegendLayout->takeAt(0)) != nullptr) {
    if (li->widget())
      li->widget()->deleteLater();
    delete li;
  }

  if (!m_hasLoadedRepo || contributors.isEmpty()) {
    m_contribChartsWidget->hide();
    m_contribLegendWidget->hide();
    m_contributorChartEmptyLabel->show();
    return;
  }
  m_contributorChartEmptyLabel->hide();
  m_contribChartsWidget->show();
  m_contribLegendWidget->show();

  const auto &pal = contribPalette();
  int total = 0;
  for (const auto &c : contributors)
    total += c.commits;
  if (total == 0)
    total = 1;

  // Donut
  auto *pieSeries = new QPieSeries();
  pieSeries->setHoleSize(0.52);
  pieSeries->setPieSize(0.82);
  for (int i = 0; i < contributors.size(); ++i) {
    const auto &c = contributors[i];
    const QColor color = pal[i % pal.size()];
    auto *slice = pieSeries->append(c.name, c.commits);
    slice->setColor(color);
    slice->setBorderColor(QColor(QStringLiteral("#1e192c")));
    slice->setBorderWidth(2);
    const double pct = static_cast<double>(c.commits) / total * 100.0;
    const QString tip =
        QStringLiteral("<div style='background-color:#1e182e; color:#f8fafc; "
                       "font-family:Segoe UI; padding:6px 10px; "
                       "border-radius:6px; border:1px solid #5a4c8a;'>"
                       "<b style='color:%1; font-size:13px;'>%2</b><br/>"
                       "<span style='color:#cbd5e1; font-size:12px;'>%3 "
                       "commits (%4%)</span></div>")
            .arg(color.name(), c.name)
            .arg(c.commits)
            .arg(pct, 0, 'f', 1);
    connect(slice, &QPieSlice::hovered, slice, [slice, tip](bool state) {
      slice->setExploded(state);
      slice->setExplodeDistanceFactor(state ? 0.08 : 0.0);
      if (state)
        QToolTip::showText(QCursor::pos(), tip);
      else
        QToolTip::hideText();
    });
  }
  auto *donutChart = new QChart();
  donutChart->addSeries(pieSeries);
  applyChartBg(donutChart);
  donutChart->legend()->hide();
  m_contribDonutView->setChart(donutChart);

  // Bar
  const int topCount = std::min<int>(6, contributors.size());
  auto *barSeries = new QBarSeries();
  QStringList cats;
  int maxC = 0;
  for (int i = 0; i < topCount; ++i) {
    const auto &c = contributors[i];
    cats << (c.name.length() > 14 ? c.name.left(12) + QStringLiteral("..")
                                  : c.name);
    if (c.commits > maxC)
      maxC = c.commits;
  }
  for (int i = 0; i < topCount; ++i) {
    const auto &c = contributors[i];
    auto *set = new QBarSet(c.name);
    set->setColor(pal[i % pal.size()]);
    set->setBorderColor(pal[i % pal.size()].lighter(130));
    for (int j = 0; j < topCount; ++j)
      *set << (j == i ? c.commits : 0);
    barSeries->append(set);
  }
  barSeries->setLabelsVisible(true);
  barSeries->setLabelsPosition(QAbstractBarSeries::LabelsOutsideEnd);

  auto *barChart = new QChart();
  barChart->addSeries(barSeries);
  applyChartBg(barChart);
  barChart->legend()->hide();

  auto *axisXBar = new QBarCategoryAxis();
  axisXBar->append(cats);
  styleAxis(axisXBar);
  barChart->addAxis(axisXBar, Qt::AlignBottom);
  barSeries->attachAxis(axisXBar);

  auto *axisYBar = new QValueAxis();
  axisYBar->setRange(0, std::max(1, maxC + 2));
  axisYBar->setLabelFormat(QStringLiteral("%d"));
  axisYBar->setTickCount(std::min(maxC + 2, 5));
  styleAxis(axisYBar);
  barChart->addAxis(axisYBar, Qt::AlignLeft);
  barSeries->attachAxis(axisYBar);
  m_contribBarView->setChart(barChart);

  // Legend
  for (int i = 0; i < contributors.size(); ++i) {
    const auto &c = contributors[i];
    const QColor color = pal[i % pal.size()];
    const double pct = static_cast<double>(c.commits) / total * 100.0;

    auto *legRow = new QWidget(m_contribLegendWidget);
    auto *legLay = new QHBoxLayout(legRow);
    legLay->setContentsMargins(6, 2, 6, 2);
    legLay->setSpacing(8);

    auto *dot = new QLabel(QString::fromUtf8("\xe2\x97\x8f"), legRow);
    dot->setStyleSheet(
        QStringLiteral("color:%1;font-size:14px;").arg(color.name()));
    legLay->addWidget(dot);

    auto *nameLbl = new QLabel(c.name, legRow);
    nameLbl->setStyleSheet(
        QStringLiteral("color:#f8fafc;font-weight:600;font-size:11px;"));
    legLay->addWidget(nameLbl, 1);

    auto *statsLbl = new QLabel(
        QStringLiteral("%1 commits (%2%)").arg(c.commits).arg(pct, 0, 'f', 1),
        legRow);
    statsLbl->setStyleSheet(
        QStringLiteral("color:%1;font-weight:700;font-size:11px;")
            .arg(color.name()));
    legLay->addWidget(statsLbl);
    m_contribLegendLayout->addWidget(legRow);
  }
}

// ============================================================================
// updateContributorActivityChart (New — P2)
// ============================================================================
void DashboardWidget::updateContributorActivityChart(
    const QList<CommitRecord> &commits,
    const QList<ContributorRecord> &contributors) {
  if (!m_hasLoadedRepo || commits.isEmpty() || contributors.isEmpty()) {
    m_contribActivityView->hide();
    m_contribActivityEmptyLabel->show();
    return;
  }

  const QDate endDate = QDate::currentDate();
  const QDate startDate = endDate.addDays(-m_currentWindowDays + 1);

  QMap<QString, QMap<QDate, int>> actMap;
  for (const auto &commit : commits) {
    const QDate d = QDate::fromString(commit.date.left(10), Qt::ISODate);
    if (!d.isValid() || d < startDate || d > endDate)
      continue;
    const QString canon = GitAnalyticsService::canonicalContributorName(
        commit.author, commit.authorEmail);
    actMap[canon][d]++;
  }

  if (actMap.isEmpty()) {
    m_contribActivityView->hide();
    m_contribActivityEmptyLabel->show();
    return;
  }
  m_contribActivityEmptyLabel->hide();
  m_contribActivityView->show();

  const auto &pal = contribPalette();
  auto *chart = new QChart();
  applyChartBg(chart);
  chart->legend()->show();
  chart->legend()->setAlignment(Qt::AlignBottom);
  chart->legend()->setFont(QFont(QStringLiteral("Segoe UI"), 8));
  chart->legend()->setLabelColor(QColor(QStringLiteral("#c4bcd6")));

  int maxY = 1;
  int ci = 0;

  // Build ordered name list matching contributor order for color consistency
  QStringList ordered;
  for (const auto &cr : contributors)
    ordered << cr.name;
  for (const auto &name : actMap.keys()) {
    if (!ordered.contains(name))
      ordered << name;
  }

  // Capture window parameters for lambda (avoid dangling ref)
  const int windowDays = m_currentWindowDays;
  const QDate sDate = startDate;

  for (const QString &cname : ordered) {
    if (!actMap.contains(cname))
      continue;
    const auto &dateMap = actMap[cname];
    const QColor color = pal[ci % pal.size()];

    auto *series = new QSplineSeries();
    series->setName(cname);
    QPen pen(color, 2.4);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    series->setPen(pen);

    for (int d = 0; d < windowDays; ++d) {
      const QDate day = sDate.addDays(d);
      const int cnt = dateMap.value(day, 0);
      series->append(d, cnt);
      if (cnt > maxY)
        maxY = cnt;
    }

    connect(
        series, &QSplineSeries::hovered, series,
        [series, sDate, cname, color, windowDays](const QPointF &pt,
                                                  bool state) {
          if (!state) {
            QToolTip::hideText();
            return;
          }
          const int idx = qRound(pt.x());
          if (idx < 0 || idx >= windowDays)
            return;
          const QDate day = sDate.addDays(idx);
          const int cnt = qRound(pt.y());
          const QString tip =
              QStringLiteral(
                  "<div style='background-color:#1e182e; color:#f8fafc; "
                  "font-family:Segoe UI; padding:6px 10px; border-radius:6px; "
                  "border:1px solid #5a4c8a;'>"
                  "<b style='color:%1; font-size:13px;'>%2</b><br/>"
                  "<span style='color:#a59abf; font-size:11px;'>%3</span><br/>"
                  "<b style='color:#ffffff; font-size:14px;'>%4 %5</b></div>")
                  .arg(color.name(), cname,
                       day.toString(QStringLiteral("d MMM yyyy")))
                  .arg(cnt)
                  .arg(cnt == 1 ? QStringLiteral("commit")
                                : QStringLiteral("commits"));
          QToolTip::showText(QCursor::pos(), tip);
        });

    chart->addSeries(series);
    ++ci;
  }

  if (chart->series().isEmpty()) {
    delete chart;
    m_contribActivityView->hide();
    m_contribActivityEmptyLabel->show();
    return;
  }

  auto *axisX = new QValueAxis();
  axisX->setRange(0, windowDays - 1);
  axisX->setTickCount(std::min<int>(windowDays, 7));
  axisX->setLabelFormat(QStringLiteral("%d"));
  if (windowDays >= 2) {
    axisX->setTitleText(
        QStringLiteral("%1 %2 %3")
            .arg(sDate.toString(QStringLiteral("d MMM")),
                 QString::fromUtf8("\xe2\x80\x93"),
                 endDate.toString(QStringLiteral("d MMM yyyy"))));
    axisX->setTitleFont(QFont(QStringLiteral("Segoe UI"), 8));
    axisX->setTitleBrush(QBrush(QColor(QStringLiteral("#a59abf"))));
  }
  styleAxis(axisX);
  chart->addAxis(axisX, Qt::AlignBottom);

  auto *axisY = new QValueAxis();
  axisY->setRange(0, maxY + 1);
  axisY->setLabelFormat(QStringLiteral("%d"));
  axisY->setTickCount(std::min(maxY + 2, 5));
  styleAxis(axisY);
  chart->addAxis(axisY, Qt::AlignLeft);

  for (auto *s : chart->series()) {
    s->attachAxis(axisX);
    s->attachAxis(axisY);
  }
  m_contribActivityView->setChart(chart);
}

// ============================================================================
// updateHotspotsList
// ============================================================================
void DashboardWidget::updateHotspotsList(const QList<HotspotRecord> &hotspots) {
  QLayoutItem *item;
  while ((item = m_hotspotsContainerLayout->takeAt(0)) != nullptr) {
    if (item->widget())
      item->widget()->deleteLater();
    delete item;
  }
  if (!m_hasLoadedRepo || hotspots.isEmpty()) {
    m_hotspotsEmptyLabel =
        new QLabel(QStringLiteral("No file churn data available."), this);
    m_hotspotsEmptyLabel->setAlignment(Qt::AlignCenter);
    m_hotspotsEmptyLabel->setStyleSheet(
        QStringLiteral("color:#8c82a5;font-size:12px;font-style:italic;"));
    m_hotspotsContainerLayout->addWidget(m_hotspotsEmptyLabel);
    return;
  }
  const int topCount = std::min<int>(5, hotspots.size());
  const int maxChurn =
      hotspots.first().commits > 0 ? hotspots.first().commits : 1;
  for (int i = 0; i < topCount; ++i) {
    auto *row = new HotspotRowWidget(i + 1, hotspots[i], maxChurn, this);
    m_hotspotsContainerLayout->addWidget(row);
  }
}

// ============================================================================
// updateRecentCommitsTable
// ============================================================================
void DashboardWidget::updateRecentCommitsTable(
    const QList<CommitRecord> &commits) {
  m_recentCommitsTable->setRowCount(0);
  if (!m_hasLoadedRepo || commits.isEmpty()) {
    m_recentCommitsTable->setRowCount(1);
    auto *msg = new QTableWidgetItem(
        QStringLiteral("No commits found in this repository."));
    msg->setFlags(Qt::ItemIsEnabled);
    msg->setTextAlignment(Qt::AlignCenter);
    msg->setForeground(QColor(QStringLiteral("#8c82a5")));
    m_recentCommitsTable->setItem(0, 0, msg);
    m_recentCommitsTable->setSpan(0, 0, 1, 4);
    return;
  }
  const int count = std::min<int>(5, commits.size());
  m_recentCommitsTable->setRowCount(count);
  QFont monoFont(QStringLiteral("Consolas"));
  monoFont.setPointSize(9);
  for (int r = 0; r < count; ++r) {
    const auto &c = commits[r];
    auto *hashItem = new QTableWidgetItem(c.hash);
    hashItem->setFont(monoFont);
    hashItem->setForeground(QColor(QStringLiteral("#ff2d95")));
    hashItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    auto *subjItem = new QTableWidgetItem(c.subject);
    subjItem->setForeground(QColor(QStringLiteral("#f8fafc")));
    subjItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    subjItem->setToolTip(c.subject);

    const QString resolved =
        GitAnalyticsService::canonicalContributorName(c.author, c.authorEmail);
    auto *authorItem = new QTableWidgetItem(resolved);
    if (resolved == QStringLiteral("Tanuja 658"))
      authorItem->setForeground(QColor(QStringLiteral("#22d3ee")));
    else if (resolved.contains(QStringLiteral("sneha"), Qt::CaseInsensitive))
      authorItem->setForeground(QColor(QStringLiteral("#34d399")));
    else
      authorItem->setForeground(QColor(QStringLiteral("#a855f7")));
    authorItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    if (c.author != resolved && !c.author.isEmpty()) {
      authorItem->setToolTip(
          QStringLiteral("Original: %1%2")
              .arg(c.author, c.authorEmail.isEmpty()
                                 ? QString()
                                 : QStringLiteral(" <%1>").arg(c.authorEmail)));
    }

    auto *dateItem = new QTableWidgetItem(c.date);
    dateItem->setForeground(QColor(QStringLiteral("#8c82a5")));
    dateItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    m_recentCommitsTable->setItem(r, 0, hashItem);
    m_recentCommitsTable->setItem(r, 1, subjItem);
    m_recentCommitsTable->setItem(r, 2, authorItem);
    m_recentCommitsTable->setItem(r, 3, dateItem);
  }
}

// ============================================================================
// clearData
// ============================================================================
void DashboardWidget::clearData() {
  m_hasLoadedRepo = false;
  m_cachedCommits.clear();
  m_cachedContributors.clear();

  m_repoPathLabel->setText(QStringLiteral("No repository loaded"));
  m_branchBadge->setText(QString::fromUtf8("\xe2\x80\x94"));
  m_statusPill->setText(QStringLiteral("Unloaded"));
  m_statusPill->setStyleSheet(
      QStringLiteral("color:#8c82a5;background:rgba(140,130,165,0.12);border:"
                     "1px solid rgba(140,130,165,0.25);border-radius:10px;"
                     "padding:3px 10px;font-size:11px;font-weight:600;"));

  m_commitsCard->setValue(QString::fromUtf8("\xe2\x80\x94"));
  m_commitsCard->setSubText(QStringLiteral("No repository open"));
  m_contributorsCard->setValue(QString::fromUtf8("\xe2\x80\x94"));
  m_contributorsCard->setSubText(QStringLiteral("No repository open"));
  m_filesCard->setValue(QString::fromUtf8("\xe2\x80\x94"));
  m_filesCard->setSubText(QStringLiteral("No repository open"));
  m_workingTreeCard->setValue(QString::fromUtf8("\xe2\x80\x94"));
  m_workingTreeCard->setSubText(QStringLiteral("No repository open"));

  m_commitChartView->hide();
  m_commitChartEmptyLabel->show();
  m_commitChartSubLabel->setText(
      QStringLiteral("Daily commit frequency across selected time window"));

  m_contribChartsWidget->hide();
  m_contribLegendWidget->hide();
  m_contributorChartEmptyLabel->show();

  m_contribActivityView->hide();
  m_contribActivityEmptyLabel->show();

  QLayoutItem *item;
  while ((item = m_hotspotsContainerLayout->takeAt(0)) != nullptr) {
    if (item->widget())
      item->widget()->deleteLater();
    delete item;
  }
  m_hotspotsContainerLayout->addWidget(m_hotspotsEmptyLabel);
  m_hotspotsEmptyLabel->show();

  m_recentCommitsTable->setRowCount(0);
}
