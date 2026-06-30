#pragma once
#include <Windows.h>

class CTestMFCDlg : public CDialogEx
{
public:
    CTestMFCDlg(CWnd* pParent = nullptr);
    virtual ~CTestMFCDlg();

#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_TESTMFC_DIALOG };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX);

protected:
    HICON m_hIcon;

    // 고정(Freeze) 스레드용 멤버
    HANDLE          m_hFreezeThread;
    volatile bool   m_bFreeze;
    uintptr_t       m_freezeAddr;
    int             m_freezeValue;
    DWORD           m_freezePID;

    static DWORD WINAPI FreezeThreadProc(LPVOID lpParam);

    virtual BOOL OnInitDialog();
    afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    afx_msg void OnBnClickedButtonWrite();
    afx_msg void OnBnClickedButtonRead();
    afx_msg void OnBnClickedButtonFreeze();
    DECLARE_MESSAGE_MAP()
};
