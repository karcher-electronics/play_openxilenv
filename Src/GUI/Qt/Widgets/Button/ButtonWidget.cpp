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

extern "C" {
#include "Blackboard.h"
#include "BlackboardAccess.h"
}

ButtonWidget::ButtonWidget(QString par_WindowTitle, MdiSubWindow* par_SubWindow, MdiWindowType *par_Type, QWidget *parent) :
    MdiWindowWidget(par_WindowTitle, par_SubWindow, par_Type, parent),
    m_Vid(0),
    m_Mode(Taster),
    m_Color(240, 240, 240),
    m_ObserverConnection(this)
{
    m_Button = new QPushButton(this);
    m_Layout = new QVBoxLayout(this);
    m_Layout->setContentsMargins(2, 2, 2, 2);
    m_Layout->addWidget(m_Button);
    setLayout(m_Layout);

    connect(m_Button, SIGNAL(toggled(bool)), this, SLOT(ButtonToggled(bool)));
    connect(m_Button, SIGNAL(pressed()), this, SLOT(ButtonPressed()));
    connect(m_Button, SIGNAL(released()), this, SLOT(ButtonReleased()));

    setAcceptDrops(true);
    setMinimumSize(40, 24);

    readFromIni();
}

ButtonWidget::~ButtonWidget()
{
    writeToIni();
    DetachVariable();
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

    if (!VariableName.isEmpty()) {
        AttachVariable(VariableName);
    } else {
        UpdateLabel();
    }
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
}

QString ButtonWidget::GetVariableName() const
{
    return m_VariableName;
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
        AttachVariable(Infos.GetName());
    } else {
        event->ignore();
    }
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
        return;
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
}

void ButtonWidget::blackboardVariableConfigChanged(int arg_vid, unsigned int arg_observationFlag)
{
    if ((arg_vid != m_Vid) || (m_Vid <= 0)) {
        return;
    }
    if ((arg_observationFlag & OBSERVE_REMOVE_VARIABLE) != 0) {
        DetachVariable();
    }
}

void ButtonWidget::ButtonToggled(bool arg_checked)
{
    if ((m_Mode != Schalter) || (m_Vid <= 0)) {
        return;
    }
    write_bbvari_minmax_check(m_Vid, arg_checked ? 1.0 : 0.0);
}

void ButtonWidget::ButtonPressed()
{
    if ((m_Mode != Taster) || (m_Vid <= 0)) {
        return;
    }
    write_bbvari_minmax_check(m_Vid, 1.0);
}

void ButtonWidget::ButtonReleased()
{
    if ((m_Mode != Taster) || (m_Vid <= 0)) {
        return;
    }
    write_bbvari_minmax_check(m_Vid, 0.0);
}

void ButtonWidget::changeColor(QColor arg_color)
{
    m_Color = arg_color;
    QPalette Pal = m_Button->palette();
    Pal.setColor(QPalette::Button, arg_color);
    m_Button->setAutoFillBackground(true);
    m_Button->setPalette(Pal);
}

void ButtonWidget::changeFont(QFont arg_font)
{
    m_Button->setFont(arg_font);
}

void ButtonWidget::changeWindowName(QString arg_name)
{
    RenameWindowTo(arg_name);
}

void ButtonWidget::changeVariable(QString arg_variable, bool arg_visible)
{
    if (arg_visible) {
        AttachVariable(arg_variable);
    } else if (arg_variable.compare(m_VariableName) == 0) {
        DetachVariable();
    }
}

void ButtonWidget::changeVaraibles(QStringList arg_variables, bool arg_visible)
{
    Q_UNUSED(arg_visible)
    if (!arg_variables.isEmpty()) {
        AttachVariable(arg_variables.first());
    }
}

void ButtonWidget::resetDefaultVariables(QStringList arg_variables)
{
    if (!arg_variables.isEmpty()) {
        AttachVariable(arg_variables.first());
    } else {
        DetachVariable();
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

void ButtonWidget::AttachVariable(const QString &arg_VariableName)
{
    DetachVariable();
    if (!arg_VariableName.isEmpty()) {
        int Vid = add_bbvari(QStringToConstChar(arg_VariableName), BB_UNKNOWN_WAIT, nullptr);
        if (Vid > 0) {
            m_Vid = Vid;
            m_VariableName = arg_VariableName;
            m_ObserverConnection.AddObserveVariable(m_Vid, OBSERVE_CONFIG_ANYTHING_CHANGED);
        }
    }
    UpdateLabel();
}

void ButtonWidget::DetachVariable()
{
    if (m_Vid > 0) {
        m_ObserverConnection.RemoveObserveVariable(m_Vid);
        remove_bbvari_unknown_wait(m_Vid);
        m_Vid = 0;
    }
    m_VariableName.clear();
    UpdateLabel();
}

void ButtonWidget::UpdateLabel()
{
    m_Button->setText(m_VariableName.isEmpty() ? tr("(no variable)") : m_VariableName);
}
