/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_EDITOR_DECALOBJECTPANEL_H
#define CRYINCLUDE_EDITOR_DECALOBJECTPANEL_H
#pragma once


#include <QWidget>

#include <ui_DecalObjectPanel.h>

class CDecalObject;


class CDecalObjectTool
    : public CEditTool
{
    Q_OBJECT
public:
    CDecalObjectTool();

    virtual void Display(DisplayContext& dc) {};
    virtual bool MouseCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags);
    virtual bool OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags);
    virtual bool OnKeyUp(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags);
    virtual void SetUserData(const char* userKey, void* userData);

protected:
    virtual ~CDecalObjectTool();
    void DeleteThis() { delete this; };

private:
    CDecalObject* m_pDecalObj;
};

class CDecalObjectPanel
    : public QWidget
{
    Q_OBJECT

public:
    CDecalObjectPanel(QWidget* pParent = nullptr);
    virtual ~CDecalObjectPanel();

    void SetDecalObject(CDecalObject* pObj);

protected slots:
    void OnUpdateProjection();

protected:
    QScopedPointer<Ui::DecalObjectPanel> m_ui;

    CDecalObject* m_pDecalObj;
};


#endif // CRYINCLUDE_EDITOR_DECALOBJECTPANEL_H
