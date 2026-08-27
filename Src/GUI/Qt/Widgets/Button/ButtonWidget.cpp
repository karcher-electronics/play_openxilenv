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


#include "ButtonWidget.h"
#include "ButtonDialogFrame.h"
#include "MdiWindowType.h"
#include "DragAndDrop.h"
#include "QtIniFile.h"
#include "StringHelpers.h"

#include <QSignalBlocker>
#include <QMimeData>
#include <QMenu>
#include <QAction>
#include <QContextMenuEvent>
#include <QResizeEvent>
#include <QFontMetrics>

extern "C" {
#include "Blackboard.h"
#include "BlackboardAccess.h"
}

// The ini file stores colours as 0x00bbggrr, same as the text window does.
static unsigned int ButtonColorToIni(const QColor &par_Color)
{
    return static_cast<unsigned int>(par_Color.red()) |
           (static_cast<unsigned int>(par_Color.green()) << 8) |
           (static_cast<unsigned int>(par_Color.blue()) << 16);
}

static QColor ButtonIniToColor(int par_Value)
{
    return QColor(par_Value & 0x000000FF, (par_Value & 0x0000FF00) >> 8, (par_Value & 0x00FF0000) >> 16);
}

RoundPushButton::RoundPushButton(QWidget *parent) : QPushButton(parent)
{
}

bool RoundPushButton::hitButton(const QPoint &pos) const
{
    QRectF Rect(rect());
    qreal Radius = qMin(Rect.width(), Rect.height()) / 2.0;
    QPointF Delta = QPointF(pos) - Rect.center();
    return ((Delta.x() * Delta.x()) + (Delta.y() * Delta.y())) <= (Radius * Radius);
}

ButtonWidget::ButtonWidget(QString par_WindowTitle, MdiSubWindow* par_SubWindow, MdiWindowType *par_Type, QWidget *parent) :
    MdiWindowWidget(par_WindowTitle, par_SubWindow, par_Type, parent),
    m_Vid(0),
    m_Mode(Taster),
    m_Color(240, 240, 240),
    m_ColorOn(0, 200, 0),
    m_ColorOff(240, 240, 240),
    m_IsOn(false),
    m_ObserverConnection(this)
{
    m_Button = new RoundPushButton(this);
    // Deliberately no layout: button and label are placed by
    // UpdateButtonGeometry(). A layout would propagate their size as the minimum
    // size of the whole sub window, and the user could no longer shrink it again.
    m_Button->setMinimumSize(1, 1);

    m_Label = new QLabel(this);
    m_Label->setAlignment(Qt::AlignCenter);

    connect(m_Button, SIGNAL(toggled(bool)), this, SLOT(ButtonToggled(bool)));
    connect(m_Button, SIGNAL(pressed()), this, SLOT(ButtonPressed()));
    connect(m_Button, SIGNAL(released()), this, SLOT(ButtonReleased()));

    setAcceptDrops(true);
    setMinimumSize(32, 52);

    readFromIni();
    UpdateButtonGeometry();
}

ButtonWidget::~ButtonWidget()
{
    writeToIni();
    DetachFromBlackboard();
}

bool ButtonWidget::writeToIni()
{
    QString SectionPath = GetIniSectionPath();
    int Fd = ScQt_GetMainFileDescriptor();

    QString WindowType = GetMdiWindowType()->GetWindowTypeName();
    QString ModeString = (m_Mode == Schalter) ? "schalter" : "taster";
    ScQt_IniFileDataBaseWriteString(SectionPath, "type", WindowType, Fd);
    ScQt_IniFileDataBaseWriteString(SectionPath, "variable", m_VariableName, Fd);
    ScQt_IniFileDataBaseWriteString(SectionPath, "mode", ModeString, Fd);
    QString ColorOnString = QString("0x%1").arg(ButtonColorToIni(m_ColorOn), 0, 16);
    QString ColorOffString = QString("0x%1").arg(ButtonColorToIni(m_ColorOff), 0, 16);
    ScQt_IniFileDataBaseWriteString(SectionPath, "ColorOn", ColorOnString, Fd);
    ScQt_IniFileDataBaseWriteString(SectionPath, "ColorOff", ColorOffString, Fd);
    return true;
}

bool ButtonWidget::readFromIni()
{
    QString SectionPath = GetIniSectionPath();
    int Fd = ScQt_GetMainFileDescriptor();

    QString VariableName = ScQt_IniFileDataBaseReadString(SectionPath, "variable", "", Fd);
    QString ModeString = ScQt_IniFileDataBaseReadString(SectionPath, "mode", "taster", Fd);
    m_Mode = (ModeString.compare("schalter", Qt::CaseInsensitive) == 0) ? Schalter : Taster;
    m_Button->setCheckable(m_Mode == Schalter);

    int ColorOn = ScQt_IniFileDataBaseReadInt(SectionPath, "ColorOn", static_cast<int>(ButtonColorToIni(m_ColorOn)), Fd);
    int ColorOff = ScQt_IniFileDataBaseReadInt(SectionPath, "ColorOff", static_cast<int>(ButtonColorToIni(m_ColorOff)), Fd);
    m_ColorOn = ButtonIniToColor(ColorOn);
    m_ColorOff = ButtonIniToColor(ColorOff);
    m_Color = m_ColorOff;

    if (!VariableName.isEmpty()) {
        SetVariable(VariableName);
    } else {
        UpdateLabel();
    }
    ApplyStateColor();
    return true;
}

CustomDialogFrame* ButtonWidget::dialogSettings(QWidget *arg_parent)
{
    arg_parent->setWindowTitle(tr("Button config"));
    return new ButtonDialogFrame(this, arg_parent);
}

ButtonWidget::ButtonMode ButtonWidget::GetMode() const
{
    return m_Mode;
}

void ButtonWidget::SetMode(ButtonWidget::ButtonMode arg_Mode)
{
    if (m_Mode == arg_Mode) {
        return;
    }
    m_Mode = arg_Mode;
    const QSignalBlocker Blocker(m_Button);
    m_Button->setChecked(false);
    m_Button->setDown(false);
    m_Button->setCheckable(m_Mode == Schalter);
    SetOnState(false);
}

QString ButtonWidget::GetVariableName() const
{
    return m_VariableName;
}

QColor ButtonWidget::GetColorOn() const
{
    return m_ColorOn;
}

QColor ButtonWidget::GetColorOff() const
{
    return m_ColorOff;
}

void ButtonWidget::SetColorOn(QColor arg_Color)
{
    if (!arg_Color.isValid() || (m_ColorOn == arg_Color)) {
        return;
    }
    m_ColorOn = arg_Color;
    ApplyStateColor();
}

void ButtonWidget::SetColorOff(QColor arg_Color)
{
    if (!arg_Color.isValid() || (m_ColorOff == arg_Color)) {
        return;
    }
    m_ColorOff = arg_Color;
    // The generic window colour dialog edits the same colour, keep it in sync.
    m_Color = arg_Color;
    ApplyStateColor();
}

void ButtonWidget::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasText()) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void ButtonWidget::dragMoveEvent(QDragMoveEvent *event)
{
    Q_UNUSED(event)
}

void ButtonWidget::dragLeaveEvent(QDragLeaveEvent *event)
{
    Q_UNUSED(event)
}

void ButtonWidget::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasText()) {
        DragAndDropInfos Infos(event->mimeData()->text());
        event->acceptProposedAction();
        SetVariable(Infos.GetName());
    } else {
        event->ignore();
    }
}

void ButtonWidget::resizeEvent(QResizeEvent *event)
{
    MdiWindowWidget::resizeEvent(event);
    UpdateButtonGeometry();
}

void ButtonWidget::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);
    QAction *ConfigAct = menu.addAction(tr("&config"));
    connect(ConfigAct, SIGNAL(triggered()), this, SLOT(ConfigureSlot()));
    menu.exec(event->globalPos());
}

void ButtonWidget::ConfigureSlot()
{
    openDialog();
}

void ButtonWidget::CyclicUpdate()
{
    if (m_Vid <= 0) {
        // The variable may have vanished from the blackboard in the meantime
        // (process was stopped), attach again as soon as it is back.
        AttachToBlackboard();
        if (m_Vid <= 0) {
            return;
        }
    }
    bool On = (read_bbvari_convert_double(m_Vid) != 0.0);
    if (m_Mode == Schalter) {
        if (m_Button->isChecked() != On) {
            const QSignalBlocker Blocker(m_Button);
            m_Button->setChecked(On);
        }
    } else {
        if (m_Button->isDown() != On) {
            const QSignalBlocker Blocker(m_Button);
            m_Button->setDown(On);
        }
    }
    SetOnState(On);
}

void ButtonWidget::blackboardVariableConfigChanged(int arg_vid, unsigned int arg_observationFlag)
{
    if ((arg_vid != m_Vid) || (m_Vid <= 0)) {
        return;
    }
    if ((arg_observationFlag & OBSERVE_REMOVE_VARIABLE) != 0) {
        DetachFromBlackboard();  // keep the configured name, CyclicUpdate() attaches again
    }
}

void ButtonWidget::ButtonToggled(bool arg_checked)
{
    if (m_Mode != Schalter) {
        return;
    }
    // Colour follows the click even without a variable, so the state stays visible.
    SetOnState(arg_checked);
    if (m_Vid <= 0) {
        return;
    }
    write_bbvari_minmax_check(m_Vid, arg_checked ? 1.0 : 0.0);
}

void ButtonWidget::ButtonPressed()
{
    if (m_Mode != Taster) {
        return;
    }
    SetOnState(true);
    if (m_Vid <= 0) {
        return;
    }
    write_bbvari_minmax_check(m_Vid, 1.0);
}

void ButtonWidget::ButtonReleased()
{
    if (m_Mode != Taster) {
        return;
    }
    SetOnState(false);
    if (m_Vid <= 0) {
        return;
    }
    write_bbvari_minmax_check(m_Vid, 0.0);
}

void ButtonWidget::changeColor(QColor arg_color)
{
    // The generic window colour dialog sets the off colour of the button.
    SetColorOff(arg_color);
}

void ButtonWidget::changeFont(QFont arg_font)
{
    // The name is drawn by the label now, so the font has to reach it as well.
    m_Button->setFont(arg_font);
    m_Label->setFont(arg_font);
    UpdateButtonGeometry();
}

void ButtonWidget::changeWindowName(QString arg_name)
{
    RenameWindowTo(arg_name);
}

void ButtonWidget::changeVariable(QString arg_variable, bool arg_visible)
{
    if (arg_visible) {
        SetVariable(arg_variable);
    } else if (arg_variable.compare(m_VariableName) == 0) {
        ClearVariable();
    }
}

void ButtonWidget::changeVaraibles(QStringList arg_variables, bool arg_visible)
{
    Q_UNUSED(arg_visible)
    if (!arg_variables.isEmpty()) {
        SetVariable(arg_variables.first());
    }
}

void ButtonWidget::resetDefaultVariables(QStringList arg_variables)
{
    if (!arg_variables.isEmpty()) {
        SetVariable(arg_variables.first());
    } else {
        ClearVariable();
    }
}

void ButtonWidget::openDialog()
{
    QStringList List;
    if (!m_VariableName.isEmpty()) {
        List.append(m_VariableName);
    }
    emit openStandardDialog(List, true, false, m_Color);
}

void ButtonWidget::SetVariable(const QString &arg_VariableName)
{
    DetachFromBlackboard();
    // The name is stored even if the variable is currently not inside the
    // blackboard, otherwise the configuration would be lost with the next
    // writeToIni() and the user had to select the variable again.
    m_VariableName = arg_VariableName;
    AttachToBlackboard();
    UpdateLabel();
}

void ButtonWidget::ClearVariable()
{
    DetachFromBlackboard();
    m_VariableName.clear();
    UpdateLabel();
}

void ButtonWidget::AttachToBlackboard()
{
    if ((m_Vid > 0) || m_VariableName.isEmpty()) {
        return;
    }
    int Vid = add_bbvari(QStringToConstChar(m_VariableName), BB_UNKNOWN_WAIT, nullptr);
    if (Vid > 0) {
        m_Vid = Vid;
        m_ObserverConnection.AddObserveVariable(m_Vid, OBSERVE_CONFIG_ANYTHING_CHANGED);
    }
}

void ButtonWidget::DetachFromBlackboard()
{
    if (m_Vid > 0) {
        m_ObserverConnection.RemoveObserveVariable(m_Vid);
        remove_bbvari_unknown_wait(m_Vid);
        m_Vid = 0;
    }
}

void ButtonWidget::UpdateLabel()
{
    // The name sits in the label below, the circle itself stays a plain colour.
    m_Button->setText(QString());
    m_Button->setToolTip(m_VariableName);
    m_Label->setToolTip(m_VariableName);
    // The elided text depends on the current width, so let the geometry set it.
    UpdateButtonGeometry();
}

void ButtonWidget::SetOnState(bool arg_On)
{
    if (m_IsOn == arg_On) {
        return;
    }
    m_IsOn = arg_On;
    ApplyStateColor();
}

void ButtonWidget::ApplyStateColor()
{
    QColor Color = m_IsOn ? m_ColorOn : m_ColorOff;
    if (!Color.isValid()) {
        return;
    }
    // Most styles ignore QPalette::Button for a push button, so the background
    // has to be set through the style sheet to become visible at all.
    QColor TextColor = (Color.lightness() < 128) ? QColor(Qt::white) : QColor(Qt::black);
    // Half of the (square) side turns the rounded rectangle into a full circle.
    int Radius = qMin(m_Button->width(), m_Button->height()) / 2;
    m_Button->setStyleSheet(QString("QPushButton { background-color: %1; color: %2; border: 1px solid %3; border-radius: %4px; padding: 2px; }")
                            .arg(Color.name(), TextColor.name(), Color.darker(150).name(), QString::number(Radius)));
}

void ButtonWidget::UpdateButtonGeometry()
{
    const int Margin = 2;
    const int Spacing = 2;

    QString Text = m_VariableName.isEmpty() ? tr("(no variable)") : m_VariableName;
    QFontMetrics Metrics(m_Label->font());
    int LabelHeight = Metrics.height();
    int LabelWidth = qMax(1, width() - (2 * Margin));
    m_Label->setText(Metrics.elidedText(Text, Qt::ElideRight, LabelWidth));
    m_Label->setGeometry(Margin, height() - Margin - LabelHeight, LabelWidth, LabelHeight);

    // Whatever is left above the label carries the circle.
    int Available = height() - (2 * Margin) - LabelHeight - Spacing;
    int Side = qMin(width() - (2 * Margin), Available);
    if (Side < 1) {
        Side = 1;
    }
    m_Button->setGeometry((width() - Side) / 2, Margin + ((Available - Side) / 2), Side, Side);
    ApplyStateColor();
}
