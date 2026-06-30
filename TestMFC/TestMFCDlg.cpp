#include "pch.h"
#include "framework.h"
#include "TestMFC.h"
#include "TestMFCDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CTestMFCDlg::CTestMFCDlg(CWnd* pParent /*=nullptr*/)
    : CDialogEx(IDD_TESTMFC_DIALOG, pParent)
    , m_hFreezeThread(nullptr)
    , m_bFreeze(false)
    , m_freezeAddr(0)
    , m_freezeValue(0)
    , m_freezePID(0)
{
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CTestMFCDlg::~CTestMFCDlg()
{
    m_bFreeze = false;
    if (m_hFreezeThread)
    {
        WaitForSingleObject(m_hFreezeThread, 1000);
        CloseHandle(m_hFreezeThread);
        m_hFreezeThread = nullptr;
    }
}

void CTestMFCDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CTestMFCDlg, CDialogEx)
    ON_WM_SYSCOMMAND()
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_BN_CLICKED(IDC_BUTTON_READ,   &CTestMFCDlg::OnBnClickedButtonRead)
    ON_BN_CLICKED(IDC_BUTTON1,       &CTestMFCDlg::OnBnClickedButtonWrite)
    ON_BN_CLICKED(IDC_BUTTON_FREEZE, &CTestMFCDlg::OnBnClickedButtonFreeze)
END_MESSAGE_MAP()

BOOL CTestMFCDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    SetIcon(m_hIcon, TRUE);
    SetIcon(m_hIcon, FALSE);
    return TRUE;
}

void CTestMFCDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
    CDialogEx::OnSysCommand(nID, lParam);
}

void CTestMFCDlg::OnPaint()
{
    if (IsIconic())
    {
        CPaintDC dc(this);
        SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);
        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;
        dc.DrawIcon(x, y, m_hIcon);
    }
    else
    {
        CDialogEx::OnPaint();
    }
}

HCURSOR CTestMFCDlg::OnQueryDragIcon()
{
    return static_cast<HCURSOR>(m_hIcon);
}

// ─────────────────────────────────────────────────────────────
// 공통 헬퍼 : UI에서 PID / 주소(Hex) / 값 읽기
// ─────────────────────────────────────────────────────────────
static bool GetInputs(CWnd* pWnd, DWORD& outPID, uintptr_t& outAddr, int& outValue)
{
    UINT nPID = pWnd->GetDlgItemInt(IDC_EDIT_PID);
    if (nPID == 0)
    {
        AfxMessageBox(L"올바른 PID를 입력해주세요.", MB_ICONWARNING);
        return false;
    }

    CString strAddr;
    pWnd->GetDlgItemText(IDC_EDIT_ADDR, strAddr);
    if (strAddr.IsEmpty())
    {
        AfxMessageBox(L"메모리 주소를 입력해주세요.", MB_ICONWARNING);
        return false;
    }

    outPID   = (DWORD)nPID;
    outAddr  = (uintptr_t)_tcstoui64(strAddr, nullptr, 16);
    outValue = (int)pWnd->GetDlgItemInt(IDC_EDIT_VALUE);
    return true;
}

// ─────────────────────────────────────────────────────────────
// 읽기 버튼 (IDC_BUTTON_READ)
// ─────────────────────────────────────────────────────────────
void CTestMFCDlg::OnBnClickedButtonRead()
{
    DWORD pid; uintptr_t addr; int dummy;
    if (!GetInputs(this, pid, addr, dummy))
        return;

    HANDLE hProc = OpenProcess(PROCESS_VM_READ, FALSE, pid);
    if (!hProc)
    {
        CString msg;
        msg.Format(L"OpenProcess 실패. 에러: %lu (5=관리자 권한 필요)", GetLastError());
        AfxMessageBox(msg, MB_ICONERROR);
        return;
    }

    int readValue = 0;
    SIZE_T bytesRead = 0;
    BOOL ok = ReadProcessMemory(hProc, (LPCVOID)addr, &readValue, sizeof(readValue), &bytesRead);
    CloseHandle(hProc);

    CString result;
    if (ok)
    {
        result.Format(L"[읽기 성공] 0x%IX → %d (0x%X)", addr, readValue, (UINT)readValue);
        SetDlgItemInt(IDC_EDIT_VALUE, (UINT)readValue);
    }
    else
    {
        result.Format(L"[읽기 실패] 에러: %lu", GetLastError());
    }
    SetDlgItemText(IDC_STATIC_RESULT, result);
}

// ─────────────────────────────────────────────────────────────
// 쓰기 버튼 (IDC_BUTTON1)
// ─────────────────────────────────────────────────────────────
void CTestMFCDlg::OnBnClickedButtonWrite()
{
    DWORD pid; uintptr_t addr; int newValue;
    if (!GetInputs(this, pid, addr, newValue))
        return;

    HANDLE hProc = OpenProcess(PROCESS_VM_WRITE | PROCESS_VM_OPERATION, FALSE, pid);
    if (!hProc)
    {
        CString msg;
        msg.Format(L"OpenProcess 실패. 에러: %lu (5=관리자 권한 필요)", GetLastError());
        AfxMessageBox(msg, MB_ICONERROR);
        return;
    }

    SIZE_T bytesWritten = 0;
    BOOL ok = WriteProcessMemory(hProc, (LPVOID)addr, &newValue, sizeof(newValue), &bytesWritten);
    CloseHandle(hProc);

    CString result;
    if (ok)
        result.Format(L"[쓰기 성공] 0x%IX ← %d", addr, newValue);
    else
        result.Format(L"[쓰기 실패] 에러: %lu", GetLastError());
    SetDlgItemText(IDC_STATIC_RESULT, result);
}

// ─────────────────────────────────────────────────────────────
// 고정 스레드 : 100ms 마다 값을 계속 씀
// ─────────────────────────────────────────────────────────────
DWORD WINAPI CTestMFCDlg::FreezeThreadProc(LPVOID lpParam)
{
    CTestMFCDlg* pDlg = reinterpret_cast<CTestMFCDlg*>(lpParam);

    while (pDlg->m_bFreeze)
    {
        HANDLE hProc = OpenProcess(PROCESS_VM_WRITE | PROCESS_VM_OPERATION, FALSE, pDlg->m_freezePID);
        if (hProc)
        {
            WriteProcessMemory(hProc, (LPVOID)pDlg->m_freezeAddr,
                               &pDlg->m_freezeValue, sizeof(pDlg->m_freezeValue), nullptr);
            CloseHandle(hProc);
        }
        Sleep(100);
    }
    return 0;
}

// ─────────────────────────────────────────────────────────────
// 고정 버튼 (IDC_BUTTON_FREEZE) - 토글
// ─────────────────────────────────────────────────────────────
void CTestMFCDlg::OnBnClickedButtonFreeze()
{
    if (m_bFreeze)
    {
        m_bFreeze = false;
        if (m_hFreezeThread)
        {
            WaitForSingleObject(m_hFreezeThread, 1000);
            CloseHandle(m_hFreezeThread);
            m_hFreezeThread = nullptr;
        }
        SetDlgItemText(IDC_STATIC_RESULT, L"[고정 해제됨]");
        GetDlgItem(IDC_BUTTON_FREEZE)->SetWindowText(L"Freeze ON/OFF");
    }
    else
    {
        DWORD pid; uintptr_t addr; int value;
        if (!GetInputs(this, pid, addr, value))
            return;

        m_freezePID   = pid;
        m_freezeAddr  = addr;
        m_freezeValue = value;
        m_bFreeze     = true;

        m_hFreezeThread = CreateThread(nullptr, 0, FreezeThreadProc, this, 0, nullptr);
        if (!m_hFreezeThread)
        {
            m_bFreeze = false;
            AfxMessageBox(L"고정 스레드 생성 실패.", MB_ICONERROR);
            return;
        }

        CString result;
        result.Format(L"[고정 중] 0x%IX = %d (100ms 간격)", addr, value);
        SetDlgItemText(IDC_STATIC_RESULT, result);
        GetDlgItem(IDC_BUTTON_FREEZE)->SetWindowText(L"★ 고정 해제");
    }
}