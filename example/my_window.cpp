#include "my_window.h"

CMyWindowUI::CMyWindowUI()
{

}

CMyWindowUI::~CMyWindowUI()
{

}

void CMyWindowUI::SetInternVisible(bool bVisible)
{
    __super::SetInternVisible(bVisible);
    ::ShowWindow(_hWnd, bVisible);
}

void CMyWindowUI::SetPos(RECT rc)
{
    __super::SetPos(rc);
    ::SetWindowPos(_hWnd, NULL, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, SWP_NOZORDER | SWP_NOACTIVATE);
}

BOOL CMyWindowUI::Attach(HWND hWndNew)
{
    if (!::IsWindow(hWndNew))
    {
        return FALSE;
    }

    _hWnd = hWndNew;
    return TRUE;
}

HWND CMyWindowUI::Detach()
{
    HWND hWnd = _hWnd;
    _hWnd = NULL;
    return hWnd;
}

HWND CMyWindowUI::GetHwnd()
{
    return _hWnd;
}