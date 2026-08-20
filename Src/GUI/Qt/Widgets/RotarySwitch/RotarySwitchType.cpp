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


#include "RotarySwitchType.h"
#include "RotarySwitchWidget.h"
#include "OpenElementDialog.h"

RotarySwitchType::RotarySwitchType(QObject* parent) : MdiWindowType(FastUpdateWindow,
                                                    "RotarySwitch",
                                                    "GUI/AllRotarySwitchWindows",
                                                    "Rotary switch",
                                                    "RotarySwitch", parent,
                                                    QIcon(":/Icons/Knob.png"),
                                                    60, 60, 140, 160)
{
}

RotarySwitchType::~RotarySwitchType()
{
}

MdiWindowWidget* RotarySwitchType::newElement(MdiSubWindow* par_SubWindow)
{
    RotarySwitchWidget *loc_rotarySwitch = new RotarySwitchWidget(GetDefaultNewWindowTitle(), par_SubWindow, this);
    loc_rotarySwitch->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    return loc_rotarySwitch;
}

MdiWindowWidget* RotarySwitchType::openElement(MdiSubWindow* par_SubWindow, QString &arg_WindowName)
{
    if (arg_WindowName == nullptr) {
        return newElement(par_SubWindow);
    } else {
        RotarySwitchWidget *loc_rotarySwitch = new RotarySwitchWidget(arg_WindowName, par_SubWindow, this);
        return loc_rotarySwitch;
    }
}

QStringList RotarySwitchType::openElementDialog()
{
    QStringList WindowNameList;
    OpenElementDialog dlg(GetWindowIniListName(), GetWindowTypeName());
    if (dlg.exec() == QDialog::Accepted) {
        WindowNameList = dlg.getToOpenWidgetName();
    }
    return WindowNameList;
}
