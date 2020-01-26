#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shlwapi.h>
#include <strsafe.h>
#include <cstdio>
#include <cassert>
#include "MRegKey.hpp"
#include "resource.h"

#define IDW_STATUS 1
#define IDW_TOOLBAR 2
#define IDW_LISTVIEW 3

#define NUM_TOOLS 32
#define CX_TOOL 24
#define CY_TOOL 24

enum VIEW
{
    STANDARD_VIEW,
    LIST_VIEW
};

HINSTANCE g_hInstance = NULL;
HICON s_hIcon = NULL;
HICON s_hIconSmall = NULL;

HWND g_hwnd = NULL;
HWND s_hStatusBar = NULL;
HWND s_hToolBar = NULL;
HWND s_hListView = NULL;

VIEW s_nView = LIST_VIEW;

const TCHAR g_szClassName[] = TEXT("ImagePocke by katahiromz");

static BOOL s_bModified = FALSE;
static TCHAR s_szFileName[MAX_PATH] = TEXT("");
static HACCEL s_hMainAccel = NULL;
static HIMAGELIST s_himlToolBar = NULL;

static BOOL s_bMaximized = FALSE;
static INT s_nWindowX = CW_USEDEFAULT;
static INT s_nWindowY = CW_USEDEFAULT;
static INT s_nWindowCX = CW_USEDEFAULT;
static INT s_nWindowCY = CW_USEDEFAULT;
static BOOL s_bShowStatusBar = FALSE;
static BOOL s_bShowToolBar = FALSE;

static TBBUTTON s_buttons[] =
{
    { 0, ID_NEW, TBSTATE_ENABLED, BTNS_BUTTON, NULL, 0 },
    { 1, ID_OPEN, TBSTATE_ENABLED, BTNS_BUTTON, NULL, 0 },
    { 2, ID_SAVE, TBSTATE_ENABLED, BTNS_BUTTON, NULL, 0 },
    { 0, 0, 0, BTNS_SEP, NULL, 0 },
    { 3, ID_PAGE_LAYOUT, TBSTATE_ENABLED, BTNS_BUTTON, NULL, 0 },
    { 4, ID_PRINT, TBSTATE_ENABLED, BTNS_BUTTON, NULL, 0 },
    { 5, ID_PRINT_PREVIEW, TBSTATE_ENABLED, BTNS_BUTTON, NULL, 0 },
    { 0, 0, 0, BTNS_SEP, NULL, 0 },
    { 6, ID_CUT, TBSTATE_ENABLED, BTNS_BUTTON, NULL, 0 },
    { 7, ID_COPY, TBSTATE_ENABLED, BTNS_BUTTON, NULL, 0 },
    { 8, ID_PASTE, TBSTATE_ENABLED, BTNS_BUTTON, NULL, 0 },
    { 0, 0, 0, BTNS_SEP, NULL, 0 },
    { 9, ID_UNDO, TBSTATE_ENABLED, BTNS_BUTTON, NULL, 0 },
    { 10, ID_REDO, TBSTATE_ENABLED, BTNS_BUTTON, NULL, 0 },
    { 0, 0, 0, BTNS_SEP, NULL, 0 },
    { 12, ID_ALIGN_LEFT, TBSTATE_ENABLED, BTNS_BUTTON, NULL, 0 },
    { 13, ID_ALIGN_CENTER, TBSTATE_ENABLED, BTNS_BUTTON, NULL, 0 },
    { 14, ID_ALIGN_RIGHT, TBSTATE_ENABLED, BTNS_BUTTON, NULL, 0 },
    { 0, 0, 0, BTNS_SEP, NULL, 0 },
    { 15, ID_VALIGN_TOP, TBSTATE_ENABLED, BTNS_BUTTON, NULL, 0 },
    { 16, ID_VALIGN_MIDDLE, TBSTATE_ENABLED, BTNS_BUTTON, NULL, 0 },
    { 17, ID_VALIGN_BOTTOM, TBSTATE_ENABLED, BTNS_BUTTON, NULL, 0 },
    { 0, 0, 0, BTNS_SEP, NULL, 0 },
    { 19, ID_TEXT_BOLD, TBSTATE_ENABLED, BTNS_BUTTON, NULL, 0 },
    { 20, ID_TEXT_ITALIC, TBSTATE_ENABLED, BTNS_BUTTON, NULL, 0 },
    { 21, ID_TEXT_UNDERLINE, TBSTATE_ENABLED, BTNS_BUTTON, NULL, 0 },
    { 0, 0, 0, BTNS_SEP, NULL, 0 },
    { 25, ID_ZOOM_IN, TBSTATE_ENABLED, BTNS_BUTTON, NULL, 0 },
    { 26, ID_ZOOM_OUT, TBSTATE_ENABLED, BTNS_BUTTON, NULL, 0 },
    { 0, 0, 0, BTNS_SEP, NULL, 0 },
    { 31, ID_SHOW_README, TBSTATE_ENABLED, BTNS_BUTTON, NULL, 0 },
};

LPTSTR LoadStringDx(INT nID)
{
    static UINT s_index = 0;
    const UINT cchBuffMax = 1024;
    static TCHAR s_sz[4][cchBuffMax];

    TCHAR *pszBuff = s_sz[s_index];
    s_index = (s_index + 1) % _countof(s_sz);
    pszBuff[0] = 0;
    if (!::LoadString(NULL, nID, pszBuff, cchBuffMax))
        assert(0);
    return pszBuff;
}

LPTSTR MakeFilterDx(LPTSTR psz)
{
    for (LPTSTR pch = psz; *pch; ++pch)
    {
        if (*pch == TEXT('|'))
            *pch = 0;
    }
    return psz;
}

LPTSTR DoGetFileName(HWND hwnd)
{
    if (s_szFileName[0])
        return s_szFileName;
    return LoadStringDx(IDS_UNTITLED);
}

BOOL DoSetFileName(HWND hwnd, LPCTSTR pszFileName)
{
    // store s_szFileName and get szName
    TCHAR szName[128];
    if (pszFileName && pszFileName[0])
    {
        StringCbCopy(s_szFileName, sizeof(s_szFileName), pszFileName);
        GetFileTitle(s_szFileName, szName, ARRAYSIZE(szName));
    }
    else
    {
        s_szFileName[0] = 0;
        StringCbCopy(szName, sizeof(szName), LoadStringDx(IDS_UNTITLED));
    }

    // set title
    TCHAR szTitle[MAX_PATH + 256];
    LPTSTR psz = LoadStringDx(s_bModified ? IDS_TITLEWITHNAMEMODIFIED : IDS_TITLEWITHNAME);
    StringCbPrintf(szTitle, sizeof(szTitle), psz, szName);
    SetWindowText(hwnd, szTitle);

    return TRUE;
}

BOOL DoOpen(HWND hwnd, LPCTSTR pszFileName)
{
    assert(pszFileName);
    assert(pszFileName[0]);

    // TODO:

    s_bModified = FALSE;
    DoSetFileName(hwnd, pszFileName);
    return TRUE;
}

BOOL DoSaveAs(HWND hwnd);

BOOL DoSave(HWND hwnd, LPCTSTR pszFileName)
{
    if (!pszFileName || !pszFileName[0])
        return DoSaveAs(hwnd);

    // TODO:

    s_bModified = FALSE;
    DoSetFileName(hwnd, pszFileName);
    return TRUE;
}

BOOL DoSaveAs(HWND hwnd)
{
    TCHAR szFile[MAX_PATH];

    if (!s_szFileName[0])
        StringCbCopy(szFile, sizeof(szFile), LoadStringDx(IDS_UNTITLED));
    else
        StringCbCopy(szFile, sizeof(szFile), s_szFileName);

    OPENFILENAME ofn = { OPENFILENAME_SIZE_VERSION_400 };
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_IPKFILTER));
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = ARRAYSIZE(szFile);
    ofn.lpstrTitle = LoadStringDx(IDS_SAVEAS);
    ofn.Flags = OFN_EXPLORER | OFN_ENABLESIZING |
                OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
    ofn.lpstrDefExt = TEXT("ipk");
    if (GetSaveFileName(&ofn))
    {
        return DoSave(hwnd, szFile);
    }

    return FALSE;
}

BOOL ParseCommandLine(HWND hwnd, INT argc, LPWSTR *wargv)
{
    return TRUE;
}

void DoResetSettings(void)
{
    s_nWindowX = CW_USEDEFAULT;
    s_nWindowY = CW_USEDEFAULT;
    s_nWindowCX = CW_USEDEFAULT;
    s_nWindowCY = CW_USEDEFAULT;
    s_bMaximized = FALSE;
    s_bShowStatusBar = TRUE;
    s_bShowToolBar = TRUE;
}

BOOL DoLoadSettings(void)
{
    DoResetSettings();

    MRegKey keyCompany(HKEY_CURRENT_USER, TEXT("Software\\Katayama Hirofumi MZ"), FALSE);
    if (!keyCompany)
        return FALSE;

    MRegKey keyApp(keyCompany, TEXT("ImagePocke"), FALSE);
    if (!keyApp)
        return FALSE;

    keyApp.QueryDword(TEXT("WindowX"), (DWORD&)s_nWindowX);
    keyApp.QueryDword(TEXT("WindowY"), (DWORD&)s_nWindowY);
    keyApp.QueryDword(TEXT("WindowCX"), (DWORD&)s_nWindowCX);
    keyApp.QueryDword(TEXT("WindowCY"), (DWORD&)s_nWindowCY);
    keyApp.QueryDword(TEXT("Maximized"), (DWORD&)s_bMaximized);
    keyApp.QueryDword(TEXT("ShowStatusBar"), (DWORD&)s_bShowStatusBar);
    keyApp.QueryDword(TEXT("ShowToolBar"), (DWORD&)s_bShowToolBar);
    return TRUE;
}

BOOL DoSaveSettings(void)
{
    MRegKey keyCompany(HKEY_CURRENT_USER, TEXT("Software\\Katayama Hirofumi MZ"), TRUE);
    if (!keyCompany)
        return FALSE;

    MRegKey keyApp(keyCompany, TEXT("ImagePocke"), TRUE);
    if (!keyApp)
        return FALSE;

    keyApp.SetDword(TEXT("WindowX"), s_nWindowX);
    keyApp.SetDword(TEXT("WindowY"), s_nWindowY);
    keyApp.SetDword(TEXT("WindowCX"), s_nWindowCX);
    keyApp.SetDword(TEXT("WindowCY"), s_nWindowCY);
    keyApp.SetDword(TEXT("Maximized"), s_bMaximized);
    keyApp.SetDword(TEXT("ShowStatusBar"), s_bShowStatusBar);
    keyApp.SetDword(TEXT("ShowToolBar"), s_bShowToolBar);
    return TRUE;
}

static void OnNew(HWND hwnd)
{
    s_bModified = FALSE;
    DoSetFileName(hwnd, NULL);
}

static void OnOpen(HWND hwnd)
{
    TCHAR szFile[MAX_PATH] = TEXT("");

    OPENFILENAME ofn = { OPENFILENAME_SIZE_VERSION_400 };
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_IPKFILTER));
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = ARRAYSIZE(szFile);
    ofn.lpstrTitle = LoadStringDx(IDS_OPEN);
    ofn.Flags = OFN_EXPLORER | OFN_ENABLESIZING |
                OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    ofn.lpstrDefExt = TEXT("ipk");
    if (GetOpenFileName(&ofn))
    {
        DoOpen(hwnd, szFile);
    }
}

BOOL DoCreateToolBar(HWND hwnd)
{
    DWORD dwStyle = WS_CHILD | CCS_TOP | TBSTYLE_WRAPABLE;
    s_hToolBar = CreateWindowEx(0, TOOLBARCLASSNAME, NULL,
        dwStyle, 0, 0, 0, 0,
        hwnd, (HMENU)IDW_TOOLBAR, g_hInstance, NULL);
    if (!s_hToolBar)
        return FALSE;

    s_himlToolBar = ImageList_Create(CX_TOOL, CY_TOOL, ILC_COLOR24, NUM_TOOLS, 0);

    HBITMAP hbm = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_TOOLBAR));
    ImageList_Add(s_himlToolBar, hbm, NULL);
    DeleteObject(hbm);

    SendMessage(s_hToolBar, TB_SETIMAGELIST, 0, (LPARAM)s_himlToolBar);
    SendMessage(s_hToolBar, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);
    SendMessage(s_hToolBar, TB_SETBUTTONSIZE, 0, MAKELONG(CX_TOOL, CY_TOOL));
    SendMessage(s_hToolBar, TB_ADDBUTTONS, (WPARAM)ARRAYSIZE(s_buttons), (LPARAM)s_buttons);
    SendMessage(s_hToolBar, TB_AUTOSIZE, 0, 0);
    return TRUE;
}


BOOL DoCreateListView(HWND hwnd)
{
    DWORD dwStyle = WS_CHILD | LVS_REPORT | LVS_EDITLABELS;

    s_hListView = CreateWindowEx(0, WC_LISTVIEW, NULL, dwStyle,
        0, 0, 0, 0, hwnd, (HMENU)IDW_LISTVIEW, g_hInstance, NULL);
    if (!s_hListView)
        return FALSE;

    INT iColumn = 0;
    LV_COLUMN column = { LVCF_TEXT | LVCF_WIDTH | LVCF_FMT | LVCF_SUBITEM };

    column.fmt = LVCFMT_LEFT;
    column.cx = 100;
    column.pszText = TEXT("Name");
    column.iSubItem = iColumn;
    ListView_InsertColumn(s_hListView, iColumn, &column);
    ++iColumn;

    column.fmt = LVCFMT_LEFT | LVCFMT_COL_HAS_IMAGES;
    column.cx = 100;
    column.pszText = TEXT("Image");
    column.iSubItem = iColumn;
    ListView_InsertColumn(s_hListView, iColumn, &column);
    ++iColumn;

    column.fmt = LVCFMT_LEFT;
    column.cx = 400;
    column.pszText = TEXT("Description");
    column.iSubItem = iColumn;
    ListView_InsertColumn(s_hListView, iColumn, &column);
    ++iColumn;

    return TRUE;
}

static BOOL OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
{
    g_hwnd = hwnd;

    SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)s_hIcon);
    SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)s_hIconSmall);

    DragAcceptFiles(hwnd, TRUE);

    s_hStatusBar = CreateStatusWindow(WS_CHILD, L"", hwnd, IDW_STATUS);
    if (s_bShowStatusBar)
        ShowWindow(s_hStatusBar, SW_SHOWNORMAL);

    DoCreateToolBar(hwnd);
    if (s_bShowToolBar)
        ShowWindow(s_hToolBar, SW_SHOWNORMAL);

    DoCreateListView(hwnd);
    ShowWindow(s_hListView, SW_SHOWNORMAL);

    DoSetFileName(hwnd, NULL);

    PostMessage(hwnd, WM_COMMAND, 0, 0);

    return TRUE;
}

void OnExit(HWND hwnd)
{
    PostMessage(hwnd, WM_CLOSE, 0, 0);
}

void OnShowReadMe(HWND hwnd)
{
    // TODO:
}

void OnAbout(HWND hwnd)
{
    MSGBOXPARAMS params = { sizeof(params) };
    params.hwndOwner = hwnd;
    params.hInstance = g_hInstance;
    params.lpszText = LoadStringDx(IDS_VERSIONINFO);
    params.lpszCaption = LoadStringDx(IDS_APPNAME);
    params.dwStyle = MB_USERICON;
    params.lpszIcon = MAKEINTRESOURCE(IDI_MAIN);
    MessageBoxIndirectW(&params);
}

void OnToolBar(HWND hwnd)
{
    if (IsWindowVisible(s_hToolBar))
    {
        s_bShowToolBar = FALSE;
        ShowWindow(s_hToolBar, SW_HIDE);
    }
    else
    {
        s_bShowToolBar = TRUE;
        ShowWindow(s_hToolBar, SW_SHOWNORMAL);
    }
    PostMessage(hwnd, WM_SIZE, 0, 0);
}

void OnStatusBar(HWND hwnd)
{
    if (IsWindowVisible(s_hStatusBar))
    {
        s_bShowStatusBar = FALSE;
        ShowWindow(s_hStatusBar, SW_HIDE);
    }
    else
    {
        s_bShowStatusBar = TRUE;
        ShowWindow(s_hStatusBar, SW_SHOWNORMAL);
    }
    PostMessage(hwnd, WM_SIZE, 0, 0);
}

static void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    static INT s_nLock = 0;

    if (s_nLock == 0)
    {
        SendMessage(s_hStatusBar, SB_SETTEXT, 0 | 0, (LPARAM)LoadStringDx(IDS_EXECUTINGCMD));
    }

    ++s_nLock;

    switch (id)
    {
    case ID_NEW:
        OnNew(hwnd);
        break;
    case ID_OPEN:
        OnOpen(hwnd);
        break;
    case ID_SAVE:
        DoSave(hwnd, s_szFileName);
        break;
    case ID_SAVE_AS:
        DoSaveAs(hwnd);
        break;
    case ID_PAGE_LAYOUT:
    case ID_PRINT_PREVIEW:
    case ID_PRINT:
    case ID_EXPORT_PDF:
    case ID_EXPORT_SQL:
        break;
    case ID_EXIT:
        OnExit(hwnd);
        break;
    case ID_UNDO:
    case ID_REDO:
    case ID_CUT:
    case ID_COPY:
    case ID_PASTE:
    case ID_SELECT_ALL:
    case ID_FIND:
    case ID_REPLACE:
    case ID_CLEAR:
        break;
    case ID_STATUS_BAR:
        OnStatusBar(hwnd);
        break;
    case ID_TOOL_BAR:
        OnToolBar(hwnd);
        break;
    case ID_VIEW_STANDARD:
    case ID_VIEW_LIST:
    case ID_ZOOM_50:
    case ID_ZOOM_75:
    case ID_ZOOM_100:
    case ID_ZOOM_150:
    case ID_ZOOM_200:
    case ID_ZOOM:
    case ID_VIEW_IMAGES:
    case ID_VIEW_BORDERS:
    case ID_TEXT_BOLD:
    case ID_TEXT_ITALIC:
    case ID_TEXT_UNDERLINE:
    case ID_TEXT_UPPERCASE:
    case ID_TEXT_LOWERCASE:
    case ID_TEXT_TITLECASE:
    case ID_TEXT_FULLWIDTH:
    case ID_TEXT_HALFWIDTH:
    case ID_TEXT_HIRAGANA:
    case ID_TEXT_KATAKANA:
    case ID_TEXT_COLOR:
    case ID_TEXT_FONT:
    case ID_IMAGE_SIZE:
    case ID_IMAGE_SIZE_AUTO:
    case ID_IMAGE_SIZE_FIXED:
    case ID_IMAGE_TRIMMING:
    case ID_IMAGE_KEEP_ASPECT:
    case ID_IMAGE_ROTATE_90:
    case ID_IMAGE_ROTATE_180:
    case ID_IMAGE_ROTATE_270:
    case ID_IMAGE_FLIP_V:
    case ID_IMAGE_FLIP_H:
    case ID_IMAGE_NO_ROTATE:
    case ID_IMAGE_BRIGHTNESS:
    case ID_IMAGE_HUE:
    case ID_IMAGE_AUTO_COLOR:
    case ID_IMAGE_FILTER_BLUR:
    case ID_IMAGE_FILTER_SHARPEN:
    case ID_LINE_SPACE_1:
    case ID_LINE_SPACE_1_5:
    case ID_LINE_SPACE_2:
    case ID_INDENT_INCREASE:
    case ID_INDENT_DECREASE:
    case ID_ALIGN_LEFT:
    case ID_ALIGN_CENTER:
    case ID_ALIGN_RIGHT:
    case ID_VALIGN_TOP:
    case ID_VALIGN_MIDDLE:
    case ID_VALIGN_BOTTOM:
    case ID_CELLS:
    case ID_ROW_HEIGHT:
    case ID_ROW_HIDE:
    case ID_ROW_SHOW:
    case ID_COLUMN_WIDTH:
    case ID_COLUMN_HIDE:
    case ID_COLUMN_SHOW:
    case ID_CELL_MERGE:
    case ID_CELL_SPLIT:
    case ID_SHOW_README:
        OnShowReadMe(hwnd);
        break;
    case ID_ABOUT:
        OnAbout(hwnd);
        break;
    case ID_VIEW_TEXTS:
    case ID_ZOOM_IN:
    case ID_ZOOM_OUT:
        break;
    }
    --s_nLock;

    if (s_nLock == 0)
    {
        SendMessage(s_hStatusBar, SB_SETTEXT, 0 | 0, (LPARAM)LoadStringDx(IDS_READY));
    }
}

static void OnDropFiles(HWND hwnd, HDROP hdrop)
{
}

static void OnClose(HWND hwnd)
{
    if (s_bModified)
    {
        TCHAR szText[MAX_PATH + 256];
        StringCbPrintf(szText, sizeof(szText), LoadStringDx(IDS_QUERYSAVE), DoGetFileName(hwnd));
        INT nID = MessageBox(hwnd, szText, LoadStringDx(IDS_APPNAME), MB_ICONINFORMATION | MB_YESNOCANCEL);
        switch (nID)
        {
        case IDYES:
            DoSave(hwnd, s_szFileName);
            break;
        case IDNO:
            break;
        case IDCANCEL:
            return;
        }
    }

    DestroyWindow(hwnd);
}

static void OnDestroy(HWND hwnd)
{
    DestroyWindow(s_hStatusBar);
    s_hStatusBar = NULL;

    DestroyWindow(s_hToolBar);
    s_hToolBar = NULL;

    DestroyWindow(s_hListView);
    s_hListView = NULL;

    ImageList_Destroy(s_himlToolBar);

    PostQuitMessage(0);
    g_hwnd = NULL;
}

static void OnMove(HWND hwnd, int x, int y)
{
    if (IsMinimized(hwnd) || IsMaximized(hwnd))
        return;

    RECT rc;
    GetWindowRect(hwnd, &rc);
    s_nWindowX = rc.left;
    s_nWindowY = rc.top;
}

static void OnSize(HWND hwnd, UINT state, int cx, int cy)
{
    RECT rc;
    INT cyToolBar = 0, cyStatusBar = 0;


    if (IsWindowVisible(s_hToolBar))
    {
        INT nCount = (INT)SendMessage(s_hToolBar, TB_BUTTONCOUNT, 0, 0);
        SendMessage(s_hToolBar, TB_GETITEMRECT, nCount - 1, (LPARAM)&rc);
        DWORD dwPadding = SendMessage(s_hToolBar, TB_GETPADDING, 0, 0);
        cyToolBar = rc.bottom + HIWORD(dwPadding);

        GetClientRect(hwnd, &rc);
        MoveWindow(s_hToolBar, 0, 0, rc.right - rc.left, cyToolBar, TRUE);
    }
    if (IsWindowVisible(s_hStatusBar))
    {
        SendMessage(s_hStatusBar, WM_SIZE, 0, 0);
        GetWindowRect(s_hStatusBar, &rc);
        cyStatusBar = rc.bottom - rc.top;
    }

    s_bMaximized = IsMaximized(hwnd);

    GetClientRect(hwnd, &rc);
    INT x = rc.left, y = rc.top;
    cx = rc.right - rc.left;
    cy = rc.bottom - rc.top;

    if (IsWindowVisible(s_hToolBar))
    {
        y += cyToolBar;
        cy -= cyToolBar;
    }
    if (IsWindowVisible(s_hStatusBar))
    {
        cy -= cyStatusBar;
    }

    MoveWindow(s_hListView, x, y, cx, cy, TRUE);

    if (IsMinimized(hwnd) || IsMaximized(hwnd))
        return;

    GetWindowRect(hwnd, &rc);
    s_nWindowCX = rc.right - rc.left;
    s_nWindowCY = rc.bottom - rc.top;
}

static void OnInitMenuPopup(HWND hwnd, HMENU hMenu, UINT item, BOOL fSystemMenu)
{
    if (s_bShowStatusBar)
        CheckMenuItem(hMenu, ID_STATUS_BAR, MF_BYCOMMAND | MF_CHECKED);
    else
        CheckMenuItem(hMenu, ID_STATUS_BAR, MF_BYCOMMAND | MF_UNCHECKED);

    if (s_bShowToolBar)
        CheckMenuItem(hMenu, ID_TOOL_BAR, MF_BYCOMMAND | MF_CHECKED);
    else
        CheckMenuItem(hMenu, ID_TOOL_BAR, MF_BYCOMMAND | MF_UNCHECKED);

    switch (s_nView)
    {
    case STANDARD_VIEW:
        CheckMenuRadioItem(hMenu, ID_VIEW_STANDARD, ID_VIEW_LIST, ID_VIEW_STANDARD, MF_BYCOMMAND);
        break;
    case LIST_VIEW:
        CheckMenuRadioItem(hMenu, ID_VIEW_STANDARD, ID_VIEW_LIST, ID_VIEW_LIST, MF_BYCOMMAND);
        break;
    }
}

LRESULT CALLBACK
WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_CREATE, OnCreate);
        HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
        HANDLE_MSG(hwnd, WM_DROPFILES, OnDropFiles);
        HANDLE_MSG(hwnd, WM_CLOSE, OnClose);
        HANDLE_MSG(hwnd, WM_MOVE, OnMove);
        HANDLE_MSG(hwnd, WM_SIZE, OnSize);
        HANDLE_MSG(hwnd, WM_INITMENUPOPUP, OnInitMenuPopup);
        HANDLE_MSG(hwnd, WM_DESTROY, OnDestroy);
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

BOOL DoRegisterWnds(HINSTANCE hInstance)
{
    WNDCLASSEX wcx = { sizeof(wcx) };
    wcx.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wcx.lpfnWndProc = WindowProc;
    wcx.hInstance = hInstance;
    wcx.hIcon = s_hIcon;
    wcx.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcx.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
    wcx.lpszMenuName = MAKEINTRESOURCE(IDR_MAINMENU);
    wcx.lpszClassName = g_szClassName;
    wcx.hIconSm = s_hIconSmall;
    return !!RegisterClassEx(&wcx);
}

BOOL DoCreateWnds(HINSTANCE hInstance, INT nCmdShow)
{
    HWND hwnd = CreateWindow(g_szClassName, LoadStringDx(IDS_APPNAME),
                        WS_OVERLAPPEDWINDOW,
                        s_nWindowX, s_nWindowY,
                        s_nWindowCX, s_nWindowCY,
                        NULL, NULL, hInstance, NULL);
    if (!hwnd)
        return FALSE;

    if (nCmdShow != SW_HIDE && s_bMaximized)
    {
        nCmdShow = SW_SHOWMAXIMIZED;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);
    return TRUE;
}

BOOL InitInstance(HINSTANCE hInstance, INT nCmdShow)
{
    BOOL ret;
    g_hInstance = hInstance;

    INITCOMMONCONTROLSEX iccx = { sizeof(iccx) };
    iccx.dwICC = ICC_BAR_CLASSES | ICC_LISTVIEW_CLASSES | ICC_STANDARD_CLASSES |
                 ICC_TAB_CLASSES | ICC_TREEVIEW_CLASSES | ICC_UPDOWN_CLASS |
                 ICC_USEREX_CLASSES | ICC_WIN95_CLASSES;
    InitCommonControlsEx(&iccx);

    s_hMainAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_MAINACCEL));

    DoLoadSettings();

    s_hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAIN));
    s_hIconSmall = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_MAIN),
        IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);

    if (!DoRegisterWnds(hInstance))
    {
        MessageBox(NULL, TEXT("RegisterClassEx failed"), NULL, MB_ICONERROR);
        return FALSE;
    }

    if (!DoCreateWnds(hInstance, nCmdShow))
    {
        MessageBox(NULL, TEXT("CreateWindow failed"), NULL, MB_ICONERROR);
        return FALSE;
    }

#ifdef UNICODE
    INT argc = 0;
    LPWSTR *wargv = CommandLineToArgvW(GetCommandLineW(), &argc);
    ret = ParseCommandLine(g_hwnd, argc, wargv);
    LocalFree(wargv);
#else
    ret = ParseCommandLine(g_hwnd, __argc, __argv);
#endif
    if (!ret)
    {
        PostMessage(g_hwnd, WM_DESTROY, 0, 0);
    }

    return ret;
}

INT Run(void)
{
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (TranslateAccelerator(g_hwnd, s_hMainAccel, &msg))
            continue;

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return INT(msg.wParam);
}

void ExitInstance(void)
{
    DoSaveSettings();
    DestroyAcceleratorTable(s_hMainAccel);
    s_hMainAccel = NULL;
}

INT WINAPI
WinMain(HINSTANCE   hInstance,
        HINSTANCE   hPrevInstance,
        LPSTR       lpCmdLine,
        INT         nCmdShow)
{
    INT ret = -1;
    if (InitInstance(hInstance, nCmdShow))
    {
        ret = Run();
    }
    ExitInstance();
    return ret;
}
