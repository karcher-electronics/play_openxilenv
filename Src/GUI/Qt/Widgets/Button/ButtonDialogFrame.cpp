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
#include <QGridLayout>
#include <QLabel>
#include <QColorDialog>
#include <QPixmap>
#include <QIcon>

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

    Layout->addSpacing(8);
    Layout->addWidget(new QLabel(tr("Button colours:"), this));

    m_ColorOn = m_Button->GetColorOn();
    m_ColorOff = m_Button->GetColorOff();

    m_ColorOnButton = new QPushButton(tr("On (value not 0)"), this);
    m_ColorOffButton = new QPushButton(tr("Off (value 0)"), this);
    m_ColorOnButton->setIconSize(QSize(24, 16));
    m_ColorOffButton->setIconSize(QSize(24, 16));
    UpdateColorButton(m_ColorOnButton, m_ColorOn);
    UpdateColorButton(m_ColorOffButton, m_ColorOff);

    QGridLayout *ColorLayout = new QGridLayout();
    ColorLayout->addWidget(new QLabel(tr("On colour:"), this), 0, 0);
    ColorLayout->addWidget(m_ColorOnButton, 0, 1);
    ColorLayout->addWidget(new QLabel(tr("Off colour:"), this), 1, 0);
    ColorLayout->addWidget(m_ColorOffButton, 1, 1);
    ColorLayout->setColumnStretch(1, 1);
    Layout->addLayout(ColorLayout);

    Layout->addStretch(1);
    setLayout(Layout);

    connect(m_ColorOnButton, SIGNAL(clicked()), this, SLOT(SelectColorOn()));
    connect(m_ColorOffButton, SIGNAL(clicked()), this, SLOT(SelectColorOff()));

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
    m_Button->SetColorOn(m_ColorOn);
    m_Button->SetColorOff(m_ColorOff);
}

void ButtonDialogFrame::userReject()
{
    // nothing was applied live, so there is nothing to roll back
}

void ButtonDialogFrame::SelectColorOn()
{
    if (OpenColorDialog(&m_ColorOn)) {
        UpdateColorButton(m_ColorOnButton, m_ColorOn);
    }
}

void ButtonDialogFrame::SelectColorOff()
{
    if (OpenColorDialog(&m_ColorOff)) {
        UpdateColorButton(m_ColorOffButton, m_ColorOff);
    }
}

void ButtonDialogFrame::UpdateColorButton(QPushButton *par_Button, const QColor &par_Color)
{
    QPixmap Pixmap(par_Button->iconSize());
    Pixmap.fill(par_Color);
    par_Button->setIcon(QIcon(Pixmap));
}

bool ButtonDialogFrame::OpenColorDialog(QColor *ptr_Color)
{
    QColorDialog ColorDialog(*ptr_Color, this);
    if (ColorDialog.exec() == QDialog::Accepted) {
        *ptr_Color = ColorDialog.selectedColor();
        return true;
    } else {
        return false;
    }
}
