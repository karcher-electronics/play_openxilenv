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


#ifndef ROTARYSWITCHDIALOGFRAME_H
#define ROTARYSWITCHDIALOGFRAME_H

#include "DialogFrame.h"
#include "RotarySwitchWidget.h"

#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QSpinBox>
#include <QTableWidget>

class RotarySwitchDialogFrame : public CustomDialogFrame
{
    Q_OBJECT
public:
    explicit RotarySwitchDialogFrame(RotarySwitchWidget *arg_RotarySwitch, QWidget *arg_parent = nullptr);
    ~RotarySwitchDialogFrame();

    virtual void userAccept() Q_DECL_OVERRIDE;
    virtual void userReject() Q_DECL_OVERRIDE;

private slots:
    void UpdatePreviewLabel();
    void UpdateDetentTable();

private:
    QStringList CollectLabels() const;

    RotarySwitchWidget *m_RotarySwitch;

    QDoubleSpinBox *m_MinSpinBox;
    QDoubleSpinBox *m_MaxSpinBox;
    QSpinBox *m_DetentCountSpinBox;
    QDoubleSpinBox *m_ThresholdSpinBox;
    QCheckBox *m_SnapOnlyCheckBox;
    QLabel *m_PreviewLabel;
    QTableWidget *m_DetentTable;
};

#endif // ROTARYSWITCHDIALOGFRAME_H
