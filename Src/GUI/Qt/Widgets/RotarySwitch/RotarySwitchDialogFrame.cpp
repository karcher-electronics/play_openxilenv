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


#include "RotarySwitchDialogFrame.h"

#include <QFormLayout>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QVBoxLayout>

RotarySwitchDialogFrame::RotarySwitchDialogFrame(RotarySwitchWidget *arg_RotarySwitch, QWidget *arg_parent) :
    CustomDialogFrame(arg_parent),
    m_RotarySwitch(arg_RotarySwitch)
{
    QVBoxLayout *Layout = new QVBoxLayout(this);
    QFormLayout *FormLayout = new QFormLayout();

    m_MinSpinBox = new QDoubleSpinBox(this);
    m_MinSpinBox->setRange(-1.0e12, 1.0e12);
    m_MinSpinBox->setDecimals(6);
    m_MinSpinBox->setValue(m_RotarySwitch->GetMin());
    FormLayout->addRow(tr("Value at left stop:"), m_MinSpinBox);

    m_MaxSpinBox = new QDoubleSpinBox(this);
    m_MaxSpinBox->setRange(-1.0e12, 1.0e12);
    m_MaxSpinBox->setDecimals(6);
    m_MaxSpinBox->setValue(m_RotarySwitch->GetMax());
    FormLayout->addRow(tr("Value at right stop:"), m_MaxSpinBox);

    m_DetentCountSpinBox = new QSpinBox(this);
    m_DetentCountSpinBox->setRange(2, 64);
    m_DetentCountSpinBox->setValue(m_RotarySwitch->GetDetentCount());
    FormLayout->addRow(tr("Number of detent positions:"), m_DetentCountSpinBox);

    m_ThresholdSpinBox = new QDoubleSpinBox(this);
    m_ThresholdSpinBox->setRange(0.0, 50.0);
    m_ThresholdSpinBox->setDecimals(2);
    m_ThresholdSpinBox->setSingleStep(0.5);
    m_ThresholdSpinBox->setSuffix(tr(" %"));
    m_ThresholdSpinBox->setValue(m_RotarySwitch->GetSnapThreshold());
    m_ThresholdSpinBox->setToolTip(tr("Half width of the capture band around a detent, "
                                      "in percent of the whole value range. Inside this small "
                                      "range the knob snaps onto the detent."));
    FormLayout->addRow(tr("Detent threshold (+/-):"), m_ThresholdSpinBox);

    m_SnapOnlyCheckBox = new QCheckBox(tr("Only detent positions (no free values in between)"), this);
    m_SnapOnlyCheckBox->setChecked(m_RotarySwitch->GetSnapOnly());
    m_SnapOnlyCheckBox->setToolTip(tr("The knob always jumps to the nearest detent position, "
                                      "so no value in between can be set. The detent threshold "
                                      "is not used in this mode."));
    FormLayout->addRow(QString(), m_SnapOnlyCheckBox);

    Layout->addLayout(FormLayout);

    Layout->addWidget(new QLabel(tr("Text of the detent positions (optional):"), this));
    m_DetentTable = new QTableWidget(0, 2, this);
    QStringList HeaderLabels;
    HeaderLabels << tr("Value") << tr("Text");
    m_DetentTable->setHorizontalHeaderLabels(HeaderLabels);
    m_DetentTable->verticalHeader()->setVisible(false);
    m_DetentTable->horizontalHeader()->setStretchLastSection(true);
    m_DetentTable->setSelectionBehavior(QAbstractItemView::SelectItems);
    m_DetentTable->setToolTip(tr("Leave a line empty to show no text for that position. "
                                 "The text of the engaged position is shown below the knob."));
    Layout->addWidget(m_DetentTable, 1);

    m_PreviewLabel = new QLabel(this);
    m_PreviewLabel->setWordWrap(true);
    Layout->addWidget(m_PreviewLabel);
    setLayout(Layout);

    connect(m_MinSpinBox, SIGNAL(valueChanged(double)), this, SLOT(UpdatePreviewLabel()));
    connect(m_MaxSpinBox, SIGNAL(valueChanged(double)), this, SLOT(UpdatePreviewLabel()));
    connect(m_DetentCountSpinBox, SIGNAL(valueChanged(int)), this, SLOT(UpdatePreviewLabel()));
    connect(m_ThresholdSpinBox, SIGNAL(valueChanged(double)), this, SLOT(UpdatePreviewLabel()));
    connect(m_SnapOnlyCheckBox, SIGNAL(toggled(bool)), this, SLOT(UpdatePreviewLabel()));
    connect(m_MinSpinBox, SIGNAL(valueChanged(double)), this, SLOT(UpdateDetentTable()));
    connect(m_MaxSpinBox, SIGNAL(valueChanged(double)), this, SLOT(UpdateDetentTable()));
    connect(m_DetentCountSpinBox, SIGNAL(valueChanged(int)), this, SLOT(UpdateDetentTable()));

    UpdateDetentTable();
    UpdatePreviewLabel();
}

RotarySwitchDialogFrame::~RotarySwitchDialogFrame()
{
}

QStringList RotarySwitchDialogFrame::CollectLabels() const
{
    QStringList Labels;
    for (int x = 0; x < m_DetentTable->rowCount(); x++) {
        QTableWidgetItem *Item = m_DetentTable->item(x, 1);
        Labels.append((Item == nullptr) ? QString() : Item->text().trimmed());
    }
    return Labels;
}

void RotarySwitchDialogFrame::UpdateDetentTable()
{
    // texts already typed in survive a change of the number of detents
    QStringList Labels = (m_DetentTable->rowCount() > 0) ? CollectLabels() : m_RotarySwitch->GetDetentLabels();

    double Min = m_MinSpinBox->value();
    double Max = m_MaxSpinBox->value();
    if (Max <= Min) {
        Max = Min + 1.0;
    }
    int Count = m_DetentCountSpinBox->value();

    m_DetentTable->setRowCount(Count);
    for (int x = 0; x < Count; x++) {
        double Value = Min + ((Max - Min) * static_cast<double>(x)) / static_cast<double>(Count - 1);
        QTableWidgetItem *ValueItem = new QTableWidgetItem(QString::number(Value));
        ValueItem->setFlags(ValueItem->flags() & ~Qt::ItemIsEditable);
        m_DetentTable->setItem(x, 0, ValueItem);
        m_DetentTable->setItem(x, 1, new QTableWidgetItem(Labels.value(x)));
    }
    m_DetentTable->resizeColumnToContents(0);
}

void RotarySwitchDialogFrame::UpdatePreviewLabel()
{
    double Min = m_MinSpinBox->value();
    double Max = m_MaxSpinBox->value();
    if (Max <= Min) {
        Max = Min + 1.0;
    }
    int Count = m_DetentCountSpinBox->value();
    double Step = (Max - Min) / static_cast<double>(Count - 1);
    double Threshold = (Max - Min) * m_ThresholdSpinBox->value() / 100.0;

    QString Text = tr("Detents every %1, first at %2, last at %3.")
                       .arg(Step).arg(Min).arg(Max);
    m_ThresholdSpinBox->setEnabled(!m_SnapOnlyCheckBox->isChecked());
    if (m_SnapOnlyCheckBox->isChecked()) {
        Text += QLatin1String(" ") + tr("Only these values can be set.");
    } else {
        Text += QLatin1String(" ") + tr("A detent captures the knob within +/-%1.").arg(Threshold);
        if (Threshold >= (Step / 2.0)) {
            Text += QLatin1String(" ") + tr("The bands touch each other, so no free value is left in between.");
        }
    }
    m_PreviewLabel->setText(Text);
}

void RotarySwitchDialogFrame::userAccept()
{
    m_RotarySwitch->SetRange(m_MinSpinBox->value(), m_MaxSpinBox->value());
    m_RotarySwitch->SetDetentCount(m_DetentCountSpinBox->value());
    m_RotarySwitch->SetSnapThreshold(m_ThresholdSpinBox->value());
    m_RotarySwitch->SetSnapOnly(m_SnapOnlyCheckBox->isChecked());
    m_RotarySwitch->SetDetentLabels(CollectLabels());
}

void RotarySwitchDialogFrame::userReject()
{
    // nothing was applied live, so there is nothing to roll back
}
