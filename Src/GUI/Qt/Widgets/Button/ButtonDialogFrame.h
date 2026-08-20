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


#ifndef BUTTONDIALOGFRAME_H
#define BUTTONDIALOGFRAME_H

#include "DialogFrame.h"
#include "ButtonWidget.h"

#include <QRadioButton>
#include <QButtonGroup>
#include <QPushButton>
#include <QColor>

class ButtonDialogFrame : public CustomDialogFrame
{
    Q_OBJECT
public:
    explicit ButtonDialogFrame(ButtonWidget *arg_Button, QWidget *arg_parent = nullptr);
    ~ButtonDialogFrame();

    virtual void userAccept() Q_DECL_OVERRIDE;
    virtual void userReject() Q_DECL_OVERRIDE;

private slots:
    void SelectColorOn();
    void SelectColorOff();

private:
    void UpdateColorButton(QPushButton *par_Button, const QColor &par_Color);
    bool OpenColorDialog(QColor *ptr_Color);

    ButtonWidget *m_Button;
    QRadioButton *m_SchalterRadio;
    QRadioButton *m_TasterRadio;
    QButtonGroup *m_Group;
    QPushButton *m_ColorOnButton;
    QPushButton *m_ColorOffButton;

    // Edited locally and only handed over to the widget on accept.
    QColor m_ColorOn;
    QColor m_ColorOff;
};

#endif // BUTTONDIALOGFRAME_H
