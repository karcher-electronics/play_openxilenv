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


#ifndef ROTARYSWITCHWIDGET_H
#define ROTARYSWITCHWIDGET_H

#include "MdiWindowWidget.h"
#include "BlackboardObserver.h"

#include <QColor>
#include <QFont>

// A rotary switch that behaves like a potentiometer: the value follows the
// mouse continuously, but as soon as it comes close enough to one of the
// detent positions it snaps onto it and stays there until the user leaves the
// capture band again.
class RotarySwitchWidget : public MdiWindowWidget
{
    Q_OBJECT
public:
    explicit RotarySwitchWidget(QString par_WindowTitle, MdiSubWindow* par_SubWindow, MdiWindowType *par_Type, QWidget *parent = nullptr);
    ~RotarySwitchWidget() Q_DECL_OVERRIDE;

    virtual bool writeToIni() Q_DECL_OVERRIDE;
    virtual bool readFromIni() Q_DECL_OVERRIDE;
    virtual CustomDialogFrame* dialogSettings(QWidget *arg_parent) Q_DECL_OVERRIDE;

    QString GetVariableName() const;

    double GetMin() const;
    double GetMax() const;
    void SetRange(double arg_Min, double arg_Max);

    int GetDetentCount() const;
    void SetDetentCount(int arg_Count);

    // Half width of the capture band around a detent, in percent of the whole
    // value range. Inside this band the value is pulled onto the detent.
    double GetSnapThreshold() const;
    void SetSnapThreshold(double arg_Percent);

    bool GetSnapOnly() const;
    void SetSnapOnly(bool arg_SnapOnly);

    // One label per detent position ("OFF", "ON", "10%", ...). Entries may be
    // empty, the list may be shorter than the number of detents.
    QStringList GetDetentLabels() const;
    void SetDetentLabels(const QStringList &arg_Labels);
    QString DetentLabel(int arg_Index) const;

    // Detent index 0 .. GetDetentCount()-1 mapped onto the value range.
    double DetentValue(int arg_Index) const;

protected:
    virtual void dragEnterEvent(QDragEnterEvent *event) Q_DECL_OVERRIDE;
    virtual void dragMoveEvent(QDragMoveEvent *event) Q_DECL_OVERRIDE;
    virtual void dragLeaveEvent(QDragLeaveEvent *event) Q_DECL_OVERRIDE;
    virtual void dropEvent(QDropEvent *event) Q_DECL_OVERRIDE;
    void contextMenuEvent(QContextMenuEvent *event) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void wheelEvent(QWheelEvent *event) Q_DECL_OVERRIDE;
    void keyPressEvent(QKeyEvent *event) Q_DECL_OVERRIDE;
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;

private slots:
    virtual void CyclicUpdate() Q_DECL_OVERRIDE;
    void blackboardVariableConfigChanged(int arg_vid, unsigned int arg_observationFlag);
    void ConfigureSlot();
    virtual void changeColor(QColor arg_color) Q_DECL_OVERRIDE;
    virtual void changeFont(QFont arg_font) Q_DECL_OVERRIDE;
    virtual void changeWindowName(QString arg_name) Q_DECL_OVERRIDE;
    virtual void changeVariable(QString arg_variable, bool arg_visible) Q_DECL_OVERRIDE;
    virtual void changeVaraibles(QStringList arg_variables, bool arg_visible) Q_DECL_OVERRIDE;
    virtual void resetDefaultVariables(QStringList arg_variables) Q_DECL_OVERRIDE;

private:
    void openDialog() Q_DECL_OVERRIDE;
    // The variable name is configuration and is kept independent of the
    // blackboard, so it survives a variable that is (temporarily) not there.
    void SetVariable(const QString &arg_VariableName);
    void ClearVariable();
    void AttachToBlackboard();
    void DetachFromBlackboard();

    double ValueToAngle(double arg_Value) const;
    double AngleToValue(double arg_Angle) const;
    double AngleOfPoint(const QPoint &arg_Pos) const;
    // Returns the detent index the value is captured by, -1 if it is free.
    int DetentIndexOf(double arg_Value) const;
    int NearestDetentIndex(double arg_Value) const;
    double ApplySnap(double arg_Value) const;
    double SnapThresholdValue() const;
    void SetValue(double arg_Value, bool arg_WriteToBlackboard);
    void StepDetent(int arg_Direction);
    QRectF KnobRect() const;
    QString ValueText() const;
    bool HasAnyDetentLabel() const;

    QString m_VariableName;
    int m_Vid;

    double m_Value;
    double m_Min;
    double m_Max;
    int m_DetentCount;
    double m_SnapThreshold;   // percent of the value range
    bool m_SnapOnly;
    QStringList m_DetentLabels;

    bool m_Dragging;

    QColor m_Color;
    QFont m_Font;

    BlackboardObserverConnection m_ObserverConnection;
};

#endif // ROTARYSWITCHWIDGET_H
