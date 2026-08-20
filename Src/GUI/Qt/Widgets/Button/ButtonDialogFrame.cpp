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


#include "ButtonDialogFrame.h"

#include <QVBoxLayout>
#include <QLabel>

ButtonDialogFrame::ButtonDialogFrame(ButtonWidget *arg_Button, QWidget *arg_parent) :
    CustomDialogFrame(arg_parent),
    m_Button(arg_Button)
{
    QVBoxLayout *Layout = new QVBoxLayout(this);
    Layout->addWidget(new QLabel(tr("Button behaviour:"), this));

    m_SchalterRadio = new QRadioButton(tr("Schalter (state latches on click)"), this);
    m_TasterRadio = new QRadioButton(tr("Taster (active only while pressed)"), this);

    m_Group = new QButtonGroup(this);
    m_Group->addButton(m_SchalterRadio);
    m_Group->addButton(m_TasterRadio);

    Layout->addWidget(m_SchalterRadio);
    Layout->addWidget(m_TasterRadio);
    Layout->addStretch(1);
    setLayout(Layout);

    if (m_Button->GetMode() == ButtonWidget::Schalter) {
        m_SchalterRadio->setChecked(true);
    } else {
        m_TasterRadio->setChecked(true);
    }
}

ButtonDialogFrame::~ButtonDialogFrame()
{
}

void ButtonDialogFrame::userAccept()
{
    m_Button->SetMode(m_SchalterRadio->isChecked() ? ButtonWidget::Schalter : ButtonWidget::Taster);
}

void ButtonDialogFrame::userReject()
{
    // nothing was applied live, so there is nothing to roll back
}
