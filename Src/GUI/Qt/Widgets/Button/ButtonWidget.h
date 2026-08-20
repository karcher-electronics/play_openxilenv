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


#ifndef BUTTONWIDGET_H
#define BUTTONWIDGET_H

#include "MdiWindowWidget.h"
#include "BlackboardObserver.h"

#include <QPushButton>
#include <QLabel>
#include <QColor>

// A push button that is not only drawn as a circle but also hit-tested as one,
// so a click in the corner of its bounding square does not trigger it.
class RoundPushButton : public QPushButton
{
public:
    explicit RoundPushButton(QWidget *parent = nullptr);

protected:
    virtual bool hitButton(const QPoint &pos) const Q_DECL_OVERRIDE;
};

class ButtonWidget : public MdiWindowWidget
{
    Q_OBJECT
public:
    // Schalter: click toggles the value and it stays until clicked again.
    // Taster: value is 1 only while the mouse button is held down, 0 otherwise.
    enum ButtonMode { Schalter, Taster };

    explicit ButtonWidget(QString par_WindowTitle, MdiSubWindow* par_SubWindow, MdiWindowType *par_Type, QWidget *parent = nullptr);
    ~ButtonWidget() Q_DECL_OVERRIDE;

    virtual bool writeToIni() Q_DECL_OVERRIDE;
    virtual bool readFromIni() Q_DECL_OVERRIDE;
    virtual CustomDialogFrame* dialogSettings(QWidget *arg_parent) Q_DECL_OVERRIDE;

    ButtonMode GetMode() const;
    void SetMode(ButtonMode arg_Mode);
    QString GetVariableName() const;

    // Background colour the button shows while the variable is not 0 / is 0.
    QColor GetColorOn() const;
    QColor GetColorOff() const;
    void SetColorOn(QColor arg_Color);
    void SetColorOff(QColor arg_Color);

protected:
    virtual void dragEnterEvent(QDragEnterEvent *event) Q_DECL_OVERRIDE;
    virtual void dragMoveEvent(QDragMoveEvent *event) Q_DECL_OVERRIDE;
    virtual void dragLeaveEvent(QDragLeaveEvent *event) Q_DECL_OVERRIDE;
    virtual void dropEvent(QDropEvent *event) Q_DECL_OVERRIDE;
    virtual void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;
    void contextMenuEvent(QContextMenuEvent *event) Q_DECL_OVERRIDE;

private slots:
    virtual void CyclicUpdate() Q_DECL_OVERRIDE;
    void blackboardVariableConfigChanged(int arg_vid, unsigned int arg_observationFlag);
    void ConfigureSlot();
    void ButtonToggled(bool arg_checked);
    void ButtonPressed();
    void ButtonReleased();
    virtual void changeColor(QColor arg_color) Q_DECL_OVERRIDE;
    virtual void changeFont(QFont arg_font) Q_DECL_OVERRIDE;
    virtual void changeWindowName(QString arg_name) Q_DECL_OVERRIDE;
    virtual void changeVariable(QString arg_variable, bool arg_visible) Q_DECL_OVERRIDE;
    virtual void changeVaraibles(QStringList arg_variables, bool arg_visible) Q_DECL_OVERRIDE;
    virtual void resetDefaultVariables(QStringList arg_variables) Q_DECL_OVERRIDE;

private:
    void openDialog() Q_DECL_OVERRIDE;
    void AttachVariable(const QString &arg_VariableName);
    void DetachVariable();
    void UpdateLabel();
    void SetOnState(bool arg_On);
    void ApplyStateColor();
    // Keeps the button square and centred, so the circle stays a circle.
    void UpdateButtonGeometry();

    QPushButton *m_Button;
    QLabel *m_Label;

    QString m_VariableName;
    int m_Vid;
    ButtonMode m_Mode;
    QColor m_Color;
    QColor m_ColorOn;
    QColor m_ColorOff;
    bool m_IsOn;

    BlackboardObserverConnection m_ObserverConnection;
};

#endif // BUTTONWIDGET_H
