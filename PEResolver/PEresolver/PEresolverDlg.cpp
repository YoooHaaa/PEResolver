
// PEresolverDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "PEresolver.h"
#include "PEresolverDlg.h"
#include "afxdialogex.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// ����Ӧ�ó��򡰹��ڡ��˵���� CAboutDlg �Ի���

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// �Ի�������
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

// ʵ��
protected:
	DECLARE_MESSAGE_MAP()
public:
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CPEresolverDlg �Ի���



CPEresolverDlg::CPEresolverDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_PERESOLVER_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CPEresolverDlg::~CPEresolverDlg()
{
    if (m_pView != nullptr)
    {
        //1. ȡ��ӳ����ͼ
        UnmapViewOfFile(m_pView);
        //2. �ر�ӳ�����
        CloseHandle(m_hFileMap);
        //3. �ر��ļ�
        CloseHandle(m_hFile);
    }
}

void CPEresolverDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, TREE_FILE, m_treeFile);
    DDX_Control(pDX, LST_DATA, m_lstData);
    DDX_Control(pDX, EDT_INPUT, m_edtInput);
    DDX_Control(pDX, CHANGE_STA_VA, m_changeStaVA);
    DDX_Control(pDX, CHANGE_STA_RVA, m_changeStaRVA);
    DDX_Control(pDX, CHANGE_STA_OFFSET, m_changeStaOffset);
    DDX_Control(pDX, CHANGE_EDT_VA, m_changeEdtVA);
    DDX_Control(pDX, CHANGE_EDT_RVA, m_changeEdtRVA);
    DDX_Control(pDX, CHANGE_EDT_OFFSET, m_changeEdtOffset);
    DDX_Control(pDX, CHANGE_BTN_OK, m_changeBtnOK);
    DDX_Control(pDX, LST_IMPORT_DLL, m_lstImportDLL);
    DDX_Control(pDX, LST_IMPORT_FUNC, m_lstImportFunc);
    DDX_Control(pDX, IDC_LIST1, m_lstExportDll);
    DDX_Control(pDX, IDC_LIST2, m_lstExportFunc); 
    DDX_Control(pDX, TREE_RESOURCE, m_treeResource);
    DDX_Control(pDX, IDC_LIST3, m_lstResource);

}

bool CPEresolverDlg::CreateFileMap()
{
    //��ȡ�ı�����Ϣ
    GetDlgItemText(EDT_FILEPATH, m_csFilePath);

    //���ļ�m_hFile
    m_hFile = CreateFile(m_csFilePath.GetBuffer(),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (INVALID_HANDLE_VALUE == m_hFile)
    {
        //DWORD dwRet = GetLastError();
        AfxMessageBox("�ļ���ʧ�ܣ�");
        return false;
    }
    

    //��ȡ������
    strcpy_s(m_szPath, MAX_PATH, m_csFilePath.GetBuffer());
    GetProcessName();

    //�����ļ�ӳ�����
    m_hFileMap = CreateFileMapping(
        m_hFile,
        NULL,
        PAGE_READWRITE, //�ɶ���д
        0,
        0, //��ཫ�����ļ�ӳ����ڴ�
        NULL);//û������
    if (m_hFileMap == NULL)
    {
        AfxMessageBox("�ļ�ӳ����󴴽�ʧ��");
        CloseHandle(m_hFile);
        return false;
    }

    //��ȡ�ļ���С
    DWORD dwHigh = 0;
    DWORD dwLow = GetFileSize(m_hFile, &dwHigh);
    //UINT64 m_uFileSize = ((UINT64)dwHigh << 32) + dwLow;//һ���ļ���Сû��8�ֽڣ�4�ֽھ͹�
    m_FileSize = dwLow;

    //����ӳ����ͼ,���ļ�ȫ��ӳ�䵽�ڴ�
    m_pView = MapViewOfFile(
        m_hFileMap,
        FILE_MAP_WRITE, //�ɶ���д
        0,
        0, //��0��ʼӳ����ڴ�
        0); //ӳ�������ļ�
    if (m_pView == NULL)
    {
        AfxMessageBox("�ļ�ӳ��ʧ��");
        CloseHandle(m_hFile);
        CloseHandle(m_hFileMap);
        return false;
    }

    //��ȡ�ṹ���ַ
    m_pDOS = (_IMAGE_DOS_HEADER*)m_pView;
    m_pNT = (_IMAGE_NT_HEADERS*)(m_pDOS->e_lfanew + (int)m_pDOS);
    m_pFILE = (IMAGE_FILE_HEADER*)((int)m_pNT + 4);
    m_pOptional = (IMAGE_OPTIONAL_HEADER32*)((int)m_pFILE + 20);
    m_pSection = (IMAGE_SECTION_HEADER*)((int)m_pOptional + m_pFILE->SizeOfOptionalHeader);

    //��ȡ�ڴ澵���ַ
    m_nImageBase = m_pOptional->ImageBase;

    //�ļ������С
    m_nFileAlignment = m_pOptional->FileAlignment;

    //�ڴ�����С
    m_nSectionAlignment = m_pOptional->SectionAlignment;

    //��ȡ�ڱ�����
    m_nSectionTableNum = m_pFILE->NumberOfSections;
    return true;
}

bool CPEresolverDlg::InitTreeControl()
{
    //��������������ؽ��̵ľ��
    HTREEITEM  hProcess = m_treeFile.InsertItem(m_szProcessName);

    //����PE�ĸ��ṹ��
    //DOSͷ
    HTREEITEM  hDOS = m_treeFile.InsertItem("DOSͷ", NULL, NULL, hProcess);

    //DOS  _IMAGE_DOS_HEADER
    HTREEITEM  hDOSHeader = m_treeFile.InsertItem("_IMAGE_DOS_HEADER", NULL, NULL, hDOS);

    //PEͷ
    HTREEITEM  hPE = m_treeFile.InsertItem("PEͷ", NULL, NULL, hProcess);

    //NTͷ _IMAGE_NT_HEADERS
    HTREEITEM  hNtHead = m_treeFile.InsertItem("_IMAGE_NT_HEADERS", NULL, NULL, hPE);

    //PE��ʶ PE
    HTREEITEM  hSignature = m_treeFile.InsertItem("Signature", NULL, NULL, hNtHead);

    //PEͷ IMAGE_FILE_HEADER 
    HTREEITEM  hFileHead = m_treeFile.InsertItem("IMAGE_FILE_HEADER", NULL, NULL, hNtHead);

    //��ѡPEͷ IMAGE_OPTIONAL_HEADER32  
    HTREEITEM  hOptionalHead = m_treeFile.InsertItem("IMAGE_OPTIONAL_HEADER32", NULL, NULL, hNtHead);

    //�ڱ�
    HTREEITEM  hSection = m_treeFile.InsertItem("�ڱ�", NULL, NULL, hProcess);

    //��ַת��
    HTREEITEM  hAddrResolution = m_treeFile.InsertItem("��ַת��", NULL, NULL, hProcess);

    //����Ŀ¼
    HTREEITEM  hImprotDirect = m_treeFile.InsertItem("����Ŀ¼", NULL, NULL, hProcess);

    //����Ŀ¼
    HTREEITEM  hExprotDirect = m_treeFile.InsertItem("����Ŀ¼", NULL, NULL, hProcess);

    //�ض�λ��
    HTREEITEM  hRelocation = m_treeFile.InsertItem("�ض�λ��", NULL, NULL, hProcess);

    //TLs��
    HTREEITEM  hTLs = m_treeFile.InsertItem("TLs��", NULL, NULL, hProcess);

    //��Դ��
    HTREEITEM  hResource = m_treeFile.InsertItem("��Դ��", NULL, NULL, hProcess);

    return true;
}

void CPEresolverDlg::ClearListControl(CListCtrl& MyList)
{
    //ɾ��ԭ������
    int nCount = MyList.GetItemCount();
    for (int i = 0; i < nCount; i++)
    {
        MyList.DeleteItem(0);
    }

    //ɾ��ԭ����
    nCount = MyList.GetHeaderCtrl()->GetItemCount();
    for (int i = 0; i < nCount; i++)
    {
        MyList.DeleteColumn(0);
    }

    return;
}


void CPEresolverDlg::GetProcessName()
{
    int nSize = strlen(m_szPath);

    for (int i = nSize - 5; i >= 0; i--)
    {
        if (m_szPath[i] == '\\')
        {
            strcpy_s(m_szProcessName, FILENAMESIZE, &m_szPath[i + 1]);
            return;
        }
    }
}

void CPEresolverDlg::ShowDosHeader()
{
    m_nCurrentSelect = EN_DOS;
    //����list�ؼ�����
    //ɾ��ԭ����
    ClearListControl(m_lstData);

    //����±���
    m_lstData.InsertColumn(0, _T("�ں�"), LVCFMT_LEFT, m_rect.right * 2 / 5);
    m_lstData.InsertColumn(0, _T("ֵ"), LVCFMT_LEFT, m_rect.right / 5);
    m_lstData.InsertColumn(0, _T("��С"), LVCFMT_LEFT, m_rect.right / 5);
    m_lstData.InsertColumn(0, _T("ƫ��"), LVCFMT_LEFT, m_rect.right / 5);
    m_lstData.InsertColumn(0, _T("��Ա"), LVCFMT_LEFT, m_rect.right / 5);

    //���g_pOptional
    int nBegin = m_nOffsetDos;
    void* pBegin = (void*)m_pDOS;
    for (int i = 0; i < 31; i++)
    {
        //��ӳ�Ա
        m_lstData.InsertItem(i, g_pDosHeader[i].pInfo);

        //���ƫ��
        char szTemp[100] = { 0 };
        wsprintf(szTemp, "0X%p", nBegin);
        m_lstData.SetItemText(i, 1, szTemp);
        nBegin = nBegin + g_pDosHeader[i].nSize;

        //��Ӵ�С,ֵ
        if (g_pDosHeader[i].nSize == 2)
        {
            m_lstData.SetItemText(i, 2, "WORD");

            wsprintf(szTemp, "%04X", *(WORD*)pBegin);
            m_lstData.SetItemText(i, 3, szTemp);
            pBegin = (void*)((int)pBegin + 2);
        }
        else 
        {
            m_lstData.SetItemText(i, 2, "DWORD");

            wsprintf(szTemp, "%08X", *(DWORD*)pBegin);
            m_lstData.SetItemText(i, 3, szTemp);
            pBegin = (void*)((int)pBegin + 4);
        }
        
        //����ں�
        m_lstData.SetItemText(i, 4, g_pDosHeader[i].pDescription);
    }
}

void CPEresolverDlg::ShowPESign()//PE��ʶ
{
    m_nCurrentSelect = EN_NT;
    //����list�ؼ�����
    //ɾ��ԭ����
    ClearListControl(m_lstData);
    

    //����±���
    m_lstData.InsertColumn(0, _T("ֵ"), LVCFMT_LEFT, m_rect.right / 4);
    m_lstData.InsertColumn(0, _T("��С"), LVCFMT_LEFT, m_rect.right / 4);
    m_lstData.InsertColumn(0, _T("ƫ��"), LVCFMT_LEFT, m_rect.right / 4);
    m_lstData.InsertColumn(0, _T("��Ա"), LVCFMT_LEFT, m_rect.right / 4);

    //��ӳ�Ա
    m_lstData.InsertItem(0, "Signature");
    //���ƫ��
    char szTemp[16] = { 0 };
    wsprintf(szTemp, "0X%p", m_nOffsetNT);
    m_lstData.SetItemText(0, 1, szTemp);
    //��Ӵ�С
    m_lstData.SetItemText(0, 2, "DWORD");
    //���ֵ
    wsprintf(szTemp, "%08X", *(DWORD*)m_pNT);
    m_lstData.SetItemText(0, 3, szTemp);


}

void CPEresolverDlg::ShowFileHeader()//��׼PEͷ
{
    m_nCurrentSelect = EN_FILE;
    //����list�ؼ�����
    //ɾ��ԭ����
    ClearListControl(m_lstData);

    //����±���
    m_lstData.InsertColumn(0, _T("�ں�"), LVCFMT_LEFT, m_rect.right * 2);
    m_lstData.InsertColumn(0, _T("ֵ"), LVCFMT_LEFT, m_rect.right / 5);
    m_lstData.InsertColumn(0, _T("��С"), LVCFMT_LEFT, m_rect.right / 5);
    m_lstData.InsertColumn(0, _T("ƫ��"), LVCFMT_LEFT, m_rect.right / 5);
    m_lstData.InsertColumn(0, _T("��Ա"), LVCFMT_LEFT, m_rect.right / 5);

    //��ӳ�Ա
    m_lstData.InsertItem(0, "Machine");
    m_lstData.InsertItem(1, "NumberOfSections");
    m_lstData.InsertItem(2, "TimeDateStamp");
    m_lstData.InsertItem(3, "PointerToSymbolTable");
    m_lstData.InsertItem(4, "NumberOfSymbols");
    m_lstData.InsertItem(5, "SizeOfOptionalHeader");
    m_lstData.InsertItem(6, "Characteristics");

    //���ƫ�ƣ���С��ֵ
    int nNum = 0;
    int nOffset = m_nOffsetFile;
    void* pFile = (void*)m_pFILE;
    for (int i = 0; i < 7; i++)
    {
        char szTemp[16] = { 0 };
        wsprintf(szTemp, "0X%p", nOffset);
        m_lstData.SetItemText(i, 1, szTemp);

        if (i >= 2 && i <= 4)
        {
            //�����С
            m_lstData.SetItemText(i, 2, "DWORD");

            //����ֵ
            wsprintf(szTemp, "%08X", *(DWORD*)pFile);
            m_lstData.SetItemText(i, 3, szTemp);

            pFile = (void*)((int)pFile + 4);
            nOffset = nOffset + 4;
        }
        else
        {
            //�����С
            m_lstData.SetItemText(i, 2, "WORD");

            //����ֵ
            wsprintf(szTemp, "%04X", *(WORD*)pFile);
            m_lstData.SetItemText(i, 3, szTemp);

            pFile = (void*)((int)pFile + 2);
            nOffset = nOffset + 2;
        }
    }

    //�����ں�
    //��������ͺ�
    pFile = (void*)m_pFILE;
    int nValue = 0;
    //member--1
    nValue = *(WORD*)pFile;
    pFile = (void*)((int)pFile + 2);
    for (int i = 0; i < 30; i++)
    {
        if (g_pMachine[i].wData == nValue)
        {
            m_lstData.SetItemText(0, 4, g_pMachine[i].pInfo);
            break;
        }
    }
    //����ڵ�����
    m_lstData.SetItemText(1, 4, "�ڵ�����");
    //�������ʱ��
    pFile = (void*)((int)m_pFILE + 4);
    tm tFile = { 0 };
    time_t time = (time_t)*(DWORD*)pFile;
    gmtime_s(&tFile, &time);
    char szBuffTime[100] = { 0 };
    wsprintf(szBuffTime, "%d-%d-%d-%d:%d:%d", 
        tFile.tm_year + 1900, 
        tFile.tm_mon + 1,
        tFile.tm_mday,
        tFile.tm_hour,
        tFile.tm_min,
        tFile.tm_sec
        );
    m_lstData.SetItemText(2, 4, szBuffTime);
    //���� ��չPEͷ��С
    m_lstData.SetItemText(5, 4, "��չPEͷ��С");
    //�����ļ�����
    pFile = (void*)((int)m_pFILE + 18);
    nValue = *(WORD*)pFile;
    char szCharactor[0X1000] = { 0 };
    for (int i = 0; i < 15; i++)
    {
        int nCharactor = g_pCharact[i].wData & nValue;
        if (nCharactor != 0)
        {
            strcat_s(szCharactor, 0X1000, g_pCharact[i].pInfo);
            strcat_s(szCharactor, 0X1000, "  ");
        }
    }
    m_lstData.SetItemText(6, 4, szCharactor);
}

void CPEresolverDlg::ShowOptionalHeader()
{
    m_nCurrentSelect = EN_OPTIONAL;
    //����list�ؼ�����
    //ɾ��ԭ����
    ClearListControl(m_lstData);

    //����±���
    m_lstData.InsertColumn(0, _T("�ں�"), LVCFMT_LEFT, m_rect.right * 2 / 5);
    m_lstData.InsertColumn(0, _T("ֵ"), LVCFMT_LEFT, m_rect.right / 5);
    m_lstData.InsertColumn(0, _T("��С"), LVCFMT_LEFT, m_rect.right / 5);
    m_lstData.InsertColumn(0, _T("ƫ��"), LVCFMT_LEFT, m_rect.right / 5);
    m_lstData.InsertColumn(0, _T("��Ա"), LVCFMT_LEFT, m_rect.right / 5);

    //���g_pOptional
    int nBegin = m_nOffsetOptional;
    void* pBegin = (void*)m_pOptional;
    for (int i = 0; i < 30 + m_pOptional->NumberOfRvaAndSizes; i++)
    {
        //��ӳ�Ա
        m_lstData.InsertItem(i, g_pOptional[i].pInfo);

        //���ƫ��
        char szTemp[36] = { 0 };
        wsprintf(szTemp, "0X%p", nBegin);
        m_lstData.SetItemText(i, 1, szTemp);
        nBegin = nBegin + g_pOptional[i].nSize;

        //��Ӵ�С,ֵ
        if (g_pOptional[i].nSize == 1)
        {
            //��Ӵ�С
            m_lstData.SetItemText(i, 2, "BYTE");

            //���ֵ
            wsprintf(szTemp, "%02X", *(BYTE*)pBegin);
            m_lstData.SetItemText(i, 3, szTemp);
            pBegin = (void*)((int)pBegin + 1);
        }
        else if (g_pOptional[i].nSize == 2)
        {
            m_lstData.SetItemText(i, 2, "WORD");

            wsprintf(szTemp, "%04X", *(WORD*)pBegin);
            m_lstData.SetItemText(i, 3, szTemp);
            pBegin = (void*)((int)pBegin + 2);
        }
        else if (g_pOptional[i].nSize == 4)
        {
            m_lstData.SetItemText(i, 2, "DWORD");

            wsprintf(szTemp, "%08X", *(DWORD*)pBegin);
            m_lstData.SetItemText(i, 3, szTemp);
            pBegin = (void*)((int)pBegin + 4);
        }
        else
        {
            m_lstData.SetItemText(i, 2, "IMAGE_DATA_DIRECTORY ");

            wsprintf(szTemp, "%08X %08X", *(DWORD*)pBegin, *(DWORD*)((int)pBegin + 4));
            m_lstData.SetItemText(i, 3, szTemp);
            pBegin = (void*)((int)pBegin + 8);
        }
    }

    //����ں�
    m_lstData.SetItemText(6, 4, "������ڵ�");
    m_lstData.SetItemText(9, 4, "�ڴ澵���ַ");
    m_lstData.SetItemText(10, 4, "�ڴ����");
    m_lstData.SetItemText(11, 4, "�ļ�����");
    m_lstData.SetItemText(19, 4, "�ڴ�������PE�ļ���ӳ��ߴ�");
    m_lstData.SetItemText(20, 4, "ͷ+�ڱ�����Ĵ�С");
    m_lstData.SetItemText(21, 4, "У���");
    m_lstData.SetItemText(23, 4, "�ļ�����");
    m_lstData.SetItemText(24, 4, "ջ������С");
    m_lstData.SetItemText(25, 4, "ջ�ύ��С");
    m_lstData.SetItemText(26, 4, "�ѱ�����С");
    m_lstData.SetItemText(27, 4, "���ύ��С");
    m_lstData.SetItemText(29, 4, "Ŀ¼����Ŀ");

    m_lstData.SetItemText(30, 4, "Export Directory");
    m_lstData.SetItemText(31, 4, "Import Directory");
    m_lstData.SetItemText(32, 4, "Resource Directory");
    m_lstData.SetItemText(35, 4, "Base Relocation Table");
    m_lstData.SetItemText(39, 4, "TLS Directory");
    m_lstData.SetItemText(42, 4, "Import Address Table");
}

void CPEresolverDlg::ShowSectionTable()
{
    if (m_nSectionTableNum == 0)
    {
        AfxMessageBox("��ǰ�ļ�û�нڱ�");
        m_nCurrentSelect = EN_NONE;
        return;
    }

    m_nCurrentSelect = EN_SECTION;
    //����list�ؼ�����
    //ɾ��ԭ����
    ClearListControl(m_lstData);

    //����±���
    m_lstData.InsertColumn(0, _T("�ں�"), LVCFMT_LEFT, m_rect.right  / 3);
    m_lstData.InsertColumn(0, _T("ֵ"), LVCFMT_LEFT, m_rect.right / 5);
    m_lstData.InsertColumn(0, _T("��С"), LVCFMT_LEFT, m_rect.right / 9);
    m_lstData.InsertColumn(0, _T("ƫ��"), LVCFMT_LEFT, m_rect.right / 6);
    m_lstData.InsertColumn(0, _T("��Ա"), LVCFMT_LEFT, m_rect.right / 4);
    m_lstData.InsertColumn(0, _T("����"), LVCFMT_LEFT, m_rect.right / 10);
    //�������
    //g_pSectionTable
    void* pAddr = (void*)m_pSection;
    int nBegin = m_nOffsetSection;
    int nRow = 0;
    for (int i = 0; i < m_nSectionTableNum; i++)
    {
        for (int j = 0; j < 10; j++)
        {
            //�������
            if (j == 0)
            {
                m_lstData.InsertItem(nRow, (char*)m_pSection[i].Name);
            }
            else
            {
                m_lstData.InsertItem(nRow, "");
            }
            

            //��ӳ�Ա
            m_lstData.SetItemText(nRow, 1, g_pSectionTable[j].pInfo);

            //���ƫ��
            char szTemp[36] = { 0 };
            wsprintf(szTemp, "0X%p", nBegin);
            m_lstData.SetItemText(nRow, 2, szTemp);
            nBegin = nBegin + g_pSectionTable[j].nSize;

            //��Ӵ�С
            if (g_pSectionTable[j].nSize == 2)
            {
                m_lstData.SetItemText(nRow, 3, "WORD");

                //���ֵ
                wsprintf(szTemp, "%04X", *(WORD*)pAddr);
                m_lstData.SetItemText(nRow, 4, szTemp);
                pAddr = (void*)((int)pAddr + 2);
            }
            else if (g_pSectionTable[j].nSize == 4)
            {
                m_lstData.SetItemText(nRow, 3, "DWORD");

                //���ֵ
                wsprintf(szTemp, "%08X", *(DWORD*)pAddr);
                m_lstData.SetItemText(nRow, 4, szTemp);
                pAddr = (void*)((int)pAddr + 4);
            }
            else
            {
                m_lstData.SetItemText(nRow, 3, "IMAGE_SIZEOF_SHORT_NAME");

                //���ֵ
                wsprintf(szTemp, "%02X%02X%02X%02X%02X%02X%02X%02X",
                    *(BYTE*)pAddr, 
                    *(BYTE*)((int)pAddr + 1),
                    *(BYTE*)((int)pAddr + 2),
                    *(BYTE*)((int)pAddr + 3),
                    *(BYTE*)((int)pAddr + 4),
                    *(BYTE*)((int)pAddr + 5),
                    *(BYTE*)((int)pAddr + 6),
                    *(BYTE*)((int)pAddr + 7));

                m_lstData.SetItemText(nRow, 4, szTemp);
                pAddr = (void*)((int)pAddr + 8);
            }

            //����ں�
            if (j == 0)
            {
                m_lstData.SetItemText(nRow, 5, "������");
            }
            else if (j == 1)
            {
                m_lstData.SetItemText(nRow, 5, "�����С");
            }
            else if (j == 2)
            {
                m_lstData.SetItemText(nRow, 5, "����ƫ�Ƶ�ַ");
            }
            else if (j == 3)
            {
                m_lstData.SetItemText(nRow, 5, "Raw�ߴ�");
            }
            else if (j == 4)
            {
                m_lstData.SetItemText(nRow, 5, "Raw��ַ");
            }
            else if (j == 9)
            {
                char szCharact[0X1000] = { "�ڵ����ԣ�" };

                int nCharact = m_pSection[i].Characteristics & 0x10000000;
                if (nCharact != 0)
                {
                    strcat_s(szCharact, 0X1000, "����");
                    strcat_s(szCharact, 0X1000, "  ");
                }

                nCharact = m_pSection[i].Characteristics & 0x20000000;
                if (nCharact != 0)
                {
                    strcat_s(szCharact, 0X1000, "��ִ��");
                    strcat_s(szCharact, 0X1000, "  ");
                }

                nCharact = m_pSection[i].Characteristics & 0x40000000;
                if (nCharact != 0)
                {
                    strcat_s(szCharact, 0X1000, "�ɶ�");
                    strcat_s(szCharact, 0X1000, "  ");
                }

                nCharact = m_pSection[i].Characteristics & 0x80000000;
                if (nCharact != 0)
                {
                    strcat_s(szCharact, 0X1000, "��д");
                    strcat_s(szCharact, 0X1000, "  ");
                }
                m_lstData.SetItemText(nRow, 5, szCharact);
            }
            else
            {
                m_lstData.SetItemText(nRow, 5, " ");
            }
            nRow++;
        }
    }

}

//void CPEresolverDlg::ShowAddrChange()
//{
//    //�����б�ؼ�
//    m_lstData.ShowWindow(SW_HIDE);
//
//
//
//
//
//
//}

void CPEresolverDlg::ShowImport()
{
    //�ж�
    if (m_nOffsetImport == 0 )
    {
        AfxMessageBox("���ļ�û�е����");
        m_nCurrentSelect = EN_NONE;
        return;
    }

    m_nCurrentSelect = EN_IMPORTDLL;
    //����list�ؼ�����
    //ɾ��ԭ����
    ClearListControl(m_lstImportDLL);

    //����±���
    m_lstImportDLL.InsertColumn(0, _T("ģ����"), LVCFMT_LEFT, m_rectImportDll.right / 4);
    m_lstImportDLL.InsertColumn(1, _T("����RVA"), LVCFMT_LEFT, m_rectImportDll.right / 8);
    m_lstImportDLL.InsertColumn(2, _T("����"), LVCFMT_LEFT, m_rectImportDll.right / 8);
    m_lstImportDLL.InsertColumn(3, _T("INT(RVA)"), LVCFMT_LEFT, m_rectImportDll.right / 8);
    m_lstImportDLL.InsertColumn(4, _T("ʱ�����ڴ�"), LVCFMT_LEFT, m_rectImportDll.right / 8);
    m_lstImportDLL.InsertColumn(5, _T("ת����"), LVCFMT_LEFT, m_rectImportDll.right / 8);
    m_lstImportDLL.InsertColumn(6, _T("IAT(RVA)"), LVCFMT_LEFT, m_rectImportDll.right / 8);

    //����ߴ�
    m_lstImportDLL.InsertItem(0, "char*");
    m_lstImportDLL.SetItemText(0, 1, "DWORD");
    m_lstImportDLL.SetItemText(0, 2, "DWORD");
    m_lstImportDLL.SetItemText(0, 3, "DWORD");
    m_lstImportDLL.SetItemText(0, 4, "DWORD");
    m_lstImportDLL.SetItemText(0, 5, "DWORD");
    m_lstImportDLL.SetItemText(0, 6, "DWORD");

    //�������
    IMAGE_IMPORT_DESCRIPTOR strCmp = { 0 };
    char szTemp[100] = { 0 };
    int nNum = 0;
    while (true)
    {
        //����DLL��
        char* pDllName = (char*)RVAtoMapAddr(m_pImport[nNum].Name);
        m_lstImportDLL.InsertItem(nNum + 1, pDllName);
        //����DLL��RVA
        wsprintf(szTemp, "%08X", m_pImport[nNum].Name);
        m_lstImportDLL.SetItemText(nNum + 1, 1, szTemp);

        //���뵼����Ŀ
        //1. �ҵ�IAT���ڴ�ӳ���ַ
        DWORD dwIATRVA = m_pImport[nNum].FirstThunk;
        //DWORD dwIATOffset = RVAtoOFFSET(dwIATRVA);
        DWORD* dwpIATMemoryAddr = (DWORD*)RVAtoMapAddr(dwIATRVA);
        int nCount = 0;
        while (true)
        {
            if (*dwpIATMemoryAddr == 0)
            {
                break;
            }
            nCount++;
            dwpIATMemoryAddr++;
        }
        //���IAT
        wsprintf(szTemp, "%08X", dwIATRVA);
        m_lstImportDLL.SetItemText(nNum + 1, 6, szTemp);
        //��ӵ�����Ŀ
        wsprintf(szTemp, "%d", nCount);
        m_lstImportDLL.SetItemText(nNum + 1, 2, szTemp);
        //����INT
        wsprintf(szTemp, "%08X", m_pImport[nNum].OriginalFirstThunk);
        m_lstImportDLL.SetItemText(nNum + 1, 3, szTemp);
        //����ʱ���
        wsprintf(szTemp, "%08X", m_pImport[nNum].TimeDateStamp);
        m_lstImportDLL.SetItemText(nNum + 1, 4, szTemp);
        //����ת����
        wsprintf(szTemp, "%08X", m_pImport[nNum].ForwarderChain);
        m_lstImportDLL.SetItemText(nNum + 1, 5, szTemp);

        if (memcmp((const void*)&m_pImport[nNum + 1], (const void*)&strCmp, sizeof(strCmp)) == 0)
        {
            break;
        }
        nNum++;
    }
}

void CPEresolverDlg::ShowImportFunc()
{
    //����list�ؼ�����
    //ɾ��ԭ����
    ClearListControl(m_lstImportFunc);

    //����±���
    m_lstImportFunc.InsertColumn(0, _T("BY_NAME(offset)"), LVCFMT_LEFT, m_rectImportDll.right / 5);
    m_lstImportFunc.InsertColumn(1, _T("������"), LVCFMT_LEFT, m_rectImportDll.right / 5);
    m_lstImportFunc.InsertColumn(2, _T("Hint"), LVCFMT_LEFT, m_rectImportDll.right / 5);
    m_lstImportFunc.InsertColumn(3, _T("IAT(offset)"), LVCFMT_LEFT, m_rectImportDll.right / 5);
    m_lstImportFunc.InsertColumn(4, _T("IAT"), LVCFMT_LEFT, m_rectImportDll.right / 5);

    //��ȡ�������Ϣ

    //��ȡINT��
    CString cs;
    cs = m_lstImportDLL.GetItemText(m_nRow, 3);
    DWORD nRVAINT = strtoul(cs.GetBuffer(), NULL, 16);
    //ת��
    DWORD nOffsetINT = RVAtoOFFSET(nRVAINT);
    DWORD* pMapINT = (DWORD*)RVAtoMapAddr(nRVAINT);

    //��ȡIAT��
    cs = m_lstImportDLL.GetItemText(m_nRow, 6);
    DWORD nRVAIAT = strtoul(cs.GetBuffer(), NULL, 16);
    //ת��
    DWORD nOffsetIAT = RVAtoOFFSET(nRVAIAT);
    DWORD* pMapIAT = (DWORD*)RVAtoMapAddr(nRVAIAT);

    int nNum = 0;
    char szTemp[100] = { 0 };
    while (true)
    {
        if (*pMapINT == 0 && *pMapIAT == 0)
        {
            break;
        }

        if (*pMapINT > 0x80000000)
        {
            //���
            WORD* pData = (WORD*)pMapINT;
            pData++;

        }
        else
        {
            if (*pMapINT != 0)
            {
                //����RVA
                DWORD dwBYNAMEOffset = RVAtoOFFSET(*pMapINT);

                void* pBYNAME = RVAtoMapAddr(*pMapINT);

                //����INTƫ��
                wsprintf(szTemp, "0X%08X", dwBYNAMEOffset);
                m_lstImportFunc.InsertItem(nNum, szTemp);

                //���뺯����
                m_lstImportFunc.SetItemText(nNum, 1, (char*)((int)pBYNAME + 2));

                //����Hint
                wsprintf(szTemp, "%04X", *(WORD*)pBYNAME);
                m_lstImportFunc.SetItemText(nNum, 2, szTemp);

                //����IAT(offset)
                //DWORD dwIATOffset = RVAtoOFFSET(*pMapIAT);
                wsprintf(szTemp, "0X%08X", nOffsetIAT);
                m_lstImportFunc.SetItemText(nNum, 3, szTemp);

                //����IAT
                wsprintf(szTemp, "%08X", *pMapIAT);
                m_lstImportFunc.SetItemText(nNum, 4, szTemp);
            }
            else//INTΪ0����ȡIAT��Ϣ
            {
                //����RVA
                DWORD dwBYNAMEOffset = RVAtoOFFSET(*pMapIAT);

                void* pBYNAME = RVAtoMapAddr(*pMapIAT);

                //����INTƫ��
                wsprintf(szTemp, "0X%08X", dwBYNAMEOffset);
                m_lstImportFunc.InsertItem(nNum, szTemp);

                //���뺯����
                m_lstImportFunc.SetItemText(nNum, 1, (char*)((int)pBYNAME + 2));

                //����Hint
                wsprintf(szTemp, "%04X", *(WORD*)pBYNAME);
                m_lstImportFunc.SetItemText(nNum, 2, szTemp);

                //����IAT(offset)
                //DWORD dwIATOffset = RVAtoOFFSET(*pMapIAT);
                wsprintf(szTemp, "0X%08X", nOffsetIAT);
                m_lstImportFunc.SetItemText(nNum, 3, szTemp);

                //����IAT
                wsprintf(szTemp, "%08X", *pMapIAT);
                m_lstImportFunc.SetItemText(nNum, 4, szTemp);
            }
        }
        nOffsetIAT++;
        pMapIAT++;
        pMapINT++;
        nNum++;
    }

    m_lstImportFunc.ShowWindow(SW_SHOW);
}

void CPEresolverDlg::ShowExport()
{
    //�ж�
    if (m_nOffsetExport == 0)
    {
        AfxMessageBox("���ļ�û�е�����");
        m_nCurrentSelect = EN_NONE;
        return;
    }

    m_nCurrentSelect = EN_EXPORTDLL;
    //����list�ؼ�����
    //ɾ��ԭ����
    ClearListControl(m_lstExportDll);

    //����±���
    m_lstExportDll.InsertColumn(0, _T("��Ա"), LVCFMT_LEFT, m_rectExportDll.right / 5);
    m_lstExportDll.InsertColumn(1, _T("�ļ�ƫ��"), LVCFMT_LEFT, m_rectExportDll.right / 5);
    m_lstExportDll.InsertColumn(2, _T("��С"), LVCFMT_LEFT, m_rectExportDll.right / 5);
    m_lstExportDll.InsertColumn(3, _T("ֵ"), LVCFMT_LEFT, m_rectExportDll.right / 5);
    m_lstExportDll.InsertColumn(4, _T("�ں�"), LVCFMT_LEFT, m_rectExportDll.right / 5);

    int nOffset = m_nOffsetExport;
    char szTemp[100] = { 0 };
    void* pAddr = (void*)m_pExport;
    for (int i = 0; i < 11; i++)
    {
        //�����Ա
        m_lstExportDll.InsertItem(i, g_pExportTable[i].pInfo);

        //�����ļ�ƫ��
        wsprintf(szTemp, "0X%08X", nOffset);
        m_lstExportDll.SetItemText(i, 1, szTemp);
        nOffset += g_pExportTable[i].nSize;

        //�����С
        //����ֵ
        if (g_pExportTable[i].nSize == 2)
        {
            m_lstExportDll.SetItemText(i, 2, "WORD");

            wsprintf(szTemp, "%04X", *(WORD*)pAddr);
            m_lstExportDll.SetItemText(i, 3, szTemp);
        }
        else
        {
            m_lstExportDll.SetItemText(i, 2, "DWORD");

            wsprintf(szTemp, "%08X", *(DWORD*)pAddr);
            m_lstExportDll.SetItemText(i, 3, szTemp);
        }

        //��������
        if (i == 1)
        {
            //�������ʱ��
            tm tFile = { 0 };
            time_t time = (time_t)*(DWORD*)pAddr;
            gmtime_s(&tFile, &time);
            char szBuffTime[100] = { 0 };
            wsprintf(szBuffTime, "%d-%d-%d-%d:%d:%d",
                tFile.tm_year + 1900,
                tFile.tm_mon + 1,
                tFile.tm_mday,
                tFile.tm_hour,
                tFile.tm_min,
                tFile.tm_sec
            );
            m_lstExportDll.SetItemText(i, 4, szBuffTime);
        }
        else
        {
            m_lstExportDll.SetItemText(i, 4, g_pExportTable[i].pDescription);
        }
        pAddr = (void*)((int)pAddr + g_pExportTable[i].nSize);
    }

    m_dwNumOfFunctions = m_pExport->NumberOfFunctions;
    m_dwNumOfNames = m_pExport->NumberOfNames;
    m_dwAddrOfFunction = m_pExport->AddressOfFunctions;
    m_dwAddrOfNames = m_pExport->AddressOfNames;
    m_dwAddrOfOrdinal = m_pExport->AddressOfNameOrdinals;
    m_dwBase = m_pExport->Base;

    ShowExportFunc();
}

void CPEresolverDlg::ShowExportFunc()
{
    ClearListControl(m_lstExportFunc);

    DWORD* pAddrOfFunc = (DWORD*)RVAtoMapAddr(m_dwAddrOfFunction);
    WORD* pAddrOfOrdinal = (WORD*)RVAtoMapAddr(m_dwAddrOfOrdinal);
    DWORD* pAddrOfNames = (DWORD*)RVAtoMapAddr(m_dwAddrOfNames);

    //����±���
    m_lstExportFunc.InsertColumn(0, _T("�������"), LVCFMT_LEFT, m_rectExportFunc.right / 5);
    m_lstExportFunc.InsertColumn(1, _T("������ַ(offset)"), LVCFMT_LEFT, m_rectExportFunc.right / 5);
    m_lstExportFunc.InsertColumn(2, _T("�������"), LVCFMT_LEFT, m_rectExportFunc.right / 5);
    m_lstExportFunc.InsertColumn(3, _T("��������(offset)"), LVCFMT_LEFT, m_rectExportFunc.right / 5);
    m_lstExportFunc.InsertColumn(4, _T("������"), LVCFMT_LEFT, m_rectExportFunc.right / 5);

    //��Ϊ��ַ���п����п����������������һ������������i
    int nIndex = 0;
    char szTemp[100] = { 0 };
    for (int i = 0; i < m_dwNumOfFunctions; i++)
    {
        if (pAddrOfFunc[i] == 0)//��ǰ��Ϊ����
        {
            continue;
        }

        //���뵼�����
        wsprintf(szTemp, "%08X", (DWORD)(nIndex + m_dwBase));
        m_lstExportFunc.InsertItem(nIndex, szTemp);

        //���뺯����ַ(offset)
        wsprintf(szTemp, "%08X", RVAtoOFFSET(pAddrOfFunc[i]));
        m_lstExportFunc.SetItemText(nIndex, 1, szTemp);

        //�����������
        bool bFlag = false;
        for (int j = 0; j < m_dwNumOfNames; j++)
        {
            if (pAddrOfOrdinal[j] == i)
            {
                bFlag = true;

                //�����������
                wsprintf(szTemp, "%08X", j);
                m_lstExportFunc.SetItemText(nIndex, 2, szTemp);

                //���뺯������(offset)
                wsprintf(szTemp, "%08X", RVAtoOFFSET(pAddrOfNames[j]));
                m_lstExportFunc.SetItemText(nIndex, 3, szTemp);

                //���뺯����
                m_lstExportFunc.SetItemText(nIndex, 4, (LPCTSTR)RVAtoMapAddr(pAddrOfNames[j]));
                break;
            }
        }
        if (!bFlag)
        {
            //�����������
            m_lstExportFunc.SetItemText(nIndex, 2, "N/A");

            //���뺯������(offset)
            m_lstExportFunc.SetItemText(nIndex, 3, "N/A");

            //���뺯����
            m_lstExportFunc.SetItemText(nIndex, 4, "N/A");
        }

        nIndex++;
    }

}

void CPEresolverDlg::ShowRelocation()
{
    //�ж�
    if (m_nOffsetRelocation == 0)
    {
        AfxMessageBox("���ļ�û���ض�λ��");
        m_nCurrentSelect = EN_NONE;
        return;
    }

    m_nCurrentSelect = EN_RELOCATION;
    //����list�ؼ�����
    //ɾ��ԭ����
    ClearListControl(m_lstData);

    //����±���
    m_lstData.InsertColumn(0, _T("�ļ�ƫ��"), LVCFMT_LEFT, m_rectExportDll.right / 3);
    m_lstData.InsertColumn(1, _T("��ҳ"), LVCFMT_LEFT, m_rectExportDll.right / 3);
    m_lstData.InsertColumn(2, _T("��ַ"), LVCFMT_LEFT, m_rectExportDll.right / 3);

    //��������
    char szTemp[100] = { 0 };
    int nNum = 0;
    int nCurrentSize = 0;//��ǰ������ɵ��ض�λ��ߴ�
    IMAGE_BASE_RELOCATION* pCurrentReloc = m_pRelocation;
    WORD* pCurrentOffset = (WORD*)((int)pCurrentReloc + sizeof(IMAGE_BASE_RELOCATION));
    int nOffset = m_nOffsetRelocation;
    while (nCurrentSize < m_nSizeOfRelocation)
    {
        int nSize = pCurrentReloc->SizeOfBlock;//��ǰ�����ķ�ҳ���ض�λ��ߴ�
        int nCount = (nSize - sizeof(IMAGE_BASE_RELOCATION)) / 2;//��ǰ����ض�λ����
        nOffset = nOffset + sizeof(IMAGE_BASE_RELOCATION);

        for (int i = 0; i < nCount; i++)
        {
            //�����ļ�ƫ��
            wsprintf(szTemp, "0X%08X", nOffset);
            m_lstData.InsertItem(nNum, szTemp);
            
            //�����ҳ
            wsprintf(szTemp, "0X%08X", pCurrentReloc->VirtualAddress);
            m_lstData.SetItemText(nNum, 1, szTemp);

            //�����ַ
            int nTemp = 0Xfff;
            wsprintf(szTemp, "0X%08X", (pCurrentOffset[i] & nTemp));
            m_lstData.SetItemText(nNum, 2, szTemp);

            nOffset += 2;
            nNum++;
        }

        nCurrentSize += nSize;
        pCurrentReloc = (IMAGE_BASE_RELOCATION*)((int)pCurrentReloc + nSize);
        pCurrentOffset = (WORD*)((int)pCurrentReloc + sizeof(IMAGE_BASE_RELOCATION));
    }
}

void CPEresolverDlg::ShowTLs()
{
    //�ж�
    if (m_nOffsetTls == 0)
    {
        AfxMessageBox("���ļ�û��TLS��");
        m_nCurrentSelect = EN_NONE;
        return;
    }

    m_nCurrentSelect = EN_TLS;
    //����list�ؼ�����
    //ɾ��ԭ����
    ClearListControl(m_lstData);

    //����±���
    m_lstData.InsertColumn(0, _T("��Ա"), LVCFMT_LEFT, m_rect.right / 5);
    m_lstData.InsertColumn(1, _T("ƫ��"), LVCFMT_LEFT, m_rect.right / 5);
    m_lstData.InsertColumn(2, _T("��С"), LVCFMT_LEFT, m_rect.right / 5);
    m_lstData.InsertColumn(3, _T("ֵ��VA��"), LVCFMT_LEFT, m_rect.right / 5);
    m_lstData.InsertColumn(4, _T("�ں�"), LVCFMT_LEFT, m_rect.right  / 5);

    //�����
    //��Ա
    m_lstData.InsertItem(0, "StartAddressOfRawData");
    m_lstData.InsertItem(1, "EndAddressOfRawData");
    m_lstData.InsertItem(2, "AddressOfIndex");
    m_lstData.InsertItem(3, "AddressOfCallBacks");
    m_lstData.InsertItem(4, "SizeOfZeroFill");
    m_lstData.InsertItem(5, "DUMMYUNIONNAME");
    
    //ƫ��
    char szTemp[100] = { 0 };
    wsprintf(szTemp, "0X%08X", m_nOffsetTls);
    m_lstData.SetItemText(0, 1, szTemp);
    m_nOffsetTls += 4;
    wsprintf(szTemp, "0X%08X", m_nOffsetTls);
    m_lstData.SetItemText(1, 1, szTemp);
    m_nOffsetTls += 4;
    wsprintf(szTemp, "0X%08X", m_nOffsetTls);
    m_lstData.SetItemText(2, 1, szTemp);
    m_nOffsetTls += 4;
    wsprintf(szTemp, "0X%08X", m_nOffsetTls);
    m_lstData.SetItemText(3, 1, szTemp);
    m_nOffsetTls += 4;
    wsprintf(szTemp, "0X%08X", m_nOffsetTls);
    m_lstData.SetItemText(4, 1, szTemp);
    m_nOffsetTls += 4;
    wsprintf(szTemp, "0X%08X", m_nOffsetTls);
    m_lstData.SetItemText(5, 1, szTemp);

    //��С
    m_lstData.SetItemText(0, 2, "DWORD");
    m_lstData.SetItemText(1, 2, "DWORD");
    m_lstData.SetItemText(2, 2, "DWORD");
    m_lstData.SetItemText(3, 2, "DWORD");
    m_lstData.SetItemText(4, 2, "DWORD");
    m_lstData.SetItemText(5, 2, "DWORD");

    //ֵ
    wsprintf(szTemp, "0X%08X", m_pTLs->StartAddressOfRawData);
    m_lstData.SetItemText(0, 3, szTemp);
    wsprintf(szTemp, "0X%08X", m_pTLs->EndAddressOfRawData);
    m_lstData.SetItemText(1, 3, szTemp);
    wsprintf(szTemp, "0X%08X", m_pTLs->AddressOfIndex);
    m_lstData.SetItemText(2, 3, szTemp);
    wsprintf(szTemp, "0X%08X", m_pTLs->AddressOfCallBacks);
    m_lstData.SetItemText(3, 3, szTemp);
    wsprintf(szTemp, "0X%08X", m_pTLs->SizeOfZeroFill);
    m_lstData.SetItemText(4, 3, szTemp);
    wsprintf(szTemp, "0X%08X", m_pTLs->Characteristics);
    m_lstData.SetItemText(5, 3, szTemp);

    //�ں�
    m_lstData.SetItemText(0, 4, "TLs���ڴ��е���ʼVA");
    m_lstData.SetItemText(1, 4, "TLs���ڴ��еĽ���VA");
    m_lstData.SetItemText(2, 4, "�洢TLS������λ��VA");
    m_lstData.SetItemText(3, 4, "�ص�����ָ������VA");

}

void CPEresolverDlg::ShowResource()
{
    //�ж�
    if (m_nOffsetResource == 0)
    {
        AfxMessageBox("���ļ�û����Դ��");
        m_nCurrentSelect = EN_NONE;
        return;
    }

    //�������ؼ�
    m_treeResource.DeleteAllItems();

    m_nCurrentSelect = EN_RESOURCE;
    //����list�ؼ�����
    //ɾ��ԭ����
    //ClearListControl(m_lstResource);

    DWORD dwFirst = m_nOffsetResource;
    ReadResource(NULL, dwFirst, EN_DERECTY, 0);
}

//���λΪ1-����Դ���ݡ� ���λΪ2-����ԴĿ¼� ���λΪ4-����ԴĿ¼---�Դ�Ϊ��׼����ַת����Ž�InsertItem�Ĳ�����
void CPEresolverDlg::ReadResource(HTREEITEM hParent, DWORD dwNext, int nType, int nNum)
{
    //�����ļ�ƫ��
    DWORD dwOffset = dwNext & 0X0fffffff;

    //�����ڴ�ƫ��
    DWORD dwMemAddr = dwOffset + (DWORD)m_pView;

    switch (nType)
    {
    case EN_DERECTY://��ԴĿ¼
    {
        LPARAM lParam = dwMemAddr | 0X40000000;
        UINT uMask = TVIF_PARAM | TVIF_TEXT;
        HTREEITEM  hResorce = m_treeResource.InsertItem(uMask, "��ԴĿ¼", NULL, NULL, NULL, NULL, lParam, hParent, TVI_LAST);

        IMAGE_RESOURCE_DIRECTORY* pRes = (IMAGE_RESOURCE_DIRECTORY*)dwMemAddr;
        int nCount = pRes->NumberOfIdEntries + pRes->NumberOfNamedEntries;

        for (int i = 0; i < nCount; i++)
        {
            DWORD dwNextOffset = dwNext + sizeof(IMAGE_RESOURCE_DIRECTORY) + i * sizeof(IMAGE_RESOURCE_DIRECTORY_ENTRY);
            ReadResource(hResorce, dwNextOffset, EN_ENTRY,i + 1);
        }

        break;
    }
    case EN_ENTRY://��ԴĿ¼��
    {
        IMAGE_RESOURCE_DIRECTORY_ENTRY* pRes = (IMAGE_RESOURCE_DIRECTORY_ENTRY*)dwMemAddr;
        DWORD dwName = pRes->Name;
        DWORD dwOffsetToData = pRes->OffsetToData;

        //��Ʊ���
        char szTitle[100] = { 0 };
        if (dwName >= 0X80000000)//name
        {
            //����name���ڴ��ַ
            dwName = dwName & 0X0fffffff;
            dwName += m_nOffsetResource;
            dwName += (DWORD)m_pView;

            IMAGE_RESOURCE_DIR_STRING_U* pResU = (IMAGE_RESOURCE_DIR_STRING_U*)dwName;
            WORD wSize = pResU->Length;
            char* pName = (char*)pResU->NameString;

            char szName[100] = { 0 };
            for (int i = 0; i < wSize; i++)
            {
                szName[i] = pName[2 * i];
            }

            wsprintf(szTitle, "��ԴĿ¼�� %d, ���� ", nNum);
            strcat_s(szTitle, 100, szName);
        }
        else//ID
        {
            wsprintf(szTitle, "��ԴĿ¼�� %d, ID %d ", nNum, pRes->Id);
        }

        LPARAM lpParam = dwMemAddr | 0X20000000;
        UINT uMask = TVIF_PARAM | TVIF_TEXT;
        HTREEITEM  hResorce = m_treeResource.InsertItem(uMask, szTitle, NULL, NULL, NULL, NULL, lpParam, hParent, TVI_LAST);

        if (dwOffsetToData >= 0X80000000)//��ԴĿ¼
        {
            dwOffsetToData = dwOffsetToData & 0X0fffffff;
            dwOffsetToData += m_nOffsetResource;

            ReadResource(hResorce, dwOffsetToData, EN_DERECTY, 0);

        }
        else//����
        {
            dwOffsetToData += m_nOffsetResource;
            ReadResource(hResorce, dwOffsetToData, EN_DATA, 0);
        }

        break;
    }
    case EN_DATA://����
    {
        LPARAM lpParam = dwMemAddr | 0X10000000;
        UINT uMask = TVIF_PARAM | TVIF_TEXT;
        HTREEITEM  hData = m_treeResource.InsertItem(uMask, "��Դ������", NULL, NULL, NULL, NULL, lpParam, hParent, TVI_LAST);

        return;

        break;
    }
    default: break;
    }

}

void CPEresolverDlg::OperateChange()//���������ؼ�����ʾ��ǰ�ؼ�
{

    switch (m_nCurrentSelect)
    {
    case EN_NONE:
    {
        return;
    }
    case EN_DOS:
    case EN_NT:
    case EN_FILE:
    case EN_OPTIONAL:
    case EN_SECTION:
    case EN_RELOCATION:
    case EN_TLS:
    {
        //�������У�ֻ����listData�ؼ�
        m_changeStaVA.ShowWindow(SW_HIDE);
        m_changeStaRVA.ShowWindow(SW_HIDE);
        m_changeStaOffset.ShowWindow(SW_HIDE);
        m_changeEdtVA.ShowWindow(SW_HIDE);
        m_changeEdtRVA.ShowWindow(SW_HIDE);
        m_changeEdtOffset.ShowWindow(SW_HIDE);
        m_changeBtnOK.ShowWindow(SW_HIDE);

        m_lstImportDLL.ShowWindow(SW_HIDE);
        m_lstImportFunc.ShowWindow(SW_HIDE);

        m_treeResource.ShowWindow(SW_HIDE);
        m_lstResource.ShowWindow(SW_HIDE);

        m_lstExportDll.ShowWindow(SW_HIDE);
        m_lstExportFunc.ShowWindow(SW_HIDE);

        //��ʾ
        m_lstData.ShowWindow(SW_SHOW);

        break;
    }
    case EN_ADDRCHANGE:
    {
        //�������У�ֻ���µ�ַת����ؿؼ�
        m_lstData.ShowWindow(SW_HIDE);

        m_lstImportDLL.ShowWindow(SW_HIDE);
        m_lstImportFunc.ShowWindow(SW_HIDE);

        m_lstExportDll.ShowWindow(SW_HIDE);
        m_lstExportFunc.ShowWindow(SW_HIDE);

        m_treeResource.ShowWindow(SW_HIDE);
        m_lstResource.ShowWindow(SW_HIDE);

        //��ʾ
        m_changeStaVA.ShowWindow(SW_SHOW);
        m_changeStaRVA.ShowWindow(SW_SHOW);
        m_changeStaOffset.ShowWindow(SW_SHOW);
        m_changeEdtVA.ShowWindow(SW_SHOW);
        m_changeEdtRVA.ShowWindow(SW_SHOW);
        m_changeEdtOffset.ShowWindow(SW_SHOW);
        m_changeBtnOK.ShowWindow(SW_SHOW);

        break;
    }
    case EN_IMPORTDLL:
    {
        //�������У�ֻ���µ������ؿؼ�
        m_lstData.ShowWindow(SW_HIDE);

        m_changeStaVA.ShowWindow(SW_HIDE);
        m_changeStaRVA.ShowWindow(SW_HIDE);
        m_changeStaOffset.ShowWindow(SW_HIDE);
        m_changeEdtVA.ShowWindow(SW_HIDE);
        m_changeEdtRVA.ShowWindow(SW_HIDE);
        m_changeEdtOffset.ShowWindow(SW_HIDE);
        m_changeBtnOK.ShowWindow(SW_HIDE);
        m_lstImportFunc.ShowWindow(SW_HIDE);

        m_lstExportDll.ShowWindow(SW_HIDE);
        m_lstExportFunc.ShowWindow(SW_HIDE);

        m_treeResource.ShowWindow(SW_HIDE);
        m_lstResource.ShowWindow(SW_HIDE);

        //��ʾ
        m_lstImportDLL.ShowWindow(SW_SHOW);
        
        break;
    }
    case EN_IMPORTFUNC:
    {
        //�������У�ֻ���µ������ؿؼ�
        m_lstData.ShowWindow(SW_HIDE);

        m_changeStaVA.ShowWindow(SW_HIDE);
        m_changeStaRVA.ShowWindow(SW_HIDE);
        m_changeStaOffset.ShowWindow(SW_HIDE);
        m_changeEdtVA.ShowWindow(SW_HIDE);
        m_changeEdtRVA.ShowWindow(SW_HIDE);
        m_changeEdtOffset.ShowWindow(SW_HIDE);
        m_changeBtnOK.ShowWindow(SW_HIDE);
       
        m_lstExportDll.ShowWindow(SW_HIDE);
        m_lstExportFunc.ShowWindow(SW_HIDE);

        m_treeResource.ShowWindow(SW_HIDE);
        m_lstResource.ShowWindow(SW_HIDE);

        //��ʾ
        m_lstImportDLL.ShowWindow(SW_SHOW);
        m_lstImportFunc.ShowWindow(SW_SHOW);
        break;
    }
    case EN_EXPORTDLL:
    {
        //�������У�ֻ���µ�������ؿؼ�
        m_lstData.ShowWindow(SW_HIDE);

        m_changeStaVA.ShowWindow(SW_HIDE);
        m_changeStaRVA.ShowWindow(SW_HIDE);
        m_changeStaOffset.ShowWindow(SW_HIDE);
        m_changeEdtVA.ShowWindow(SW_HIDE);
        m_changeEdtRVA.ShowWindow(SW_HIDE);
        m_changeEdtOffset.ShowWindow(SW_HIDE);
        m_changeBtnOK.ShowWindow(SW_HIDE);

        m_lstImportDLL.ShowWindow(SW_HIDE);
        m_lstImportFunc.ShowWindow(SW_HIDE);

        m_treeResource.ShowWindow(SW_HIDE);
        m_lstResource.ShowWindow(SW_HIDE);

        m_lstExportDll.ShowWindow(SW_SHOW);
        m_lstExportFunc.ShowWindow(SW_SHOW);
        break;
    }
    case EN_RESOURCE:
    {
        //�������У�ֻ���µ�������ؿؼ�
        m_lstData.ShowWindow(SW_HIDE);

        m_changeStaVA.ShowWindow(SW_HIDE);
        m_changeStaRVA.ShowWindow(SW_HIDE);
        m_changeStaOffset.ShowWindow(SW_HIDE);
        m_changeEdtVA.ShowWindow(SW_HIDE);
        m_changeEdtRVA.ShowWindow(SW_HIDE);
        m_changeEdtOffset.ShowWindow(SW_HIDE);
        m_changeBtnOK.ShowWindow(SW_HIDE);

        m_lstImportDLL.ShowWindow(SW_HIDE);
        m_lstImportFunc.ShowWindow(SW_HIDE);

        m_lstExportDll.ShowWindow(SW_HIDE);
        m_lstExportFunc.ShowWindow(SW_HIDE);

        m_treeResource.ShowWindow(SW_SHOW);
        break;
    }
    default:break;
    }

}

DWORD CPEresolverDlg::RVAtoOFFSET(DWORD dwRVA)
{
    //�ж��Ƿ��ڽ���
    if (dwRVA < m_pSection[0].VirtualAddress)
    {
        return dwRVA;
    }

    //�ж��Ƿ��нڱ�
    if (m_nSectionTableNum == 0)
    {
        return dwRVA;
    }

    int nBegin = 0; //��ǰ�����ڵ���ʼ
    int nEnd = 0;
    for (int i = 0; i < m_nSectionTableNum; i++)
    {
        nBegin = m_pSection[i].VirtualAddress;

        if (i == m_nSectionTableNum - 1)
        {
            nEnd = m_pOptional->SizeOfImage;
        }
        else
        {
            nEnd = m_pSection[i + 1].VirtualAddress;
        }

        if (dwRVA < nEnd && dwRVA >= nBegin)
        {
            DWORD dwRet = dwRVA - m_pSection[i].VirtualAddress + m_pSection[i].PointerToRawData;
            return dwRet;
        }
    }
    return 0;
}

void * CPEresolverDlg::RVAtoMapAddr(DWORD dwRVA)
{
    DWORD dwRet = RVAtoOFFSET(dwRVA);
    void* pRet = (void*)((int)m_pView + dwRet);
    return pRet;
}



BEGIN_MESSAGE_MAP(CPEresolverDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
    ON_BN_CLICKED(BTN_OK, &CPEresolverDlg::OnBnClickedOk)
    ON_NOTIFY(TVN_SELCHANGED, TREE_FILE, &CPEresolverDlg::OnTvnSelchangedFile)
    ON_NOTIFY(NM_DBLCLK, LST_DATA, &CPEresolverDlg::OnNMDblclkData)
    ON_EN_KILLFOCUS(IDC_EDIT1, &CPEresolverDlg::OnEnKillfocusEdit1)
    ON_BN_CLICKED(CHANGE_BTN_OK, &CPEresolverDlg::OnBnClickedBtnOk)
    ON_WM_CONTEXTMENU()
    ON_COMMAND(ID_1_2, &CPEresolverDlg::On12)
    ON_NOTIFY(NM_DBLCLK, LST_IMPORT_DLL, &CPEresolverDlg::OnNMDblclkImportDll)
    ON_NOTIFY(NM_DBLCLK, LST_IMPORT_FUNC, &CPEresolverDlg::OnNMDblclkImportFunc)
    ON_NOTIFY(NM_CLICK, LST_IMPORT_DLL, &CPEresolverDlg::OnNMClickImportDll)
    ON_NOTIFY(TVN_SELCHANGED, TREE_RESOURCE, &CPEresolverDlg::OnTvnSelchangedResource)
END_MESSAGE_MAP()


// CPEresolverDlg ��Ϣ�������

BOOL CPEresolverDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// ��������...���˵�����ӵ�ϵͳ�˵��С�

	// IDM_ABOUTBOX ������ϵͳ���Χ�ڡ�
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// ���ô˶Ի����ͼ�ꡣ  ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	// TODO: �ڴ���Ӷ���ĳ�ʼ������
    //��ȡPEͷlist�Ŀͻ���,�������ÿһ�еĿ�ȣ����÷��
    m_lstData.GetClientRect(&m_rect); 
    m_lstData.SetExtendedStyle(LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);
    m_lstData.ShowWindow(SW_HIDE);

    //��ȡ�����list�Ŀͻ���,�������ÿһ�еĿ��
    m_lstImportDLL.GetClientRect(&m_rectImportDll);
    m_lstImportDLL.SetExtendedStyle(LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);
    m_lstImportDLL.ShowWindow(SW_HIDE);

    m_lstImportFunc.GetClientRect(&m_rectImportFunc);
    m_lstImportFunc.SetExtendedStyle(LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);
    m_lstImportFunc.ShowWindow(SW_HIDE);

    m_lstExportDll.GetClientRect(&m_rectExportDll);
    m_lstExportDll.SetExtendedStyle(LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);
    m_lstExportDll.ShowWindow(SW_HIDE);
    
    m_lstExportFunc.GetClientRect(&m_rectExportFunc);
    m_lstExportFunc.SetExtendedStyle(LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);
    m_lstExportFunc.ShowWindow(SW_HIDE);

    m_lstResource.GetClientRect(&m_rectResource);
    m_lstResource.SetExtendedStyle(LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);
    m_lstResource.ShowWindow(SW_HIDE);

    m_treeResource.ShowWindow(SW_HIDE);

    //����edit 
    m_edtInput.ShowWindow(SW_HIDE);

    //���ص�ַת����ؿؼ�
    m_changeStaVA.ShowWindow(SW_HIDE);
    m_changeStaRVA.ShowWindow(SW_HIDE);
    m_changeStaOffset.ShowWindow(SW_HIDE);
    m_changeEdtVA.ShowWindow(SW_HIDE);
    m_changeEdtRVA.ShowWindow(SW_HIDE);
    m_changeEdtOffset.ShowWindow(SW_HIDE);
    m_changeBtnOK.ShowWindow(SW_HIDE);

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

void CPEresolverDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ  ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CPEresolverDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ����������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
HCURSOR CPEresolverDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CPEresolverDlg::OnBnClickedOk()//ȷ��
{
    m_nCurrentSelect = EN_NONE;
    //�������ؼ�
    m_treeFile.DeleteAllItems();

    //�����ļ��򿪣����ͷ�
    if (m_pView)
    {
        //1. ȡ��ӳ����ͼ
        UnmapViewOfFile(m_pView);
        //2. �ر�ӳ�����
        CloseHandle(m_hFileMap);
        //3. �ر��ļ�
        CloseHandle(m_hFile);
    }

    //��ȡ�ļ��������ļ��������ļ�ӳ�䣬��ȡ�ļ���С
    if (!CreateFileMap())
    {
        return;
    }

    //�ж��Ƿ�ΪPE
    if (m_pDOS->e_magic != 0x5A4D)
    {
        AfxMessageBox("���ļ�����һ��PE�ļ�");
        return;
    }
    if (m_pNT->Signature != 0X4550)
    {
        AfxMessageBox("���ļ�����һ��PE�ļ�");
        return;
    }
    
    //��ʼ�����ؼ�
    if (!InitTreeControl())
    {
        return;
    }

    //����ƫ��
    m_nOffsetNT = m_pDOS->e_lfanew;
    m_nOffsetFile = m_nOffsetNT + 4;
    m_nOffsetOptional = m_nOffsetFile + sizeof(IMAGE_FILE_HEADER);
    m_nOffsetSection = m_nOffsetOptional + m_pFILE->SizeOfOptionalHeader;
    m_nOffsetFirstSection = m_pSection->PointerToRawData;

    //Ŀ¼�����
    m_nNumberOfData = m_pOptional->NumberOfRvaAndSizes;

    //���㵼����ļ�ƫ�Ƶ�ַ
    if (m_nNumberOfData < 2)
    {
        m_nOffsetImport = 0;
    }
    else
    {
        int nRVAImport = m_pOptional->DataDirectory[1].VirtualAddress;//������RVA
        m_nOffsetImport = RVAtoOFFSET(nRVAImport);
    }
    
    if (m_nNumberOfData >= 1)
    {
        int nRVAExport = m_pOptional->DataDirectory[0].VirtualAddress;//�������RVA
        m_nOffsetExport = RVAtoOFFSET(nRVAExport);
    }
    else
    {
        m_nOffsetExport = 0;
    }

    //����ӳ���е����ĵ�ַ
    if (m_nOffsetImport != 0)
    {
        m_pImport = (IMAGE_IMPORT_DESCRIPTOR*)((int)m_pView + m_nOffsetImport);
    }
    else
    {
        m_pImport = 0;
    }

    //����ӳ���е�����ĵ�ַ
    if (m_nOffsetExport != 0)
    {
        m_pExport = (IMAGE_EXPORT_DIRECTORY*)((int)m_pView + m_nOffsetExport);
    }
    else
    {
        m_pExport = 0;
    }

    //�����ض�λ��
    if (m_nNumberOfData >= 5)
    {
        int nRVAReloc = m_pOptional->DataDirectory[5].VirtualAddress;//�ض�λ����RVA
        m_nOffsetRelocation = RVAtoOFFSET(nRVAReloc);
        m_nSizeOfRelocation = m_pOptional->DataDirectory[5].Size;

        m_pRelocation = (IMAGE_BASE_RELOCATION*)((int)m_pView + m_nOffsetRelocation);
    }
    else
    {
        m_nOffsetRelocation = 0;
        m_pRelocation = 0;
    }
    
    //����ӳ����TLs��ĵ�ַ
    if (m_nNumberOfData < 9)
    {
        m_pTLs = nullptr;
        m_nOffsetTls = 0;
    }
    else
    {
        int nTLs = m_pOptional->DataDirectory[9].VirtualAddress;//TLs���RVA
        m_nOffsetTls = RVAtoOFFSET(nTLs);

        m_pTLs = (IMAGE_TLS_DIRECTORY32*)((int)m_pView + m_nOffsetTls);
    }
    
    //����ӳ������Դ��ĵ�ַ
    if (m_nNumberOfData < 2)
    {
        m_pResource = nullptr;
        m_nOffsetResource = 0;
        m_nSizeResource = 0;
    }
    else
    {
        int nResource = m_pOptional->DataDirectory[2].VirtualAddress;
        m_nOffsetResource = RVAtoOFFSET(nResource);
        m_nSizeResource = m_pOptional->DataDirectory[2].Size;

        m_pResource = (IMAGE_RESOURCE_DIRECTORY_ENTRY*)((int)m_pView + m_nOffsetResource);
    }
}


void CPEresolverDlg::OnTvnSelchangedFile(NMHDR *pNMHDR, LRESULT *pResult)//������ؼ�����
{
    LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
    // TODO: �ڴ���ӿؼ�֪ͨ����������
    *pResult = 0;

    HTREEITEM hItem = m_treeFile.GetSelectedItem();
    CString csItem = m_treeFile.GetItemText(hItem);


    if (strcmp(csItem.GetBuffer(), "_IMAGE_DOS_HEADER") == 0)//DOSͷ
    {
        ShowDosHeader();
    }
    else if (strcmp(csItem.GetBuffer(), "_IMAGE_NT_HEADERS") == 0)//NT
    {
        //ShowNTHeader();
    }
    else if (strcmp(csItem.GetBuffer(), "Signature") == 0)//PE��ʶ
    {
        ShowPESign();
    }
    else if (strcmp(csItem.GetBuffer(), "IMAGE_FILE_HEADER") == 0)//PEͷ
    {
        ShowFileHeader();
    }
    else if (strcmp(csItem.GetBuffer(), "IMAGE_OPTIONAL_HEADER32") == 0)//��ѡPEͷ
    {
        ShowOptionalHeader();
    }
    else if (strcmp(csItem.GetBuffer(), "�ڱ�") == 0)//�ڱ�
    {
        ShowSectionTable();
    }
    else if (strcmp(csItem.GetBuffer(), "��ַת��") == 0)//��ַת��
    {
        m_nCurrentSelect = EN_ADDRCHANGE;
    }
    else if (strcmp(csItem.GetBuffer(), "����Ŀ¼") == 0)//����Ŀ¼
    {
        ShowImport();
    }
    else if (strcmp(csItem.GetBuffer(), "����Ŀ¼") == 0)//����Ŀ¼
    {
        ShowExport();
    }
    else if (strcmp(csItem.GetBuffer(), "�ض�λ��") == 0)//����Ŀ¼
    {
        ShowRelocation();
    }
    else if (strcmp(csItem.GetBuffer(), "TLs��") == 0)//����Ŀ¼
    {
        ShowTLs();
    }
    else if (strcmp(csItem.GetBuffer(), "��Դ��") == 0)//��Դ��
    {
        ShowResource();
    }

    OperateChange(); 
}


void CPEresolverDlg::OnNMDblclkData(NMHDR *pNMHDR, LRESULT *pResult)//˫���б�ؼ�
{
    LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
    // �ж��Ƿ����Ŀɱ༭��
    switch (m_nCurrentSelect)
    {
    case EN_NONE:
    {
        return;
    }
    case EN_DOS:
    case EN_NT:
    case EN_FILE:
    case EN_OPTIONAL:
    {
        //��ȡ����
        m_nRow = pNMItemActivate->iItem;//��
        m_nColumn = pNMItemActivate->iSubItem;//��

        if (m_nColumn != 3)
        {
            m_nRow = -1;
            m_nColumn = -1;
            return;
        }
        break;
    }
    case EN_SECTION:
    {
        //��ȡ����
        m_nRow = pNMItemActivate->iItem;//��
        m_nColumn = pNMItemActivate->iSubItem;//��

        if (m_nColumn != 4)
        {
            m_nRow = -1;
            m_nColumn = -1;
            return;
        }
        break;
    }
    default:break;
    }

    //��ȡ����ľ���
    CRect rc;
    m_lstData.GetSubItemRect(m_nRow, m_nColumn, LVIR_LABEL, rc);
    //��ȡ���ؼ��ľ���
    CRect treeRc;
    m_treeFile.GetWindowRect(&treeRc);

    //ת������
    rc.left += treeRc.right - treeRc.left + 17;
    rc.right += treeRc.right - treeRc.left + 17;
    rc.top += 5;
    rc.bottom += 20;

    //��ȡ�����ֵ
    CString cs = m_lstData.GetItemText(m_nRow, m_nColumn);

    m_edtInput.ShowWindow(SW_SHOW);

    m_edtInput.SetWindowPos(&CWnd::wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
    m_edtInput.MoveWindow(&rc);
    m_edtInput.SetFocus();
    m_edtInput.SetWindowText(cs);
    ::SetFocus(m_edtInput.GetSafeHwnd());//���ý���
    m_edtInput.SetSel(-1);

    *pResult = 0;
}


void CPEresolverDlg::OnEnKillfocusEdit1()//�༭��ʧȥ���뽹��
{
    //��ȡ�༭���ֵ
    CString cs;
    GetDlgItemText(EDT_INPUT, cs);
    int nTemp = 0;
    if (cs.GetLength() <= 8)
    {
        nTemp = strtoul(cs.GetBuffer(), NULL, 16);
    }
    
    //��������
    //1. �ҵ����ݿ����ʼ��ַ
    //2. ����ƫ��
    //3. ��ֵ
    //4. ���б�ؼ�����
    void* pData = nullptr;
    switch (m_nCurrentSelect)
    {
    case EN_NONE:
    {
        return;
    }
    case EN_DOS:
    {
        pData = m_pDOS;

        for (int i = 0; i < m_nRow; i++)
        {
            pData = (void*)((int)pData + g_pDosHeader[i].nSize);
        }

        if (g_pDosHeader[m_nRow].nSize == 2)
        {
            *(WORD*)pData = nTemp;
        }
        else
        {
            *(DWORD*)pData = nTemp;
        }

        break;
    }
    case EN_NT:
    {
        pData = m_pNT;

        *(DWORD*)pData = nTemp;

        break;
    }
    case EN_FILE:
    {
        pData = m_pFILE;

        for (int i = 0; i < m_nRow; i++)
        {
            pData = (void*)((int)pData + g_pFileHeader[i].nSize);
        }

        if (g_pFileHeader[m_nRow].nSize == 2)
        {
            *(WORD*)pData = nTemp;
        }
        else
        {
            *(DWORD*)pData = nTemp;
        }

        break;
    }
    case EN_OPTIONAL:
    {
        pData = m_pOptional;

        for (int i = 0; i < m_nRow; i++)
        {
            pData = (void*)((int)pData + g_pOptional[i].nSize);
        }

        if (g_pOptional[m_nRow].nSize == 1)
        {
            *(BYTE*)pData = nTemp;
        }
        else if (g_pOptional[m_nRow].nSize == 2)
        {
            *(WORD*)pData = nTemp;
        }
        else if (g_pOptional[m_nRow].nSize == 4)
        {
            *(DWORD*)pData = nTemp;
        }
        else//8�ֽڽṹ�� �˴��Ǵ���ֵ
        {
            char szBuff1[16] = { 0 };
            char szBuff2[16] = { 0 };

            memcpy(szBuff1, cs.GetBuffer(), 8);
            memcpy(szBuff2, cs.GetBuffer() + 9, 8);

            int nTemp1 = strtoul(szBuff1, NULL, 16);
            int nTemp2 = strtoul(szBuff2, NULL, 16);

            *(DWORD*)pData = nTemp1;
            *(DWORD*)((int)pData + 4) = nTemp2;
        }
        break;
    }
    case EN_SECTION:
    {
        pData = m_pSection;

        for (int i = 0; i < m_nRow; i++)
        {
            pData = (void*)((int)pData + g_pSectionTable[i].nSize);
        }

        if (g_pSectionTable[m_nRow].nSize == 2)
        {
            *(WORD*)pData = nTemp;
        }
        else if (g_pSectionTable[m_nRow].nSize == 4)
        {
            *(DWORD*)pData = nTemp;
        }
        else
        {
            //�˴��Ǵ��ַ���
            for (int i = 0; i < 8; i++)
            {
                int cTemp = 0;
                char szBuff[10] = { 0 };

                memcpy(szBuff, cs.GetBuffer() + i * 2, 2);
                cTemp = strtoul(szBuff, NULL, 16);

                *(DWORD*)((int)pData + i) = cTemp;
            }
        }
        break;
    }
    default:break;
    }

    //��������д��ؼ���
    m_lstData.SetItemText(m_nRow, m_nColumn, cs.GetBuffer());

    //��������Ϊ-1
    m_nRow = -1;
    m_nColumn = -1;

    //����EDIT
    m_edtInput.ShowWindow(SW_HIDE);
}


void CPEresolverDlg::OnOK()
{
    // �˺�������Ϊ�գ����򰴻س��ᵼ�¶�ʧ���㣬Ȼ����ô˺���
}


void CPEresolverDlg::OnBnClickedBtnOk()//�����ַת��
{
    CString csVA;
    CString csRVA;
    CString csOffset;

    GetDlgItemText(CHANGE_EDT_VA, csVA);
    GetDlgItemText(CHANGE_EDT_RVA, csRVA);
    GetDlgItemText(CHANGE_EDT_OFFSET, csOffset);

    if (csVA.GetLength() == 0 && csRVA.GetLength() == 0 && csOffset.GetLength() == 0)
    {
        AfxMessageBox("����������ֵ");
        return;
    }

    int nOffset = 0;
    int nVA = 0;
    int nRVA = 0;

    if (csVA.GetLength() != 0)//��������ַ
    {
        nVA = strtoul(csVA.GetBuffer(), NULL, 16);
        nRVA = nVA - m_nImageBase;

        //�ж��Ƿ��ڽ���
        if (nRVA < m_pSection[0].VirtualAddress)
        {
            nOffset = nRVA;
        }
        else
        {
            int nBegin = 0; //��ǰ�����ڵ���ʼ
            int nEnd = 0;
            for (int i = 0; i < m_nSectionTableNum; i++)
            {
                nBegin = m_pSection[i].VirtualAddress;

                if (i == m_nSectionTableNum - 1)
                {
                    nEnd = m_pOptional->SizeOfImage;
                }
                else
                {
                    nEnd = m_pSection[i + 1].VirtualAddress;
                }

                if (nRVA <= nEnd && nRVA >= nBegin)
                {
                    nOffset = nRVA - m_pSection[i].VirtualAddress + m_pSection[i].PointerToRawData;
                    break;
                }
            }
        }

        //ת��
        char szVA[16] = { 0 };
        char szRVA[16] = { 0 };
        char szOffset[16] = { 0 };

        wsprintf(szVA, "0X%p", nVA);
        wsprintf(szRVA, "0X%p", nRVA);
        wsprintf(szOffset, "0X%p", nOffset);

        //д��
        SetDlgItemText(CHANGE_EDT_VA, szVA);
        SetDlgItemText(CHANGE_EDT_RVA, szRVA);
        SetDlgItemText(CHANGE_EDT_OFFSET, szOffset);
        return;
    }
    if (csRVA.GetLength() != 0)//�����������ַƫ��
    {
        nRVA = strtoul(csRVA.GetBuffer(), NULL, 16);
        nVA = nRVA + m_nImageBase;

        //�ж��Ƿ��ڽ���
        if (nRVA < m_pSection[0].VirtualAddress)
        {
            nOffset = nRVA;
        }
        else
        {
            int nBegin = 0; //��ǰ�����ڵ���ʼ
            int nEnd = 0;
            for (int i = 0; i < m_nSectionTableNum; i++)
            {
                nBegin = m_pSection[i].VirtualAddress;

                if (i == m_nSectionTableNum - 1)
                {
                    nEnd = m_pOptional->SizeOfImage;
                }
                else
                {
                    nEnd = m_pSection[i + 1].VirtualAddress;
                }

                if (nRVA <= nEnd && nRVA >= nBegin)
                {
                    nOffset = nRVA - m_pSection[i].VirtualAddress + m_pSection[i].PointerToRawData;
                    break;
                }
            }
        }

        //ת��
        char szVA[16] = { 0 };
        char szRVA[16] = { 0 };
        char szOffset[16] = { 0 };

        wsprintf(szVA, "0X%p", nVA);
        wsprintf(szRVA, "0X%p", nRVA);
        wsprintf(szOffset, "0X%p", nOffset);

        //д��
        SetDlgItemText(CHANGE_EDT_VA, szVA);
        SetDlgItemText(CHANGE_EDT_RVA, szRVA);
        SetDlgItemText(CHANGE_EDT_OFFSET, szOffset);
        return;
    }

    if (csOffset.GetLength() != 0)//����ļ�ƫ��
    {
        //��ȡƫ��
        nOffset = strtoul(csOffset.GetBuffer(), NULL, 16);

        //�ж�
        if (nOffset > m_FileSize)
        {
            AfxMessageBox("�޷�ת������ֵ��ƫ��");
            return;
        }

        if (nOffset <= m_nOffsetFirstSection)//��ͷ��
        {
            nVA = nOffset + m_nImageBase;
            nRVA = nOffset;
        }
        else//�ڽڿ���
        {
            int nBegin = 0; //��ǰ�����ڵ���ʼ
            int nEnd = 0;
            for (int i = 0; i < m_nSectionTableNum; i++)
            {
                nBegin = m_pSection[i].PointerToRawData;
                
                if (i == m_nSectionTableNum - 1)
                {
                    nEnd = m_FileSize;
                }
                else
                {
                    nEnd = m_pSection[i + 1].PointerToRawData - 1;
                }

                if (nOffset >= nBegin && nOffset <= nEnd)
                {
                    nVA = m_nImageBase + m_pSection[i].VirtualAddress + nOffset - nBegin;
                    nRVA = nVA - m_nImageBase;
                    break;
                }
            } 
        }
        //ת��
        char szVA[16] = { 0 };
        char szRVA[16] = { 0 };
        char szOffset[16] = { 0 };

        wsprintf(szVA, "0X%p", nVA);
        wsprintf(szRVA, "0X%p", nRVA);
        wsprintf(szOffset, "0X%p", nOffset);

        //д��
        SetDlgItemText(CHANGE_EDT_VA, szVA);
        SetDlgItemText(CHANGE_EDT_RVA, szRVA);
        SetDlgItemText(CHANGE_EDT_OFFSET, szOffset);

        return;
    }
    return;
}


void CPEresolverDlg::OnContextMenu(CWnd* pWnd, CPoint point)
{
    if (m_nCurrentSelect != EN_SECTION)
    {
        return;
    }

    CMenu menu;
    menu.LoadMenu(IDR_MENU1);
    CMenu * pMenu;
    pMenu = menu.GetSubMenu(0);

    pMenu->EnableMenuItem(ID_1_2, MF_BYCOMMAND | MF_ENABLED);
    //pMenu->EnableMenuItem(ID_32772, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
    pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this);
    pMenu->Detach();
    menu.DestroyMenu();
}


void CPEresolverDlg::On12()//����Ҽ��˵��½��ڿ�
{
    CFileDialog dlgFile(TRUE);
    int nRet = dlgFile.DoModal();

    if (nRet == IDCANCEL)
    {
        return;
    }
    //�޸Ľڵ�����
    m_pFILE->NumberOfSections += 1;

    CString csFilePath;
    csFilePath = dlgFile.GetPathName();

    //��Դ�ļ�
    HANDLE hSourFile = CreateFile(csFilePath.GetBuffer(),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (INVALID_HANDLE_VALUE == hSourFile)
    {
        AfxMessageBox("�ļ���ʧ�ܣ�");
        return;
    }

    //��ȡ�ļ���С
    DWORD dwHigh = 0;
    DWORD dwLow = GetFileSize(hSourFile, &dwHigh);


    //1. д����Ϣ�����ļ�
    strcpy_s((char*)m_pSection[m_nSectionTableNum].Name, 8, ".New");
    m_pSection[m_nSectionTableNum].Misc.VirtualSize = dwLow;
    m_pSection[m_nSectionTableNum].VirtualAddress = m_pSection[m_nSectionTableNum - 1].VirtualAddress +
        (m_pSection[m_nSectionTableNum - 1].Misc.VirtualSize / m_pOptional->SectionAlignment + 1) *
        m_pOptional->SectionAlignment;
    m_pSection[m_nSectionTableNum].SizeOfRawData = (dwLow / m_pOptional->FileAlignment  + 1) *
        m_pOptional->FileAlignment;
    m_pSection[m_nSectionTableNum].PointerToRawData = (m_FileSize / m_pOptional->FileAlignment + 1) *
        m_pOptional->FileAlignment;

    int nPointerToRawData = m_pSection[m_nSectionTableNum].PointerToRawData;

    m_pSection[m_nSectionTableNum].Characteristics = 0X80000000 | 0X40000000 | 0X20000000;

    //�޸��ڴ���PE�ļ���ӳ���С
    m_pOptional->SizeOfImage += (dwLow / m_pOptional->FileAlignment + 1) * m_pOptional->FileAlignment;

    if (m_pView != nullptr)
    {
        //1. ȡ��ӳ����ͼ
        UnmapViewOfFile(m_pView);
        //2. �ر�ӳ�����
        CloseHandle(m_hFileMap);
        //3. �ر��ļ�
        CloseHandle(m_hFile);
    }

    //2. ���½�д���ļ�
    //��Ŀ���ļ�
    HANDLE hDestFile = CreateFile(m_csFilePath.GetBuffer(),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (INVALID_HANDLE_VALUE == hDestFile)
    {
        AfxMessageBox("�ļ���ʧ�ܣ�");
        return;
    }

    SetFilePointer(hDestFile, 0, 0, FILE_END);
    //�����ļ�ĩβ���0��ʹ�ļ���СΪ�ļ����������������������������ݽ�ȥ
    for (int i = 0; i < nPointerToRawData - m_FileSize; i++)
    {
        char cData = 0;
        DWORD nReadWrite = 1;
        //д�ļ�
        WriteFile(hDestFile, &cData, 1, &nReadWrite, NULL);
    }

    for (int i = 0; i < dwLow; i++)
    {
        char cData = 0;
        DWORD nReadWrite = 1;
        //���ļ�
        ReadFile(hSourFile, &cData, 1, &nReadWrite, NULL);

        //д�ļ�
        WriteFile(hDestFile, &cData, 1, &nReadWrite, NULL);

    }
    //�ļ�����
    int nFullZero = (dwLow / m_nFileAlignment + 1) * m_nFileAlignment - dwLow;
    for (int i = 0; i < nFullZero; i++)
    {
        char cData = 0;
        DWORD nReadWrite = 1;
        //д�ļ�
        WriteFile(hDestFile, &cData, 1, &nReadWrite, NULL);

    }

    //���ļ�
    CloseHandle(hSourFile);
    CloseHandle(hDestFile);

    //���¶��ļ�
    CreateFileMap();

    //���²����ڱ�
    ShowSectionTable();
    OperateChange();
}


void CPEresolverDlg::OnNMDblclkImportDll(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
    // TODO: �ڴ���ӿؼ�֪ͨ����������
    m_nCurrentSelect = EN_IMPORTDLL;
    m_nRow = pNMItemActivate->iItem;//��
    m_nColumn = pNMItemActivate->iSubItem;//��



    *pResult = 0;
}


void CPEresolverDlg::OnNMDblclkImportFunc(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
    // TODO: �ڴ���ӿؼ�֪ͨ����������
    m_nCurrentSelect = EN_IMPORTFUNC;
    m_nRow = pNMItemActivate->iItem;//��
    m_nColumn = pNMItemActivate->iSubItem;//��


    *pResult = 0;
}


void CPEresolverDlg::OnNMClickImportDll(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
    // TODO: �ڴ���ӿؼ�֪ͨ����������
    m_nCurrentSelect = EN_IMPORTDLL;
    m_nRow = pNMItemActivate->iItem;//��
    

    //�༭�����FUNC���֣�����ʾ����
    if (m_nRow > 0)
    {
        ShowImportFunc();
    }

    *pResult = 0;
}


void CPEresolverDlg::OnTvnSelchangedResource(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);

    m_lstResource.ShowWindow(SW_SHOW);

    ClearListControl(m_lstResource);

    HTREEITEM hItem = m_treeResource.GetSelectedItem();
    DWORD dwData = m_treeResource.GetItemData(hItem);
    
    //����±���
    m_lstResource.InsertColumn(0, _T("��Ա"), LVCFMT_LEFT, m_rectResource.right / 4);
    m_lstResource.InsertColumn(1, _T("ƫ����"), LVCFMT_LEFT, m_rectResource.right / 4);
    m_lstResource.InsertColumn(2, _T("��С"), LVCFMT_LEFT, m_rectResource.right / 4);
    m_lstResource.InsertColumn(3, _T("ֵ"), LVCFMT_LEFT, m_rectResource.right / 4);

    char szTemp[100] = { 0 };
    DWORD dwType = dwData & 0Xf0000000;
    dwData = dwData & 0X0fffffff;
    //�����
    if (dwType == 0X10000000)//����
    {
        //����
        m_lstResource.InsertItem(0, "OffsetToData");
        m_lstResource.InsertItem(1, "Size");
        m_lstResource.InsertItem(2, "CodePage");
        m_lstResource.InsertItem(3, "Reserved");

        //ƫ����
        DWORD dwOffset = dwData - (DWORD)m_pView;
        wsprintf(szTemp, "0X%p", dwOffset);
        m_lstResource.SetItemText(0, 1, szTemp);

        dwOffset = dwOffset + sizeof(DWORD);
        wsprintf(szTemp, "0X%p", dwOffset);
        m_lstResource.SetItemText(1, 1, szTemp);

        dwOffset = dwOffset + sizeof(DWORD);
        wsprintf(szTemp, "0X%p", dwOffset);
        m_lstResource.SetItemText(2, 1, szTemp);

        dwOffset = dwOffset + sizeof(DWORD);
        wsprintf(szTemp, "0X%p", dwOffset);
        m_lstResource.SetItemText(3, 1, szTemp);

        //��С
        m_lstResource.SetItemText(0, 2, "DWORD");
        m_lstResource.SetItemText(1, 2, "DWORD");
        m_lstResource.SetItemText(2, 2, "DWORD");
        m_lstResource.SetItemText(3, 2, "DWORD");

        //ֵ
        DWORD* pdwData = (DWORD*)dwData;
        wsprintf(szTemp, "%p", *pdwData);
        m_lstResource.SetItemText(0, 3, szTemp);

        pdwData++;
        wsprintf(szTemp, "%p", *pdwData);
        m_lstResource.SetItemText(1, 3, szTemp);

        pdwData++;
        wsprintf(szTemp, "%p", *pdwData);
        m_lstResource.SetItemText(2, 3, szTemp);

        pdwData++;
        wsprintf(szTemp, "%p", *pdwData);
        m_lstResource.SetItemText(3, 3, szTemp);
    }
    else if (dwType == 0X20000000)//��ԴĿ¼��
    {
        m_lstResource.InsertItem(0, "Name");
        m_lstResource.InsertItem(1, "OffsetToData");

        //ƫ����
        DWORD dwOffset = dwData - (DWORD)m_pView;
        wsprintf(szTemp, "0X%p", dwOffset);
        m_lstResource.SetItemText(0, 1, szTemp);

        dwOffset = dwOffset + sizeof(DWORD);
        wsprintf(szTemp, "0X%p", dwOffset);
        m_lstResource.SetItemText(1, 1, szTemp);

        //��С
        m_lstResource.SetItemText(0, 2, "DWORD");
        m_lstResource.SetItemText(1, 2, "DWORD");

        //ֵ
        DWORD* pdwData = (DWORD*)dwData;
        wsprintf(szTemp, "%p", *pdwData);
        m_lstResource.SetItemText(0, 3, szTemp);

        pdwData++;
        wsprintf(szTemp, "%p", *pdwData);
        m_lstResource.SetItemText(1, 3, szTemp);

    }
    else if (dwType == 0X40000000)//��ԴĿ¼
    {
        //����
        m_lstResource.InsertItem(0, "Characteristics");
        m_lstResource.InsertItem(1, "TimeDateStamp");
        m_lstResource.InsertItem(2, "MajorVersion");
        m_lstResource.InsertItem(3, "MinorVersion");
        m_lstResource.InsertItem(4, "NumberOfNamedEntries");
        m_lstResource.InsertItem(5, "NumberOfIdEntries");

        //ƫ����
        DWORD dwOffset = dwData - (DWORD)m_pView;
        wsprintf(szTemp, "0X%p", dwOffset);
        m_lstResource.SetItemText(0, 1, szTemp);

        dwOffset = dwOffset + sizeof(DWORD);
        wsprintf(szTemp, "0X%p", dwOffset);
        m_lstResource.SetItemText(1, 1, szTemp);

        dwOffset = dwOffset + sizeof(DWORD);
        wsprintf(szTemp, "0X%p", dwOffset);
        m_lstResource.SetItemText(2, 1, szTemp);

        dwOffset = dwOffset + sizeof(WORD);
        wsprintf(szTemp, "0X%p", dwOffset);
        m_lstResource.SetItemText(3, 1, szTemp);

        dwOffset = dwOffset + sizeof(WORD);
        wsprintf(szTemp, "0X%p", dwOffset);
        m_lstResource.SetItemText(4, 1, szTemp);

        dwOffset = dwOffset + sizeof(WORD);
        wsprintf(szTemp, "0X%p", dwOffset);
        m_lstResource.SetItemText(5, 1, szTemp);

        //��С
        m_lstResource.SetItemText(0, 2, "DWORD");
        m_lstResource.SetItemText(1, 2, "DWORD");
        m_lstResource.SetItemText(2, 2, "WORD");
        m_lstResource.SetItemText(3, 2, "WORD");
        m_lstResource.SetItemText(4, 2, "WORD");
        m_lstResource.SetItemText(5, 2, "WORD");

        //ֵ
        void* pdwData = (void*)dwData;
        wsprintf(szTemp, "%p", *(DWORD*)pdwData);
        m_lstResource.SetItemText(0, 3, szTemp);
        pdwData = (void*)((int)pdwData + sizeof(DWORD));

        wsprintf(szTemp, "%p", *(DWORD*)pdwData);
        m_lstResource.SetItemText(1, 3, szTemp);
        pdwData = (void*)((int)pdwData + sizeof(DWORD));

        wsprintf(szTemp, "%04X", *(WORD*)pdwData);
        m_lstResource.SetItemText(2, 3, szTemp);
        pdwData = (void*)((int)pdwData + sizeof(WORD));

        wsprintf(szTemp, "%04X", *(WORD*)pdwData);
        m_lstResource.SetItemText(3, 3, szTemp);
        pdwData = (void*)((int)pdwData + sizeof(WORD));

        wsprintf(szTemp, "%04X", *(WORD*)pdwData);
        m_lstResource.SetItemText(4, 3, szTemp);
        pdwData = (void*)((int)pdwData + sizeof(WORD));

        wsprintf(szTemp, "%04X", *(WORD*)pdwData);
        m_lstResource.SetItemText(5, 3, szTemp);
    }

    *pResult = 0;
}
