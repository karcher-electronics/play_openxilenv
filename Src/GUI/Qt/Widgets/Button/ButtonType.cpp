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


#include "ButtonType.h"
#include "ButtonWidget.h"
#include "OpenElementDialog.h"

ButtonType::ButtonType(QObject* parent) : MdiWindowType(MediumUpdateWindow,
                                                    "Button",
                                                    "GUI/AllButtonWindows",
                                                    "Button",
                                                    "Button", parent,
                                                    QIcon(":/Icons/Button.png"),
                                                    40, 24, 100, 30)
{
}

ButtonType::~ButtonType()
{
}

MdiWindowWidget* ButtonType::newElement(MdiSubWindow* par_SubWindow)
{
    ButtonWidget *loc_button = new ButtonWidget(GetDefaultNewWindowTitle(), par_SubWindow, this);
    loc_button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    return loc_button;
}

MdiWindowWidget* ButtonType::openElement(MdiSubWindow* par_SubWindow, QString &arg_WindowName)
{
    if (arg_WindowName == nullptr) {
        return newElement(par_SubWindow);
    } else {
        ButtonWidget *loc_button = new ButtonWidget(arg_WindowName, par_SubWindow, this);
        return loc_button;
    }
}

QStringList ButtonType::openElementDialog()
{
    QStringList WindowNameList;
    OpenElementDialog dlg(GetWindowIniListName(), GetWindowTypeName());
    if (dlg.exec() == QDialog::Accepted) {
        WindowNameList = dlg.getToOpenWidgetName();
    }
    return WindowNameList;
}
