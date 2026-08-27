/*
 * Copyright 2023 ZF Friedrichshafen AG
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include "RotarySwitchWidget.h"
#include "RotarySwitchDialogFrame.h"
#include "MdiWindowType.h"
#include "DragAndDrop.h"
#include "GetEventPos.h"
#include "QtIniFile.h"
#include "StringHelpers.h"

#include <QPainter>
#include <QPainterPath>
#include <QRadialGradient>
#include <QMimeData>
#include <QMenu>
#include <QAction>
#include <QContextMenuEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>

#include <cmath>

extern "C" {
#include "Config.h"
#include "Blackboard.h"
#include "BlackboardAccess.h"
}

// The knob sweeps from -135 degree (left stop) to +135 degree (right stop),
// 0 degree points straight up. Same convention as a real panel poti.
#define ROTARY_SWITCH_START_ANGLE  (-135.0)
#define ROTARY_SWITCH_END_ANGLE     (135.0)

#define ROTARY_SWITCH_MIN_DETENTS   2
#define ROTARY_SWITCH_MAX_DETENTS   64

RotarySwitchWidget::RotarySwitchWidget(QString par_WindowTitle, MdiSubWindow* par_SubWindow, MdiWindowType *par_Type, QWidget *parent) :
    MdiWindowWidget(par_WindowTitle, par_SubWindow, par_Type, parent),
    m_Vid(0),
    m_Value(0.0),
    m_Min(0.0),
    m_Max(100.0),
    m_DetentCount(5),
    m_SnapThreshold(3.0),
    m_SnapOnly(true),
    m_Dragging(false),
    m_Color(200, 200, 205),
    m_ObserverConnection(this)
{
    m_Font = font();

    setAcceptDrops(true);
    setFocusPolicy(Qt::StrongFocus);
    setMinimumSize(60, 60);

    readFromIni();
}

RotarySwitchWidget::~RotarySwitchWidget()
{
    writeToIni();
    DetachFromBlackboard();
}

bool RotarySwitchWidget::writeToIni()
{
    QString SectionPath = GetIniSectionPath();
    int Fd = ScQt_GetMainFileDescriptor();

    QString WindowType = GetMdiWindowType()->GetWindowTypeName();
    ScQt_IniFileDataBaseWriteString(SectionPath, "type", WindowType, Fd);
    ScQt_IniFileDataBaseWriteString(SectionPath, "variable", m_VariableName, Fd);
    ScQt_IniFileDataBaseWriteFloat(SectionPath, "min", m_Min, Fd);
    ScQt_IniFileDataBaseWriteFloat(SectionPath, "max", m_Max, Fd);
    ScQt_IniFileDataBaseWriteInt(SectionPath, "detents", m_DetentCount, Fd);
    ScQt_IniFileDataBaseWriteFloat(SectionPath, "snap_threshold", m_SnapThreshold, Fd);
    ScQt_IniFileDataBaseWriteYesNo(SectionPath, "snap_only", m_SnapOnly, Fd);
    // labels are separated by '|' so that a label itself may contain a comma
    QString Labels = m_DetentLabels.join(QLatin1Char('|'));
    ScQt_IniFileDataBaseWriteString(SectionPath, "labels", Labels, Fd);
    ScQt_IniFileDataBaseWriteInt(SectionPath, "color", static_cast<int>(m_Color.rgb() & 0xFFFFFF), Fd);
    QString FontName = m_Font.family();
    ScQt_IniFileDataBaseWriteString(SectionPath, "font_name", FontName, Fd);
    ScQt_IniFileDataBaseWriteInt(SectionPath, "font_size", m_Font.pointSize(), Fd);
    return true;
}

bool RotarySwitchWidget::readFromIni()
{
    QString SectionPath = GetIniSectionPath();
    int Fd = ScQt_GetMainFileDescriptor();

    QString VariableName = ScQt_IniFileDataBaseReadString(SectionPath, "variable", "", Fd);
    m_Min = ScQt_IniFileDataBaseReadFloat(SectionPath, "min", 0.0, Fd);
    m_Max = ScQt_IniFileDataBaseReadFloat(SectionPath, "max", 100.0, Fd);
    if (m_Max <= m_Min) {
        m_Max = m_Min + 1.0;
    }
    SetDetentCount(ScQt_IniFileDataBaseReadInt(SectionPath, "detents", 5, Fd));
    SetSnapThreshold(ScQt_IniFileDataBaseReadFloat(SectionPath, "snap_threshold", 3.0, Fd));
    m_SnapOnly = (ScQt_IniFileDataBaseReadYesNo(SectionPath, "snap_only", 1, Fd) != 0);
    QString Labels = ScQt_IniFileDataBaseReadString(SectionPath, "labels", "", Fd);
    m_DetentLabels = Labels.isEmpty() ? QStringList() : Labels.split(QLatin1Char('|'));
    m_Color = QColor(QRgb(0xFF000000u | static_cast<unsigned int>(ScQt_IniFileDataBaseReadInt(SectionPath, "color", 0xC8C8CD, Fd))));

    QString FontName = ScQt_IniFileDataBaseReadString(SectionPath, "font_name", "", Fd);
    int FontSize = ScQt_IniFileDataBaseReadInt(SectionPath, "font_size", -1, Fd);
    if (!FontName.isEmpty()) {
        m_Font.setFamily(FontName);
    }
    if (FontSize > 0) {
        m_Font.setPointSize(FontSize);
    }
    setFont(m_Font);

    m_Value = m_Min;
    if (!VariableName.isEmpty()) {
        SetVariable(VariableName);
    }
    update();
    return true;
}

CustomDialogFrame* RotarySwitchWidget::dialogSettings(QWidget *arg_parent)
{
    arg_parent->setWindowTitle(tr("Rotary switch config"));
    return new RotarySwitchDialogFrame(this, arg_parent);
}

QString RotarySwitchWidget::GetVariableName() const
{
    return m_VariableName;
}

double RotarySwitchWidget::GetMin() const
{
    return m_Min;
}

double RotarySwitchWidget::GetMax() const
{
    return m_Max;
}

void RotarySwitchWidget::SetRange(double arg_Min, double arg_Max)
{
    if (arg_Max <= arg_Min) {
        arg_Max = arg_Min + 1.0;
    }
    m_Min = arg_Min;
    m_Max = arg_Max;
    if (m_Value < m_Min) m_Value = m_Min;
    if (m_Value > m_Max) m_Value = m_Max;
    update();
}

int RotarySwitchWidget::GetDetentCount() const
{
    return m_DetentCount;
}

void RotarySwitchWidget::SetDetentCount(int arg_Count)
{
    if (arg_Count < ROTARY_SWITCH_MIN_DETENTS) arg_Count = ROTARY_SWITCH_MIN_DETENTS;
    if (arg_Count > ROTARY_SWITCH_MAX_DETENTS) arg_Count = ROTARY_SWITCH_MAX_DETENTS;
    m_DetentCount = arg_Count;
    update();
}

double RotarySwitchWidget::GetSnapThreshold() const
{
    return m_SnapThreshold;
}

void RotarySwitchWidget::SetSnapThreshold(double arg_Percent)
{
    if (arg_Percent < 0.0) arg_Percent = 0.0;
    if (arg_Percent > 50.0) arg_Percent = 50.0;
    m_SnapThreshold = arg_Percent;
    update();
}

bool RotarySwitchWidget::GetSnapOnly() const
{
    return m_SnapOnly;
}

void RotarySwitchWidget::SetSnapOnly(bool arg_SnapOnly)
{
    m_SnapOnly = arg_SnapOnly;
    update();
}

QStringList RotarySwitchWidget::GetDetentLabels() const
{
    return m_DetentLabels;
}

void RotarySwitchWidget::SetDetentLabels(const QStringList &arg_Labels)
{
    m_DetentLabels = arg_Labels;
    // drop trailing empty entries, they only bloat the ini file
    while (!m_DetentLabels.isEmpty() && m_DetentLabels.last().isEmpty()) {
        m_DetentLabels.removeLast();
    }
    update();
}

QString RotarySwitchWidget::DetentLabel(int arg_Index) const
{
    if ((arg_Index < 0) || (arg_Index >= m_DetentLabels.size())) {
        return QString();
    }
    return m_DetentLabels.at(arg_Index);
}

bool RotarySwitchWidget::HasAnyDetentLabel() const
{
    for (int x = 0; (x < m_DetentCount) && (x < m_DetentLabels.size()); x++) {
        if (!m_DetentLabels.at(x).isEmpty()) {
            return true;
        }
    }
    return false;
}

double RotarySwitchWidget::DetentValue(int arg_Index) const
{
    if (arg_Index <= 0) {
        return m_Min;
    }
    if (arg_Index >= (m_DetentCount - 1)) {
        return m_Max;
    }
    return m_Min + ((m_Max - m_Min) * static_cast<double>(arg_Index)) / static_cast<double>(m_DetentCount - 1);
}

double RotarySwitchWidget::SnapThresholdValue() const
{
    return (m_Max - m_Min) * m_SnapThreshold / 100.0;
}

double RotarySwitchWidget::ValueToAngle(double arg_Value) const
{
    double Ratio = (arg_Value - m_Min) / (m_Max - m_Min);
    if (Ratio < 0.0) Ratio = 0.0;
    if (Ratio > 1.0) Ratio = 1.0;
    return ROTARY_SWITCH_START_ANGLE + Ratio * (ROTARY_SWITCH_END_ANGLE - ROTARY_SWITCH_START_ANGLE);
}

double RotarySwitchWidget::AngleToValue(double arg_Angle) const
{
    if (arg_Angle < ROTARY_SWITCH_START_ANGLE) arg_Angle = ROTARY_SWITCH_START_ANGLE;
    if (arg_Angle > ROTARY_SWITCH_END_ANGLE) arg_Angle = ROTARY_SWITCH_END_ANGLE;
    double Ratio = (arg_Angle - ROTARY_SWITCH_START_ANGLE) / (ROTARY_SWITCH_END_ANGLE - ROTARY_SWITCH_START_ANGLE);
    return m_Min + Ratio * (m_Max - m_Min);
}

double RotarySwitchWidget::AngleOfPoint(const QPoint &arg_Pos) const
{
    QRectF Knob = KnobRect();
    double Dx = static_cast<double>(arg_Pos.x()) - Knob.center().x();
    double Dy = static_cast<double>(arg_Pos.y()) - Knob.center().y();
    // 0 degree points up, positive is clockwise, result is -180 .. +180
    double Angle = std::atan2(Dx, -Dy) * 180.0 / M_PI;
    // The dead zone between the two stops is symmetric around 180 degree, which
    // is exactly where atan2 wraps around - so clamping picks the nearer stop.
    if (Angle < ROTARY_SWITCH_START_ANGLE) {
        Angle = ROTARY_SWITCH_START_ANGLE;
    } else if (Angle > ROTARY_SWITCH_END_ANGLE) {
        Angle = ROTARY_SWITCH_END_ANGLE;
    }
    return Angle;
}

int RotarySwitchWidget::NearestDetentIndex(double arg_Value) const
{
    double Step = (m_Max - m_Min) / static_cast<double>(m_DetentCount - 1);
    int Index = static_cast<int>(std::floor(((arg_Value - m_Min) / Step) + 0.5));
    if (Index < 0) Index = 0;
    if (Index > (m_DetentCount - 1)) Index = m_DetentCount - 1;
    return Index;
}

int RotarySwitchWidget::DetentIndexOf(double arg_Value) const
{
    int Index = NearestDetentIndex(arg_Value);
    if (m_SnapOnly) {
        return Index;
    }
    if (std::fabs(arg_Value - DetentValue(Index)) <= SnapThresholdValue()) {
        return Index;
    }
    return -1;
}

double RotarySwitchWidget::ApplySnap(double arg_Value) const
{
    int Index = DetentIndexOf(arg_Value);
    if (Index >= 0) {
        return DetentValue(Index);
    }
    return arg_Value;
}

void RotarySwitchWidget::SetValue(double arg_Value, bool arg_WriteToBlackboard)
{
    if (arg_Value < m_Min) arg_Value = m_Min;
    if (arg_Value > m_Max) arg_Value = m_Max;
    if (arg_Value == m_Value) {
        return;
    }
    m_Value = arg_Value;
    if (arg_WriteToBlackboard && (m_Vid > 0)) {
        write_bbvari_minmax_check(m_Vid, m_Value);
    }
    update();
}

void RotarySwitchWidget::StepDetent(int arg_Direction)
{
    int Index = NearestDetentIndex(m_Value);
    // If the knob sits between two detents a step should first move onto the
    // detent in the wanted direction, not jump over it.
    double Current = DetentValue(Index);
    if (((arg_Direction > 0) && (Current <= m_Value)) ||
        ((arg_Direction < 0) && (Current >= m_Value))) {
        Index += arg_Direction;
    }
    if (Index < 0) Index = 0;
    if (Index > (m_DetentCount - 1)) Index = m_DetentCount - 1;
    SetValue(DetentValue(Index), true);
}

QRectF RotarySwitchWidget::KnobRect() const
{
    int TextHeight = fontMetrics().height();
    QRect Area = rect().adjusted(2, TextHeight + 2, -2, -(TextHeight + 2));
    if ((Area.width() < 24) || (Area.height() < 24)) {
        Area = rect().adjusted(2, 2, -2, -2);
    }
    double Side = qMin(Area.width(), Area.height());
    QRectF Result(0.0, 0.0, Side, Side);
    Result.moveCenter(Area.center());
    return Result;
}

QString RotarySwitchWidget::ValueText() const
{
    double Step = (m_Max - m_Min) / static_cast<double>(m_DetentCount - 1);
    int Decimals = 0;
    if (Step < 0.1) Decimals = 3;
    else if (Step < 1.0) Decimals = 2;
    else if (Step < 10.0) Decimals = 1;
    return QString::number(m_Value, 'f', Decimals);
}

void RotarySwitchWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter Painter(this);
    Painter.setRenderHint(QPainter::Antialiasing, true);
    Painter.setFont(m_Font);

    QRectF Knob = KnobRect();
    QPointF Center = Knob.center();
    double Radius = Knob.width() / 2.0;
    if (Radius < 8.0) {
        return;
    }

    // the labels of the detent positions need room outside of the scale
    bool DrawLabels = HasAnyDetentLabel();
    if (DrawLabels) {
        Radius *= 0.72;
    }

    double ScaleRadius = Radius * 0.94;
    double BandRadius = ScaleRadius * 0.92;
    double TickOuter = ScaleRadius * 0.84;
    double TickInner = ScaleRadius * 0.72;
    double BodyRadius = Radius * 0.66;

    QColor AccentColor = m_Color.darker(220);
    QColor BandColor(70, 140, 220);

    QRectF BandRect(Center.x() - BandRadius, Center.y() - BandRadius,
                    BandRadius * 2.0, BandRadius * 2.0);

    // track between the two stops
    QPen TrackPen(QColor(AccentColor.red(), AccentColor.green(), AccentColor.blue(), 110));
    TrackPen.setWidthF(qMax(1.0, Radius * 0.03));
    TrackPen.setCapStyle(Qt::FlatCap);
    Painter.setPen(TrackPen);
    Painter.setBrush(Qt::NoBrush);
    Painter.drawArc(BandRect,
                    static_cast<int>((90.0 - ROTARY_SWITCH_END_ANGLE) * 16.0),
                    static_cast<int>((ROTARY_SWITCH_END_ANGLE - ROTARY_SWITCH_START_ANGLE) * 16.0));

    if (m_SnapOnly) {
        // only the detent positions can be set, so mark them as fixed points
        double DotRadius = qMax(2.0, Radius * 0.055);
        int EngagedIndex = DetentIndexOf(m_Value);
        Painter.setPen(Qt::NoPen);
        for (int x = 0; x < m_DetentCount; x++) {
            double Rad = ValueToAngle(DetentValue(x)) * M_PI / 180.0;
            Painter.setBrush(QBrush((x == EngagedIndex)
                                        ? BandColor
                                        : QColor(BandColor.red(), BandColor.green(), BandColor.blue(), 120)));
            Painter.drawEllipse(QPointF(Center.x() + std::sin(Rad) * BandRadius,
                                        Center.y() - std::cos(Rad) * BandRadius),
                                DotRadius, DotRadius);
        }
    } else if (m_SnapThreshold > 0.0) {
        // capture band of every detent - the small range in which the knob snaps in
        QPen BandPen;
        BandPen.setWidthF(qMax(3.0, Radius * 0.10));
        BandPen.setCapStyle(Qt::FlatCap);
        Painter.setBrush(Qt::NoBrush);
        double Threshold = SnapThresholdValue();
        int EngagedIndex = DetentIndexOf(m_Value);
        for (int x = 0; x < m_DetentCount; x++) {
            double DetentVal = DetentValue(x);
            double AngleFrom = ValueToAngle(DetentVal - Threshold);
            double AngleTo = ValueToAngle(DetentVal + Threshold);
            // Qt counts counter clockwise starting at 3 o'clock, in 1/16 degree
            int StartAngle = static_cast<int>((90.0 - AngleTo) * 16.0);
            int SpanAngle = static_cast<int>((AngleTo - AngleFrom) * 16.0);
            if (SpanAngle > 0) {
                BandPen.setColor(QColor(BandColor.red(), BandColor.green(), BandColor.blue(),
                                        (x == EngagedIndex) ? 255 : 120));
                Painter.setPen(BandPen);
                Painter.drawArc(BandRect, StartAngle, SpanAngle);
            }
        }
    }

    // detent marks
    for (int x = 0; x < m_DetentCount; x++) {
        double Angle = ValueToAngle(DetentValue(x));
        bool Engaged = (DetentIndexOf(m_Value) == x);
        QPen TickPen(Engaged ? BandColor : AccentColor);
        TickPen.setWidthF(Engaged ? qMax(2.5, Radius * 0.06) : qMax(1.0, Radius * 0.035));
        TickPen.setCapStyle(Qt::RoundCap);
        Painter.setPen(TickPen);
        double Rad = Angle * M_PI / 180.0;
        double Sin = std::sin(Rad);
        double Cos = std::cos(Rad);
        Painter.drawLine(QPointF(Center.x() + Sin * TickInner, Center.y() - Cos * TickInner),
                         QPointF(Center.x() + Sin * TickOuter, Center.y() - Cos * TickOuter));
    }

    // labels of the detent positions
    if (DrawLabels) {
        QFont LabelFont = m_Font;
        QFontMetrics LabelMetrics(LabelFont);
        Painter.setFont(LabelFont);
        double LabelRadius = ScaleRadius * 1.24;
        int EngagedIndex = DetentIndexOf(m_Value);
        for (int x = 0; x < m_DetentCount; x++) {
            QString Label = DetentLabel(x);
            if (Label.isEmpty()) {
                continue;
            }
            double Rad = ValueToAngle(DetentValue(x)) * M_PI / 180.0;
            QRectF TextRect(0.0, 0.0,
                            LabelMetrics.boundingRect(Label).width() + 4, LabelMetrics.height());
            TextRect.moveCenter(QPointF(Center.x() + std::sin(Rad) * LabelRadius,
                                        Center.y() - std::cos(Rad) * LabelRadius));
            // keep the text inside the window even if it is longer than the free space
            if (TextRect.left() < 0.0) TextRect.moveLeft(0.0);
            if (TextRect.right() > width()) TextRect.moveRight(width());
            if (TextRect.top() < 0.0) TextRect.moveTop(0.0);
            if (TextRect.bottom() > height()) TextRect.moveBottom(height());
            Painter.setPen((x == EngagedIndex) ? BandColor : palette().color(QPalette::WindowText));
            Painter.drawText(TextRect, Qt::AlignCenter, Label);
        }
        Painter.setFont(m_Font);
    }

    // knob body
    QRectF BodyRect(Center.x() - BodyRadius, Center.y() - BodyRadius, BodyRadius * 2.0, BodyRadius * 2.0);
    QRadialGradient Gradient(Center.x() - BodyRadius * 0.35, Center.y() - BodyRadius * 0.35, BodyRadius * 1.8);
    Gradient.setColorAt(0.0, m_Color.lighter(125));
    Gradient.setColorAt(1.0, m_Color.darker(140));
    Painter.setBrush(QBrush(Gradient));
    QPen BodyPen(AccentColor);
    BodyPen.setWidthF(qMax(1.0, Radius * 0.03));
    Painter.setPen(BodyPen);
    Painter.drawEllipse(BodyRect);

    // pointer
    double PointerAngle = ValueToAngle(m_Value);
    Painter.save();
    Painter.translate(Center);
    Painter.rotate(PointerAngle);
    QPen PointerPen((DetentIndexOf(m_Value) >= 0) ? BandColor : AccentColor);
    PointerPen.setWidthF(qMax(2.0, Radius * 0.07));
    PointerPen.setCapStyle(Qt::RoundCap);
    Painter.setPen(PointerPen);
    Painter.drawLine(QPointF(0.0, -BodyRadius * 0.15), QPointF(0.0, -BodyRadius * 0.92));
    Painter.restore();

    // variable name above, value below the knob
    int TextHeight = fontMetrics().height();
    if (rect().height() > (Knob.height() + 2 * TextHeight)) {
        Painter.setPen(palette().color(QPalette::WindowText));
        QString Name = m_VariableName.isEmpty() ? tr("(no variable)") : m_VariableName;
        Painter.drawText(QRect(2, 1, width() - 4, TextHeight),
                         Qt::AlignHCenter | Qt::AlignVCenter,
                         fontMetrics().elidedText(Name, Qt::ElideMiddle, width() - 4));
        QString Text = ValueText();
        int Index = DetentIndexOf(m_Value);
        if (Index >= 0) {
            QString Label = DetentLabel(Index);
            Text += QString(" [%1]").arg(Label.isEmpty() ? QString::number(Index) : Label);
        }
        Painter.drawText(QRect(2, height() - TextHeight - 1, width() - 4, TextHeight),
                         Qt::AlignHCenter | Qt::AlignVCenter, Text);
    }
}

void RotarySwitchWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        MdiWindowWidget::mousePressEvent(event);
        return;
    }
    setFocus();
    m_Dragging = true;
    SetValue(ApplySnap(AngleToValue(AngleOfPoint(QPoint(GetEventXPos(event), GetEventYPos(event))))), true);
    event->accept();
}

void RotarySwitchWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_Dragging) {
        MdiWindowWidget::mouseMoveEvent(event);
        return;
    }
    SetValue(ApplySnap(AngleToValue(AngleOfPoint(QPoint(GetEventXPos(event), GetEventYPos(event))))), true);
    event->accept();
}

void RotarySwitchWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_Dragging && (event->button() == Qt::LeftButton)) {
        m_Dragging = false;
        event->accept();
        return;
    }
    MdiWindowWidget::mouseReleaseEvent(event);
}

void RotarySwitchWidget::wheelEvent(QWheelEvent *event)
{
    int Delta = event->angleDelta().y();
    if (Delta != 0) {
        StepDetent((Delta > 0) ? 1 : -1);
        event->accept();
        return;
    }
    MdiWindowWidget::wheelEvent(event);
}

void RotarySwitchWidget::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Right:
    case Qt::Key_Up:
        StepDetent(1);
        event->accept();
        break;
    case Qt::Key_Left:
    case Qt::Key_Down:
        StepDetent(-1);
        event->accept();
        break;
    default:
        MdiWindowWidget::keyPressEvent(event);
        break;
    }
}

void RotarySwitchWidget::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasText()) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void RotarySwitchWidget::dragMoveEvent(QDragMoveEvent *event)
{
    Q_UNUSED(event)
}

void RotarySwitchWidget::dragLeaveEvent(QDragLeaveEvent *event)
{
    Q_UNUSED(event)
}

void RotarySwitchWidget::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasText()) {
        DragAndDropInfos Infos(event->mimeData()->text());
        event->acceptProposedAction();
        SetVariable(Infos.GetName());
    } else {
        event->ignore();
    }
}

void RotarySwitchWidget::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);
    QAction *ConfigAct = menu.addAction(tr("&config"));
    connect(ConfigAct, SIGNAL(triggered()), this, SLOT(ConfigureSlot()));
    menu.exec(event->globalPos());
}

void RotarySwitchWidget::ConfigureSlot()
{
    openDialog();
}

void RotarySwitchWidget::CyclicUpdate()
{
    if (m_Vid <= 0) {
        // The variable may have vanished from the blackboard in the meantime
        // (process was stopped), attach again as soon as it is back.
        AttachToBlackboard();
    }
    if ((m_Vid <= 0) || m_Dragging) {
        return;
    }
    double Value = read_bbvari_convert_double(m_Vid);
    if (Value != m_Value) {
        m_Value = Value;
        update();
    }
}

void RotarySwitchWidget::blackboardVariableConfigChanged(int arg_vid, unsigned int arg_observationFlag)
{
    if ((arg_vid != m_Vid) || (m_Vid <= 0)) {
        return;
    }
    if ((arg_observationFlag & OBSERVE_REMOVE_VARIABLE) != 0) {
        DetachFromBlackboard();  // keep the configured name, CyclicUpdate() attaches again
    }
}

void RotarySwitchWidget::changeColor(QColor arg_color)
{
    m_Color = arg_color;
    update();
}

void RotarySwitchWidget::changeFont(QFont arg_font)
{
    m_Font = arg_font;
    setFont(arg_font);
    update();
}

void RotarySwitchWidget::changeWindowName(QString arg_name)
{
    RenameWindowTo(arg_name);
}

void RotarySwitchWidget::changeVariable(QString arg_variable, bool arg_visible)
{
    if (arg_visible) {
        SetVariable(arg_variable);
    } else if (arg_variable.compare(m_VariableName) == 0) {
        ClearVariable();
    }
}

void RotarySwitchWidget::changeVaraibles(QStringList arg_variables, bool arg_visible)
{
    Q_UNUSED(arg_visible)
    if (!arg_variables.isEmpty()) {
        SetVariable(arg_variables.first());
    }
}

void RotarySwitchWidget::resetDefaultVariables(QStringList arg_variables)
{
    if (!arg_variables.isEmpty()) {
        SetVariable(arg_variables.first());
    } else {
        ClearVariable();
    }
}

void RotarySwitchWidget::openDialog()
{
    QStringList List;
    if (!m_VariableName.isEmpty()) {
        List.append(m_VariableName);
    }
    emit openStandardDialog(List, true, false, m_Color, m_Font);
}

void RotarySwitchWidget::SetVariable(const QString &arg_VariableName)
{
    DetachFromBlackboard();
    // The name is stored even if the variable is currently not inside the
    // blackboard, otherwise the configuration would be lost with the next
    // writeToIni() and the user had to select the variable again.
    m_VariableName = arg_VariableName;
    AttachToBlackboard();
    update();
}

void RotarySwitchWidget::ClearVariable()
{
    DetachFromBlackboard();
    m_VariableName.clear();
    update();
}

void RotarySwitchWidget::AttachToBlackboard()
{
    if ((m_Vid > 0) || m_VariableName.isEmpty()) {
        return;
    }
    int Vid = add_bbvari(QStringToConstChar(m_VariableName), BB_UNKNOWN_WAIT, nullptr);
    if (Vid > 0) {
        m_Vid = Vid;
        m_ObserverConnection.AddObserveVariable(m_Vid, OBSERVE_CONFIG_ANYTHING_CHANGED);
        m_Value = read_bbvari_convert_double(m_Vid);
    }
}

void RotarySwitchWidget::DetachFromBlackboard()
{
    if (m_Vid > 0) {
        m_ObserverConnection.RemoveObserveVariable(m_Vid);
        remove_bbvari_unknown_wait(m_Vid);
        m_Vid = 0;
    }
}
