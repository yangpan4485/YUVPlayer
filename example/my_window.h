#pragma once

#include "UIlib.h"

class CMyWindowUI : public DuiLib::CControlUI
{
public:
    CMyWindowUI();

    ~CMyWindowUI();

    virtual void SetInternVisible(bool bVisible = true) override;

    virtual void SetPos(RECT rc);

    BOOL Attach(HWND hWndNew);

    HWND Detach();

    HWND GetHwnd();

private:
    HWND _hWnd{};
};