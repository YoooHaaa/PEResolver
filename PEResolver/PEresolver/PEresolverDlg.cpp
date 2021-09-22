
// PEresolverDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "PEresolver.h"
#include "PEresolverDlg.h"
#include "afxdialogex.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
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


// CPEresolverDlg 对话框



CPEresolverDlg::CPEresolverDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_PERESOLVER_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CPEresolverDlg::~CPEresolverDlg()
{
    if (m_pView != nullptr)
    {
        //1. 取消映射视图
        UnmapViewOfFile(m_pView);
        //2. 关闭映射对象
        CloseHandle(m_hFileMap);
        //3. 关闭文件
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
    //获取文本框信息
    GetDlgItemText(EDT_FILEPATH, m_csFilePath);

    //打开文件m_hFile
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
        AfxMessageBox("文件打开失败！");
        return false;
    }
    

    //获取进程名
    strcpy_s(m_szPath, MAX_PATH, m_csFilePath.GetBuffer());
    GetProcessName();

    //创建文件映射对象
    m_hFileMap = CreateFileMapping(
        m_hFile,
        NULL,
        PAGE_READWRITE, //可读可写
        0,
        0, //最多将整个文件映射进内存
        NULL);//没有名字
    if (m_hFileMap == NULL)
    {
        AfxMessageBox("文件映射对象创建失败");
        CloseHandle(m_hFile);
        return false;
    }

    //获取文件大小
    DWORD dwHigh = 0;
    DWORD dwLow = GetFileSize(m_hFile, &dwHigh);
    //UINT64 m_uFileSize = ((UINT64)dwHigh << 32) + dwLow;//一般文件大小没到8字节，4字节就够
    m_FileSize = dwLow;

    //创建映射视图,把文件全部映射到内存
    m_pView = MapViewOfFile(
        m_hFileMap,
        FILE_MAP_WRITE, //可读可写
        0,
        0, //从0开始映射进内存
        0); //映射整个文件
    if (m_pView == NULL)
    {
        AfxMessageBox("文件映射失败");
        CloseHandle(m_hFile);
        CloseHandle(m_hFileMap);
        return false;
    }

    //获取结构体地址
    m_pDOS = (_IMAGE_DOS_HEADER*)m_pView;
    m_pNT = (_IMAGE_NT_HEADERS*)(m_pDOS->e_lfanew + (int)m_pDOS);
    m_pFILE = (IMAGE_FILE_HEADER*)((int)m_pNT + 4);
    m_pOptional = (IMAGE_OPTIONAL_HEADER32*)((int)m_pFILE + 20);
    m_pSection = (IMAGE_SECTION_HEADER*)((int)m_pOptional + m_pFILE->SizeOfOptionalHeader);

    //获取内存镜像基址
    m_nImageBase = m_pOptional->ImageBase;

    //文件对齐大小
    m_nFileAlignment = m_pOptional->FileAlignment;

    //内存对齐大小
    m_nSectionAlignment = m_pOptional->SectionAlignment;

    //获取节表数量
    m_nSectionTableNum = m_pFILE->NumberOfSections;
    return true;
}

bool CPEresolverDlg::InitTreeControl()
{
    //插入进程名，返回进程的句柄
    HTREEITEM  hProcess = m_treeFile.InsertItem(m_szProcessName);

    //插入PE的各结构体
    //DOS头
    HTREEITEM  hDOS = m_treeFile.InsertItem("DOS头", NULL, NULL, hProcess);

    //DOS  _IMAGE_DOS_HEADER
    HTREEITEM  hDOSHeader = m_treeFile.InsertItem("_IMAGE_DOS_HEADER", NULL, NULL, hDOS);

    //PE头
    HTREEITEM  hPE = m_treeFile.InsertItem("PE头", NULL, NULL, hProcess);

    //NT头 _IMAGE_NT_HEADERS
    HTREEITEM  hNtHead = m_treeFile.InsertItem("_IMAGE_NT_HEADERS", NULL, NULL, hPE);

    //PE标识 PE
    HTREEITEM  hSignature = m_treeFile.InsertItem("Signature", NULL, NULL, hNtHead);

    //PE头 IMAGE_FILE_HEADER 
    HTREEITEM  hFileHead = m_treeFile.InsertItem("IMAGE_FILE_HEADER", NULL, NULL, hNtHead);

    //可选PE头 IMAGE_OPTIONAL_HEADER32  
    HTREEITEM  hOptionalHead = m_treeFile.InsertItem("IMAGE_OPTIONAL_HEADER32", NULL, NULL, hNtHead);

    //节表
    HTREEITEM  hSection = m_treeFile.InsertItem("节表", NULL, NULL, hProcess);

    //地址转换
    HTREEITEM  hAddrResolution = m_treeFile.InsertItem("地址转换", NULL, NULL, hProcess);

    //导入目录
    HTREEITEM  hImprotDirect = m_treeFile.InsertItem("导入目录", NULL, NULL, hProcess);

    //导出目录
    HTREEITEM  hExprotDirect = m_treeFile.InsertItem("导出目录", NULL, NULL, hProcess);

    //重定位表
    HTREEITEM  hRelocation = m_treeFile.InsertItem("重定位表", NULL, NULL, hProcess);

    //TLs表
    HTREEITEM  hTLs = m_treeFile.InsertItem("TLs表", NULL, NULL, hProcess);

    //资源表
    HTREEITEM  hResource = m_treeFile.InsertItem("资源表", NULL, NULL, hProcess);

    return true;
}

void CPEresolverDlg::ClearListControl(CListCtrl& MyList)
{
    //删除原表内容
    int nCount = MyList.GetItemCount();
    for (int i = 0; i < nCount; i++)
    {
        MyList.DeleteItem(0);
    }

    //删除原标题
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
    //设置list控件标题
    //删除原内容
    ClearListControl(m_lstData);

    //添加新标题
    m_lstData.InsertColumn(0, _T("内涵"), LVCFMT_LEFT, m_rect.right * 2 / 5);
    m_lstData.InsertColumn(0, _T("值"), LVCFMT_LEFT, m_rect.right / 5);
    m_lstData.InsertColumn(0, _T("大小"), LVCFMT_LEFT, m_rect.right / 5);
    m_lstData.InsertColumn(0, _T("偏移"), LVCFMT_LEFT, m_rect.right / 5);
    m_lstData.InsertColumn(0, _T("成员"), LVCFMT_LEFT, m_rect.right / 5);

    //添加g_pOptional
    int nBegin = m_nOffsetDos;
    void* pBegin = (void*)m_pDOS;
    for (int i = 0; i < 31; i++)
    {
        //添加成员
        m_lstData.InsertItem(i, g_pDosHeader[i].pInfo);

        //添加偏移
        char szTemp[100] = { 0 };
        wsprintf(szTemp, "0X%p", nBegin);
        m_lstData.SetItemText(i, 1, szTemp);
        nBegin = nBegin + g_pDosHeader[i].nSize;

        //添加大小,值
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
        
        //添加内涵
        m_lstData.SetItemText(i, 4, g_pDosHeader[i].pDescription);
    }
}

void CPEresolverDlg::ShowPESign()//PE标识
{
    m_nCurrentSelect = EN_NT;
    //设置list控件标题
    //删除原内容
    ClearListControl(m_lstData);
    

    //添加新标题
    m_lstData.InsertColumn(0, _T("值"), LVCFMT_LEFT, m_rect.right / 4);
    m_lstData.InsertColumn(0, _T("大小"), LVCFMT_LEFT, m_rect.right / 4);
    m_lstData.InsertColumn(0, _T("偏移"), LVCFMT_LEFT, m_rect.right / 4);
    m_lstData.InsertColumn(0, _T("成员"), LVCFMT_LEFT, m_rect.right / 4);

    //添加成员
    m_lstData.InsertItem(0, "Signature");
    //添加偏移
    char szTemp[16] = { 0 };
    wsprintf(szTemp, "0X%p", m_nOffsetNT);
    m_lstData.SetItemText(0, 1, szTemp);
    //添加大小
    m_lstData.SetItemText(0, 2, "DWORD");
    //添加值
    wsprintf(szTemp, "%08X", *(DWORD*)m_pNT);
    m_lstData.SetItemText(0, 3, szTemp);


}

void CPEresolverDlg::ShowFileHeader()//标准PE头
{
    m_nCurrentSelect = EN_FILE;
    //设置list控件标题
    //删除原内容
    ClearListControl(m_lstData);

    //添加新标题
    m_lstData.InsertColumn(0, _T("内涵"), LVCFMT_LEFT, m_rect.right * 2);
    m_lstData.InsertColumn(0, _T("值"), LVCFMT_LEFT, m_rect.right / 5);
    m_lstData.InsertColumn(0, _T("大小"), LVCFMT_LEFT, m_rect.right / 5);
    m_lstData.InsertColumn(0, _T("偏移"), LVCFMT_LEFT, m_rect.right / 5);
    m_lstData.InsertColumn(0, _T("成员"), LVCFMT_LEFT, m_rect.right / 5);

    //添加成员
    m_lstData.InsertItem(0, "Machine");
    m_lstData.InsertItem(1, "NumberOfSections");
    m_lstData.InsertItem(2, "TimeDateStamp");
    m_lstData.InsertItem(3, "PointerToSymbolTable");
    m_lstData.InsertItem(4, "NumberOfSymbols");
    m_lstData.InsertItem(5, "SizeOfOptionalHeader");
    m_lstData.InsertItem(6, "Characteristics");

    //添加偏移，大小，值
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
            //插入大小
            m_lstData.SetItemText(i, 2, "DWORD");

            //插入值
            wsprintf(szTemp, "%08X", *(DWORD*)pFile);
            m_lstData.SetItemText(i, 3, szTemp);

            pFile = (void*)((int)pFile + 4);
            nOffset = nOffset + 4;
        }
        else
        {
            //插入大小
            m_lstData.SetItemText(i, 2, "WORD");

            //插入值
            wsprintf(szTemp, "%04X", *(WORD*)pFile);
            m_lstData.SetItemText(i, 3, szTemp);

            pFile = (void*)((int)pFile + 2);
            nOffset = nOffset + 2;
        }
    }

    //插入内涵
    //插入机器型号
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
    //插入节的数量
    m_lstData.SetItemText(1, 4, "节的数量");
    //插入编译时间
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
    //插入 拓展PE头大小
    m_lstData.SetItemText(5, 4, "拓展PE头大小");
    //插入文件属性
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
    //设置list控件标题
    //删除原内容
    ClearListControl(m_lstData);

    //添加新标题
    m_lstData.InsertColumn(0, _T("内涵"), LVCFMT_LEFT, m_rect.right * 2 / 5);
    m_lstData.InsertColumn(0, _T("值"), LVCFMT_LEFT, m_rect.right / 5);
    m_lstData.InsertColumn(0, _T("大小"), LVCFMT_LEFT, m_rect.right / 5);
    m_lstData.InsertColumn(0, _T("偏移"), LVCFMT_LEFT, m_rect.right / 5);
    m_lstData.InsertColumn(0, _T("成员"), LVCFMT_LEFT, m_rect.right / 5);

    //添加g_pOptional
    int nBegin = m_nOffsetOptional;
    void* pBegin = (void*)m_pOptional;
    for (int i = 0; i < 30 + m_pOptional->NumberOfRvaAndSizes; i++)
    {
        //添加成员
        m_lstData.InsertItem(i, g_pOptional[i].pInfo);

        //添加偏移
        char szTemp[36] = { 0 };
        wsprintf(szTemp, "0X%p", nBegin);
        m_lstData.SetItemText(i, 1, szTemp);
        nBegin = nBegin + g_pOptional[i].nSize;

        //添加大小,值
        if (g_pOptional[i].nSize == 1)
        {
            //添加大小
            m_lstData.SetItemText(i, 2, "BYTE");

            //添加值
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

    //添加内涵
    m_lstData.SetItemText(6, 4, "程序入口点");
    m_lstData.SetItemText(9, 4, "内存镜像基址");
    m_lstData.SetItemText(10, 4, "内存对齐");
    m_lstData.SetItemText(11, 4, "文件对齐");
    m_lstData.SetItemText(19, 4, "内存中整个PE文件的映射尺寸");
    m_lstData.SetItemText(20, 4, "头+节表对齐后的大小");
    m_lstData.SetItemText(21, 4, "校验和");
    m_lstData.SetItemText(23, 4, "文件特性");
    m_lstData.SetItemText(24, 4, "栈保留大小");
    m_lstData.SetItemText(25, 4, "栈提交大小");
    m_lstData.SetItemText(26, 4, "堆保留大小");
    m_lstData.SetItemText(27, 4, "堆提交大小");
    m_lstData.SetItemText(29, 4, "目录项数目");

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
        AfxMessageBox("当前文件没有节表");
        m_nCurrentSelect = EN_NONE;
        return;
    }

    m_nCurrentSelect = EN_SECTION;
    //设置list控件标题
    //删除原内容
    ClearListControl(m_lstData);

    //添加新标题
    m_lstData.InsertColumn(0, _T("内涵"), LVCFMT_LEFT, m_rect.right  / 3);
    m_lstData.InsertColumn(0, _T("值"), LVCFMT_LEFT, m_rect.right / 5);
    m_lstData.InsertColumn(0, _T("大小"), LVCFMT_LEFT, m_rect.right / 9);
    m_lstData.InsertColumn(0, _T("偏移"), LVCFMT_LEFT, m_rect.right / 6);
    m_lstData.InsertColumn(0, _T("成员"), LVCFMT_LEFT, m_rect.right / 4);
    m_lstData.InsertColumn(0, _T("名称"), LVCFMT_LEFT, m_rect.right / 10);
    //添加内容
    //g_pSectionTable
    void* pAddr = (void*)m_pSection;
    int nBegin = m_nOffsetSection;
    int nRow = 0;
    for (int i = 0; i < m_nSectionTableNum; i++)
    {
        for (int j = 0; j < 10; j++)
        {
            //添加名称
            if (j == 0)
            {
                m_lstData.InsertItem(nRow, (char*)m_pSection[i].Name);
            }
            else
            {
                m_lstData.InsertItem(nRow, "");
            }
            

            //添加成员
            m_lstData.SetItemText(nRow, 1, g_pSectionTable[j].pInfo);

            //添加偏移
            char szTemp[36] = { 0 };
            wsprintf(szTemp, "0X%p", nBegin);
            m_lstData.SetItemText(nRow, 2, szTemp);
            nBegin = nBegin + g_pSectionTable[j].nSize;

            //添加大小
            if (g_pSectionTable[j].nSize == 2)
            {
                m_lstData.SetItemText(nRow, 3, "WORD");

                //添加值
                wsprintf(szTemp, "%04X", *(WORD*)pAddr);
                m_lstData.SetItemText(nRow, 4, szTemp);
                pAddr = (void*)((int)pAddr + 2);
            }
            else if (g_pSectionTable[j].nSize == 4)
            {
                m_lstData.SetItemText(nRow, 3, "DWORD");

                //添加值
                wsprintf(szTemp, "%08X", *(DWORD*)pAddr);
                m_lstData.SetItemText(nRow, 4, szTemp);
                pAddr = (void*)((int)pAddr + 4);
            }
            else
            {
                m_lstData.SetItemText(nRow, 3, "IMAGE_SIZEOF_SHORT_NAME");

                //添加值
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

            //添加内涵
            if (j == 0)
            {
                m_lstData.SetItemText(nRow, 5, "节名称");
            }
            else if (j == 1)
            {
                m_lstData.SetItemText(nRow, 5, "虚拟大小");
            }
            else if (j == 2)
            {
                m_lstData.SetItemText(nRow, 5, "虚拟偏移地址");
            }
            else if (j == 3)
            {
                m_lstData.SetItemText(nRow, 5, "Raw尺寸");
            }
            else if (j == 4)
            {
                m_lstData.SetItemText(nRow, 5, "Raw地址");
            }
            else if (j == 9)
            {
                char szCharact[0X1000] = { "节的属性：" };

                int nCharact = m_pSection[i].Characteristics & 0x10000000;
                if (nCharact != 0)
                {
                    strcat_s(szCharact, 0X1000, "共享");
                    strcat_s(szCharact, 0X1000, "  ");
                }

                nCharact = m_pSection[i].Characteristics & 0x20000000;
                if (nCharact != 0)
                {
                    strcat_s(szCharact, 0X1000, "可执行");
                    strcat_s(szCharact, 0X1000, "  ");
                }

                nCharact = m_pSection[i].Characteristics & 0x40000000;
                if (nCharact != 0)
                {
                    strcat_s(szCharact, 0X1000, "可读");
                    strcat_s(szCharact, 0X1000, "  ");
                }

                nCharact = m_pSection[i].Characteristics & 0x80000000;
                if (nCharact != 0)
                {
                    strcat_s(szCharact, 0X1000, "可写");
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
//    //隐藏列表控件
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
    //判断
    if (m_nOffsetImport == 0 )
    {
        AfxMessageBox("该文件没有导入表");
        m_nCurrentSelect = EN_NONE;
        return;
    }

    m_nCurrentSelect = EN_IMPORTDLL;
    //设置list控件标题
    //删除原内容
    ClearListControl(m_lstImportDLL);

    //添加新标题
    m_lstImportDLL.InsertColumn(0, _T("模块名"), LVCFMT_LEFT, m_rectImportDll.right / 4);
    m_lstImportDLL.InsertColumn(1, _T("名称RVA"), LVCFMT_LEFT, m_rectImportDll.right / 8);
    m_lstImportDLL.InsertColumn(2, _T("导入"), LVCFMT_LEFT, m_rectImportDll.right / 8);
    m_lstImportDLL.InsertColumn(3, _T("INT(RVA)"), LVCFMT_LEFT, m_rectImportDll.right / 8);
    m_lstImportDLL.InsertColumn(4, _T("时间日期戳"), LVCFMT_LEFT, m_rectImportDll.right / 8);
    m_lstImportDLL.InsertColumn(5, _T("转发链"), LVCFMT_LEFT, m_rectImportDll.right / 8);
    m_lstImportDLL.InsertColumn(6, _T("IAT(RVA)"), LVCFMT_LEFT, m_rectImportDll.right / 8);

    //插入尺寸
    m_lstImportDLL.InsertItem(0, "char*");
    m_lstImportDLL.SetItemText(0, 1, "DWORD");
    m_lstImportDLL.SetItemText(0, 2, "DWORD");
    m_lstImportDLL.SetItemText(0, 3, "DWORD");
    m_lstImportDLL.SetItemText(0, 4, "DWORD");
    m_lstImportDLL.SetItemText(0, 5, "DWORD");
    m_lstImportDLL.SetItemText(0, 6, "DWORD");

    //添加内容
    IMAGE_IMPORT_DESCRIPTOR strCmp = { 0 };
    char szTemp[100] = { 0 };
    int nNum = 0;
    while (true)
    {
        //插入DLL名
        char* pDllName = (char*)RVAtoMapAddr(m_pImport[nNum].Name);
        m_lstImportDLL.InsertItem(nNum + 1, pDllName);
        //插入DLL名RVA
        wsprintf(szTemp, "%08X", m_pImport[nNum].Name);
        m_lstImportDLL.SetItemText(nNum + 1, 1, szTemp);

        //插入导入数目
        //1. 找到IAT的内存映射地址
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
        //添加IAT
        wsprintf(szTemp, "%08X", dwIATRVA);
        m_lstImportDLL.SetItemText(nNum + 1, 6, szTemp);
        //添加导入数目
        wsprintf(szTemp, "%d", nCount);
        m_lstImportDLL.SetItemText(nNum + 1, 2, szTemp);
        //插入INT
        wsprintf(szTemp, "%08X", m_pImport[nNum].OriginalFirstThunk);
        m_lstImportDLL.SetItemText(nNum + 1, 3, szTemp);
        //插入时间戳
        wsprintf(szTemp, "%08X", m_pImport[nNum].TimeDateStamp);
        m_lstImportDLL.SetItemText(nNum + 1, 4, szTemp);
        //插入转发链
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
    //设置list控件标题
    //删除原内容
    ClearListControl(m_lstImportFunc);

    //添加新标题
    m_lstImportFunc.InsertColumn(0, _T("BY_NAME(offset)"), LVCFMT_LEFT, m_rectImportDll.right / 5);
    m_lstImportFunc.InsertColumn(1, _T("函数名"), LVCFMT_LEFT, m_rectImportDll.right / 5);
    m_lstImportFunc.InsertColumn(2, _T("Hint"), LVCFMT_LEFT, m_rectImportDll.right / 5);
    m_lstImportFunc.InsertColumn(3, _T("IAT(offset)"), LVCFMT_LEFT, m_rectImportDll.right / 5);
    m_lstImportFunc.InsertColumn(4, _T("IAT"), LVCFMT_LEFT, m_rectImportDll.right / 5);

    //获取点击的信息

    //获取INT表
    CString cs;
    cs = m_lstImportDLL.GetItemText(m_nRow, 3);
    DWORD nRVAINT = strtoul(cs.GetBuffer(), NULL, 16);
    //转化
    DWORD nOffsetINT = RVAtoOFFSET(nRVAINT);
    DWORD* pMapINT = (DWORD*)RVAtoMapAddr(nRVAINT);

    //获取IAT表
    cs = m_lstImportDLL.GetItemText(m_nRow, 6);
    DWORD nRVAIAT = strtoul(cs.GetBuffer(), NULL, 16);
    //转化
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
            //序号
            WORD* pData = (WORD*)pMapINT;
            pData++;

        }
        else
        {
            if (*pMapINT != 0)
            {
                //名称RVA
                DWORD dwBYNAMEOffset = RVAtoOFFSET(*pMapINT);

                void* pBYNAME = RVAtoMapAddr(*pMapINT);

                //插入INT偏移
                wsprintf(szTemp, "0X%08X", dwBYNAMEOffset);
                m_lstImportFunc.InsertItem(nNum, szTemp);

                //插入函数名
                m_lstImportFunc.SetItemText(nNum, 1, (char*)((int)pBYNAME + 2));

                //插入Hint
                wsprintf(szTemp, "%04X", *(WORD*)pBYNAME);
                m_lstImportFunc.SetItemText(nNum, 2, szTemp);

                //插入IAT(offset)
                //DWORD dwIATOffset = RVAtoOFFSET(*pMapIAT);
                wsprintf(szTemp, "0X%08X", nOffsetIAT);
                m_lstImportFunc.SetItemText(nNum, 3, szTemp);

                //插入IAT
                wsprintf(szTemp, "%08X", *pMapIAT);
                m_lstImportFunc.SetItemText(nNum, 4, szTemp);
            }
            else//INT为0，获取IAT信息
            {
                //名称RVA
                DWORD dwBYNAMEOffset = RVAtoOFFSET(*pMapIAT);

                void* pBYNAME = RVAtoMapAddr(*pMapIAT);

                //插入INT偏移
                wsprintf(szTemp, "0X%08X", dwBYNAMEOffset);
                m_lstImportFunc.InsertItem(nNum, szTemp);

                //插入函数名
                m_lstImportFunc.SetItemText(nNum, 1, (char*)((int)pBYNAME + 2));

                //插入Hint
                wsprintf(szTemp, "%04X", *(WORD*)pBYNAME);
                m_lstImportFunc.SetItemText(nNum, 2, szTemp);

                //插入IAT(offset)
                //DWORD dwIATOffset = RVAtoOFFSET(*pMapIAT);
                wsprintf(szTemp, "0X%08X", nOffsetIAT);
                m_lstImportFunc.SetItemText(nNum, 3, szTemp);

                //插入IAT
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
    //判断
    if (m_nOffsetExport == 0)
    {
        AfxMessageBox("该文件没有导出表");
        m_nCurrentSelect = EN_NONE;
        return;
    }

    m_nCurrentSelect = EN_EXPORTDLL;
    //设置list控件标题
    //删除原内容
    ClearListControl(m_lstExportDll);

    //添加新标题
    m_lstExportDll.InsertColumn(0, _T("成员"), LVCFMT_LEFT, m_rectExportDll.right / 5);
    m_lstExportDll.InsertColumn(1, _T("文件偏移"), LVCFMT_LEFT, m_rectExportDll.right / 5);
    m_lstExportDll.InsertColumn(2, _T("大小"), LVCFMT_LEFT, m_rectExportDll.right / 5);
    m_lstExportDll.InsertColumn(3, _T("值"), LVCFMT_LEFT, m_rectExportDll.right / 5);
    m_lstExportDll.InsertColumn(4, _T("内涵"), LVCFMT_LEFT, m_rectExportDll.right / 5);

    int nOffset = m_nOffsetExport;
    char szTemp[100] = { 0 };
    void* pAddr = (void*)m_pExport;
    for (int i = 0; i < 11; i++)
    {
        //插入成员
        m_lstExportDll.InsertItem(i, g_pExportTable[i].pInfo);

        //插入文件偏移
        wsprintf(szTemp, "0X%08X", nOffset);
        m_lstExportDll.SetItemText(i, 1, szTemp);
        nOffset += g_pExportTable[i].nSize;

        //插入大小
        //插入值
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

        //插入描述
        if (i == 1)
        {
            //插入编译时间
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

    //添加新标题
    m_lstExportFunc.InsertColumn(0, _T("导出序号"), LVCFMT_LEFT, m_rectExportFunc.right / 5);
    m_lstExportFunc.InsertColumn(1, _T("函数地址(offset)"), LVCFMT_LEFT, m_rectExportFunc.right / 5);
    m_lstExportFunc.InsertColumn(2, _T("名称序号"), LVCFMT_LEFT, m_rectExportFunc.right / 5);
    m_lstExportFunc.InsertColumn(3, _T("函数名称(offset)"), LVCFMT_LEFT, m_rectExportFunc.right / 5);
    m_lstExportFunc.InsertColumn(4, _T("函数名"), LVCFMT_LEFT, m_rectExportFunc.right / 5);

    //因为地址表中可能有空项，所以索引单独用一个变量，不用i
    int nIndex = 0;
    char szTemp[100] = { 0 };
    for (int i = 0; i < m_dwNumOfFunctions; i++)
    {
        if (pAddrOfFunc[i] == 0)//当前项为空项
        {
            continue;
        }

        //插入导出序号
        wsprintf(szTemp, "%08X", (DWORD)(nIndex + m_dwBase));
        m_lstExportFunc.InsertItem(nIndex, szTemp);

        //插入函数地址(offset)
        wsprintf(szTemp, "%08X", RVAtoOFFSET(pAddrOfFunc[i]));
        m_lstExportFunc.SetItemText(nIndex, 1, szTemp);

        //插入名称序号
        bool bFlag = false;
        for (int j = 0; j < m_dwNumOfNames; j++)
        {
            if (pAddrOfOrdinal[j] == i)
            {
                bFlag = true;

                //插入名称序号
                wsprintf(szTemp, "%08X", j);
                m_lstExportFunc.SetItemText(nIndex, 2, szTemp);

                //插入函数名称(offset)
                wsprintf(szTemp, "%08X", RVAtoOFFSET(pAddrOfNames[j]));
                m_lstExportFunc.SetItemText(nIndex, 3, szTemp);

                //插入函数名
                m_lstExportFunc.SetItemText(nIndex, 4, (LPCTSTR)RVAtoMapAddr(pAddrOfNames[j]));
                break;
            }
        }
        if (!bFlag)
        {
            //插入名称序号
            m_lstExportFunc.SetItemText(nIndex, 2, "N/A");

            //插入函数名称(offset)
            m_lstExportFunc.SetItemText(nIndex, 3, "N/A");

            //插入函数名
            m_lstExportFunc.SetItemText(nIndex, 4, "N/A");
        }

        nIndex++;
    }

}

void CPEresolverDlg::ShowRelocation()
{
    //判断
    if (m_nOffsetRelocation == 0)
    {
        AfxMessageBox("该文件没有重定位表");
        m_nCurrentSelect = EN_NONE;
        return;
    }

    m_nCurrentSelect = EN_RELOCATION;
    //设置list控件标题
    //删除原内容
    ClearListControl(m_lstData);

    //添加新标题
    m_lstData.InsertColumn(0, _T("文件偏移"), LVCFMT_LEFT, m_rectExportDll.right / 3);
    m_lstData.InsertColumn(1, _T("分页"), LVCFMT_LEFT, m_rectExportDll.right / 3);
    m_lstData.InsertColumn(2, _T("地址"), LVCFMT_LEFT, m_rectExportDll.right / 3);

    //插入内容
    char szTemp[100] = { 0 };
    int nNum = 0;
    int nCurrentSize = 0;//当前操作完成的重定位表尺寸
    IMAGE_BASE_RELOCATION* pCurrentReloc = m_pRelocation;
    WORD* pCurrentOffset = (WORD*)((int)pCurrentReloc + sizeof(IMAGE_BASE_RELOCATION));
    int nOffset = m_nOffsetRelocation;
    while (nCurrentSize < m_nSizeOfRelocation)
    {
        int nSize = pCurrentReloc->SizeOfBlock;//当前操作的分页的重定位表尺寸
        int nCount = (nSize - sizeof(IMAGE_BASE_RELOCATION)) / 2;//当前表的重定位个数
        nOffset = nOffset + sizeof(IMAGE_BASE_RELOCATION);

        for (int i = 0; i < nCount; i++)
        {
            //插入文件偏移
            wsprintf(szTemp, "0X%08X", nOffset);
            m_lstData.InsertItem(nNum, szTemp);
            
            //插入分页
            wsprintf(szTemp, "0X%08X", pCurrentReloc->VirtualAddress);
            m_lstData.SetItemText(nNum, 1, szTemp);

            //插入地址
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
    //判断
    if (m_nOffsetTls == 0)
    {
        AfxMessageBox("该文件没有TLS表");
        m_nCurrentSelect = EN_NONE;
        return;
    }

    m_nCurrentSelect = EN_TLS;
    //设置list控件标题
    //删除原内容
    ClearListControl(m_lstData);

    //添加新标题
    m_lstData.InsertColumn(0, _T("成员"), LVCFMT_LEFT, m_rect.right / 5);
    m_lstData.InsertColumn(1, _T("偏移"), LVCFMT_LEFT, m_rect.right / 5);
    m_lstData.InsertColumn(2, _T("大小"), LVCFMT_LEFT, m_rect.right / 5);
    m_lstData.InsertColumn(3, _T("值（VA）"), LVCFMT_LEFT, m_rect.right / 5);
    m_lstData.InsertColumn(4, _T("内涵"), LVCFMT_LEFT, m_rect.right  / 5);

    //添加项
    //成员
    m_lstData.InsertItem(0, "StartAddressOfRawData");
    m_lstData.InsertItem(1, "EndAddressOfRawData");
    m_lstData.InsertItem(2, "AddressOfIndex");
    m_lstData.InsertItem(3, "AddressOfCallBacks");
    m_lstData.InsertItem(4, "SizeOfZeroFill");
    m_lstData.InsertItem(5, "DUMMYUNIONNAME");
    
    //偏移
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

    //大小
    m_lstData.SetItemText(0, 2, "DWORD");
    m_lstData.SetItemText(1, 2, "DWORD");
    m_lstData.SetItemText(2, 2, "DWORD");
    m_lstData.SetItemText(3, 2, "DWORD");
    m_lstData.SetItemText(4, 2, "DWORD");
    m_lstData.SetItemText(5, 2, "DWORD");

    //值
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

    //内涵
    m_lstData.SetItemText(0, 4, "TLs在内存中的起始VA");
    m_lstData.SetItemText(1, 4, "TLs在内存中的结束VA");
    m_lstData.SetItemText(2, 4, "存储TLS索引的位置VA");
    m_lstData.SetItemText(3, 4, "回调函数指针数组VA");

}

void CPEresolverDlg::ShowResource()
{
    //判断
    if (m_nOffsetResource == 0)
    {
        AfxMessageBox("该文件没有资源表");
        m_nCurrentSelect = EN_NONE;
        return;
    }

    //清理树控件
    m_treeResource.DeleteAllItems();

    m_nCurrentSelect = EN_RESOURCE;
    //设置list控件标题
    //删除原内容
    //ClearListControl(m_lstResource);

    DWORD dwFirst = m_nOffsetResource;
    ReadResource(NULL, dwFirst, EN_DERECTY, 0);
}

//最高位为1-》资源数据。 最高位为2-》资源目录项。 最高位为4-》资源目录---以此为标准将地址转换后放进InsertItem的参数里
void CPEresolverDlg::ReadResource(HTREEITEM hParent, DWORD dwNext, int nType, int nNum)
{
    //计算文件偏移
    DWORD dwOffset = dwNext & 0X0fffffff;

    //计算内存偏移
    DWORD dwMemAddr = dwOffset + (DWORD)m_pView;

    switch (nType)
    {
    case EN_DERECTY://资源目录
    {
        LPARAM lParam = dwMemAddr | 0X40000000;
        UINT uMask = TVIF_PARAM | TVIF_TEXT;
        HTREEITEM  hResorce = m_treeResource.InsertItem(uMask, "资源目录", NULL, NULL, NULL, NULL, lParam, hParent, TVI_LAST);

        IMAGE_RESOURCE_DIRECTORY* pRes = (IMAGE_RESOURCE_DIRECTORY*)dwMemAddr;
        int nCount = pRes->NumberOfIdEntries + pRes->NumberOfNamedEntries;

        for (int i = 0; i < nCount; i++)
        {
            DWORD dwNextOffset = dwNext + sizeof(IMAGE_RESOURCE_DIRECTORY) + i * sizeof(IMAGE_RESOURCE_DIRECTORY_ENTRY);
            ReadResource(hResorce, dwNextOffset, EN_ENTRY,i + 1);
        }

        break;
    }
    case EN_ENTRY://资源目录项
    {
        IMAGE_RESOURCE_DIRECTORY_ENTRY* pRes = (IMAGE_RESOURCE_DIRECTORY_ENTRY*)dwMemAddr;
        DWORD dwName = pRes->Name;
        DWORD dwOffsetToData = pRes->OffsetToData;

        //设计标题
        char szTitle[100] = { 0 };
        if (dwName >= 0X80000000)//name
        {
            //计算name的内存地址
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

            wsprintf(szTitle, "资源目录项 %d, 名称 ", nNum);
            strcat_s(szTitle, 100, szName);
        }
        else//ID
        {
            wsprintf(szTitle, "资源目录项 %d, ID %d ", nNum, pRes->Id);
        }

        LPARAM lpParam = dwMemAddr | 0X20000000;
        UINT uMask = TVIF_PARAM | TVIF_TEXT;
        HTREEITEM  hResorce = m_treeResource.InsertItem(uMask, szTitle, NULL, NULL, NULL, NULL, lpParam, hParent, TVI_LAST);

        if (dwOffsetToData >= 0X80000000)//资源目录
        {
            dwOffsetToData = dwOffsetToData & 0X0fffffff;
            dwOffsetToData += m_nOffsetResource;

            ReadResource(hResorce, dwOffsetToData, EN_DERECTY, 0);

        }
        else//数据
        {
            dwOffsetToData += m_nOffsetResource;
            ReadResource(hResorce, dwOffsetToData, EN_DATA, 0);
        }

        break;
    }
    case EN_DATA://数据
    {
        LPARAM lpParam = dwMemAddr | 0X10000000;
        UINT uMask = TVIF_PARAM | TVIF_TEXT;
        HTREEITEM  hData = m_treeResource.InsertItem(uMask, "资源数据项", NULL, NULL, NULL, NULL, lpParam, hParent, TVI_LAST);

        return;

        break;
    }
    default: break;
    }

}

void CPEresolverDlg::OperateChange()//隐藏其他控件，显示当前控件
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
        //隐藏所有，只留下listData控件
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

        //显示
        m_lstData.ShowWindow(SW_SHOW);

        break;
    }
    case EN_ADDRCHANGE:
    {
        //隐藏所有，只留下地址转换相关控件
        m_lstData.ShowWindow(SW_HIDE);

        m_lstImportDLL.ShowWindow(SW_HIDE);
        m_lstImportFunc.ShowWindow(SW_HIDE);

        m_lstExportDll.ShowWindow(SW_HIDE);
        m_lstExportFunc.ShowWindow(SW_HIDE);

        m_treeResource.ShowWindow(SW_HIDE);
        m_lstResource.ShowWindow(SW_HIDE);

        //显示
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
        //隐藏所有，只留下导入表相关控件
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

        //显示
        m_lstImportDLL.ShowWindow(SW_SHOW);
        
        break;
    }
    case EN_IMPORTFUNC:
    {
        //隐藏所有，只留下导入表相关控件
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

        //显示
        m_lstImportDLL.ShowWindow(SW_SHOW);
        m_lstImportFunc.ShowWindow(SW_SHOW);
        break;
    }
    case EN_EXPORTDLL:
    {
        //隐藏所有，只留下导出表相关控件
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
        //隐藏所有，只留下导出表相关控件
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
    //判断是否在节区
    if (dwRVA < m_pSection[0].VirtualAddress)
    {
        return dwRVA;
    }

    //判断是否有节表
    if (m_nSectionTableNum == 0)
    {
        return dwRVA;
    }

    int nBegin = 0; //当前遍历节的起始
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


// CPEresolverDlg 消息处理程序

BOOL CPEresolverDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
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

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
    //获取PE头list的客户区,方便调节每一列的宽度，设置风格
    m_lstData.GetClientRect(&m_rect); 
    m_lstData.SetExtendedStyle(LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);
    m_lstData.ShowWindow(SW_HIDE);

    //获取导入表list的客户区,方便调节每一列的宽度
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

    //隐藏edit 
    m_edtInput.ShowWindow(SW_HIDE);

    //隐藏地址转换相关控件
    m_changeStaVA.ShowWindow(SW_HIDE);
    m_changeStaRVA.ShowWindow(SW_HIDE);
    m_changeStaOffset.ShowWindow(SW_HIDE);
    m_changeEdtVA.ShowWindow(SW_HIDE);
    m_changeEdtRVA.ShowWindow(SW_HIDE);
    m_changeEdtOffset.ShowWindow(SW_HIDE);
    m_changeBtnOK.ShowWindow(SW_HIDE);

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
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

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CPEresolverDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CPEresolverDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CPEresolverDlg::OnBnClickedOk()//确定
{
    m_nCurrentSelect = EN_NONE;
    //清理树控件
    m_treeFile.DeleteAllItems();

    //若有文件打开，先释放
    if (m_pView)
    {
        //1. 取消映射视图
        UnmapViewOfFile(m_pView);
        //2. 关闭映射对象
        CloseHandle(m_hFileMap);
        //3. 关闭文件
        CloseHandle(m_hFile);
    }

    //获取文件名，打开文件，创建文件映射，获取文件大小
    if (!CreateFileMap())
    {
        return;
    }

    //判断是否为PE
    if (m_pDOS->e_magic != 0x5A4D)
    {
        AfxMessageBox("此文件不是一个PE文件");
        return;
    }
    if (m_pNT->Signature != 0X4550)
    {
        AfxMessageBox("此文件不是一个PE文件");
        return;
    }
    
    //初始化树控件
    if (!InitTreeControl())
    {
        return;
    }

    //计算偏移
    m_nOffsetNT = m_pDOS->e_lfanew;
    m_nOffsetFile = m_nOffsetNT + 4;
    m_nOffsetOptional = m_nOffsetFile + sizeof(IMAGE_FILE_HEADER);
    m_nOffsetSection = m_nOffsetOptional + m_pFILE->SizeOfOptionalHeader;
    m_nOffsetFirstSection = m_pSection->PointerToRawData;

    //目录项个数
    m_nNumberOfData = m_pOptional->NumberOfRvaAndSizes;

    //计算导入表文件偏移地址
    if (m_nNumberOfData < 2)
    {
        m_nOffsetImport = 0;
    }
    else
    {
        int nRVAImport = m_pOptional->DataDirectory[1].VirtualAddress;//导入表的RVA
        m_nOffsetImport = RVAtoOFFSET(nRVAImport);
    }
    
    if (m_nNumberOfData >= 1)
    {
        int nRVAExport = m_pOptional->DataDirectory[0].VirtualAddress;//导出表的RVA
        m_nOffsetExport = RVAtoOFFSET(nRVAExport);
    }
    else
    {
        m_nOffsetExport = 0;
    }

    //计算映射中导入表的地址
    if (m_nOffsetImport != 0)
    {
        m_pImport = (IMAGE_IMPORT_DESCRIPTOR*)((int)m_pView + m_nOffsetImport);
    }
    else
    {
        m_pImport = 0;
    }

    //计算映射中导出表的地址
    if (m_nOffsetExport != 0)
    {
        m_pExport = (IMAGE_EXPORT_DIRECTORY*)((int)m_pView + m_nOffsetExport);
    }
    else
    {
        m_pExport = 0;
    }

    //计算重定位表
    if (m_nNumberOfData >= 5)
    {
        int nRVAReloc = m_pOptional->DataDirectory[5].VirtualAddress;//重定位表表的RVA
        m_nOffsetRelocation = RVAtoOFFSET(nRVAReloc);
        m_nSizeOfRelocation = m_pOptional->DataDirectory[5].Size;

        m_pRelocation = (IMAGE_BASE_RELOCATION*)((int)m_pView + m_nOffsetRelocation);
    }
    else
    {
        m_nOffsetRelocation = 0;
        m_pRelocation = 0;
    }
    
    //计算映射中TLs表的地址
    if (m_nNumberOfData < 9)
    {
        m_pTLs = nullptr;
        m_nOffsetTls = 0;
    }
    else
    {
        int nTLs = m_pOptional->DataDirectory[9].VirtualAddress;//TLs表的RVA
        m_nOffsetTls = RVAtoOFFSET(nTLs);

        m_pTLs = (IMAGE_TLS_DIRECTORY32*)((int)m_pView + m_nOffsetTls);
    }
    
    //计算映射中资源表的地址
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


void CPEresolverDlg::OnTvnSelchangedFile(NMHDR *pNMHDR, LRESULT *pResult)//点击树控件的项
{
    LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
    // TODO: 在此添加控件通知处理程序代码
    *pResult = 0;

    HTREEITEM hItem = m_treeFile.GetSelectedItem();
    CString csItem = m_treeFile.GetItemText(hItem);


    if (strcmp(csItem.GetBuffer(), "_IMAGE_DOS_HEADER") == 0)//DOS头
    {
        ShowDosHeader();
    }
    else if (strcmp(csItem.GetBuffer(), "_IMAGE_NT_HEADERS") == 0)//NT
    {
        //ShowNTHeader();
    }
    else if (strcmp(csItem.GetBuffer(), "Signature") == 0)//PE标识
    {
        ShowPESign();
    }
    else if (strcmp(csItem.GetBuffer(), "IMAGE_FILE_HEADER") == 0)//PE头
    {
        ShowFileHeader();
    }
    else if (strcmp(csItem.GetBuffer(), "IMAGE_OPTIONAL_HEADER32") == 0)//可选PE头
    {
        ShowOptionalHeader();
    }
    else if (strcmp(csItem.GetBuffer(), "节表") == 0)//节表
    {
        ShowSectionTable();
    }
    else if (strcmp(csItem.GetBuffer(), "地址转换") == 0)//地址转换
    {
        m_nCurrentSelect = EN_ADDRCHANGE;
    }
    else if (strcmp(csItem.GetBuffer(), "导入目录") == 0)//导入目录
    {
        ShowImport();
    }
    else if (strcmp(csItem.GetBuffer(), "导出目录") == 0)//导出目录
    {
        ShowExport();
    }
    else if (strcmp(csItem.GetBuffer(), "重定位表") == 0)//导出目录
    {
        ShowRelocation();
    }
    else if (strcmp(csItem.GetBuffer(), "TLs表") == 0)//导出目录
    {
        ShowTLs();
    }
    else if (strcmp(csItem.GetBuffer(), "资源表") == 0)//资源表
    {
        ShowResource();
    }

    OperateChange(); 
}


void CPEresolverDlg::OnNMDblclkData(NMHDR *pNMHDR, LRESULT *pResult)//双击列表控件
{
    LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
    // 判断是否点击的可编辑列
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
        //获取行列
        m_nRow = pNMItemActivate->iItem;//行
        m_nColumn = pNMItemActivate->iSubItem;//列

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
        //获取行列
        m_nRow = pNMItemActivate->iItem;//行
        m_nColumn = pNMItemActivate->iSubItem;//列

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

    //获取子项的矩形
    CRect rc;
    m_lstData.GetSubItemRect(m_nRow, m_nColumn, LVIR_LABEL, rc);
    //获取树控件的矩形
    CRect treeRc;
    m_treeFile.GetWindowRect(&treeRc);

    //转换矩形
    rc.left += treeRc.right - treeRc.left + 17;
    rc.right += treeRc.right - treeRc.left + 17;
    rc.top += 5;
    rc.bottom += 20;

    //获取子项的值
    CString cs = m_lstData.GetItemText(m_nRow, m_nColumn);

    m_edtInput.ShowWindow(SW_SHOW);

    m_edtInput.SetWindowPos(&CWnd::wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
    m_edtInput.MoveWindow(&rc);
    m_edtInput.SetFocus();
    m_edtInput.SetWindowText(cs);
    ::SetFocus(m_edtInput.GetSafeHwnd());//设置焦点
    m_edtInput.SetSel(-1);

    *pResult = 0;
}


void CPEresolverDlg::OnEnKillfocusEdit1()//编辑框失去输入焦点
{
    //获取编辑框的值
    CString cs;
    GetDlgItemText(EDT_INPUT, cs);
    int nTemp = 0;
    if (cs.GetLength() <= 8)
    {
        nTemp = strtoul(cs.GetBuffer(), NULL, 16);
    }
    
    //填入数据
    //1. 找到数据块的起始地址
    //2. 计算偏移
    //3. 赋值
    //4. 改列表控件的项
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
        else//8字节结构体 此处是存数值
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
            //此处是存字符串
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

    //将新数据写入控件中
    m_lstData.SetItemText(m_nRow, m_nColumn, cs.GetBuffer());

    //将行列置为-1
    m_nRow = -1;
    m_nColumn = -1;

    //隐藏EDIT
    m_edtInput.ShowWindow(SW_HIDE);
}


void CPEresolverDlg::OnOK()
{
    // 此函数必须为空，否则按回车会导致丢失焦点，然后调用此函数
}


void CPEresolverDlg::OnBnClickedBtnOk()//点击地址转换
{
    CString csVA;
    CString csRVA;
    CString csOffset;

    GetDlgItemText(CHANGE_EDT_VA, csVA);
    GetDlgItemText(CHANGE_EDT_RVA, csRVA);
    GetDlgItemText(CHANGE_EDT_OFFSET, csOffset);

    if (csVA.GetLength() == 0 && csRVA.GetLength() == 0 && csOffset.GetLength() == 0)
    {
        AfxMessageBox("请先输入数值");
        return;
    }

    int nOffset = 0;
    int nVA = 0;
    int nRVA = 0;

    if (csVA.GetLength() != 0)//获得虚拟地址
    {
        nVA = strtoul(csVA.GetBuffer(), NULL, 16);
        nRVA = nVA - m_nImageBase;

        //判断是否在节区
        if (nRVA < m_pSection[0].VirtualAddress)
        {
            nOffset = nRVA;
        }
        else
        {
            int nBegin = 0; //当前遍历节的起始
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

        //转化
        char szVA[16] = { 0 };
        char szRVA[16] = { 0 };
        char szOffset[16] = { 0 };

        wsprintf(szVA, "0X%p", nVA);
        wsprintf(szRVA, "0X%p", nRVA);
        wsprintf(szOffset, "0X%p", nOffset);

        //写入
        SetDlgItemText(CHANGE_EDT_VA, szVA);
        SetDlgItemText(CHANGE_EDT_RVA, szRVA);
        SetDlgItemText(CHANGE_EDT_OFFSET, szOffset);
        return;
    }
    if (csRVA.GetLength() != 0)//获得相对虚拟地址偏移
    {
        nRVA = strtoul(csRVA.GetBuffer(), NULL, 16);
        nVA = nRVA + m_nImageBase;

        //判断是否在节区
        if (nRVA < m_pSection[0].VirtualAddress)
        {
            nOffset = nRVA;
        }
        else
        {
            int nBegin = 0; //当前遍历节的起始
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

        //转化
        char szVA[16] = { 0 };
        char szRVA[16] = { 0 };
        char szOffset[16] = { 0 };

        wsprintf(szVA, "0X%p", nVA);
        wsprintf(szRVA, "0X%p", nRVA);
        wsprintf(szOffset, "0X%p", nOffset);

        //写入
        SetDlgItemText(CHANGE_EDT_VA, szVA);
        SetDlgItemText(CHANGE_EDT_RVA, szRVA);
        SetDlgItemText(CHANGE_EDT_OFFSET, szOffset);
        return;
    }

    if (csOffset.GetLength() != 0)//获得文件偏移
    {
        //获取偏移
        nOffset = strtoul(csOffset.GetBuffer(), NULL, 16);

        //判断
        if (nOffset > m_FileSize)
        {
            AfxMessageBox("无法转换此数值的偏移");
            return;
        }

        if (nOffset <= m_nOffsetFirstSection)//在头区
        {
            nVA = nOffset + m_nImageBase;
            nRVA = nOffset;
        }
        else//在节块区
        {
            int nBegin = 0; //当前遍历节的起始
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
        //转化
        char szVA[16] = { 0 };
        char szRVA[16] = { 0 };
        char szOffset[16] = { 0 };

        wsprintf(szVA, "0X%p", nVA);
        wsprintf(szRVA, "0X%p", nRVA);
        wsprintf(szOffset, "0X%p", nOffset);

        //写入
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


void CPEresolverDlg::On12()//点击右键菜单新建节块
{
    CFileDialog dlgFile(TRUE);
    int nRet = dlgFile.DoModal();

    if (nRet == IDCANCEL)
    {
        return;
    }
    //修改节的数量
    m_pFILE->NumberOfSections += 1;

    CString csFilePath;
    csFilePath = dlgFile.GetPathName();

    //打开源文件
    HANDLE hSourFile = CreateFile(csFilePath.GetBuffer(),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (INVALID_HANDLE_VALUE == hSourFile)
    {
        AfxMessageBox("文件打开失败！");
        return;
    }

    //获取文件大小
    DWORD dwHigh = 0;
    DWORD dwLow = GetFileSize(hSourFile, &dwHigh);


    //1. 写节信息，关文件
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

    //修改内存中PE文件的映射大小
    m_pOptional->SizeOfImage += (dwLow / m_pOptional->FileAlignment + 1) * m_pOptional->FileAlignment;

    if (m_pView != nullptr)
    {
        //1. 取消映射视图
        UnmapViewOfFile(m_pView);
        //2. 关闭映射对象
        CloseHandle(m_hFileMap);
        //3. 关闭文件
        CloseHandle(m_hFile);
    }

    //2. 将新节写进文件
    //打开目标文件
    HANDLE hDestFile = CreateFile(m_csFilePath.GetBuffer(),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (INVALID_HANDLE_VALUE == hDestFile)
    {
        AfxMessageBox("文件打开失败！");
        return;
    }

    SetFilePointer(hDestFile, 0, 0, FILE_END);
    //现在文件末尾添加0，使文件大小为文件对齐的整数倍，再添加新增的数据进去
    for (int i = 0; i < nPointerToRawData - m_FileSize; i++)
    {
        char cData = 0;
        DWORD nReadWrite = 1;
        //写文件
        WriteFile(hDestFile, &cData, 1, &nReadWrite, NULL);
    }

    for (int i = 0; i < dwLow; i++)
    {
        char cData = 0;
        DWORD nReadWrite = 1;
        //读文件
        ReadFile(hSourFile, &cData, 1, &nReadWrite, NULL);

        //写文件
        WriteFile(hDestFile, &cData, 1, &nReadWrite, NULL);

    }
    //文件对齐
    int nFullZero = (dwLow / m_nFileAlignment + 1) * m_nFileAlignment - dwLow;
    for (int i = 0; i < nFullZero; i++)
    {
        char cData = 0;
        DWORD nReadWrite = 1;
        //写文件
        WriteFile(hDestFile, &cData, 1, &nReadWrite, NULL);

    }

    //关文件
    CloseHandle(hSourFile);
    CloseHandle(hDestFile);

    //重新读文件
    CreateFileMap();

    //重新操作节表
    ShowSectionTable();
    OperateChange();
}


void CPEresolverDlg::OnNMDblclkImportDll(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
    // TODO: 在此添加控件通知处理程序代码
    m_nCurrentSelect = EN_IMPORTDLL;
    m_nRow = pNMItemActivate->iItem;//行
    m_nColumn = pNMItemActivate->iSubItem;//列



    *pResult = 0;
}


void CPEresolverDlg::OnNMDblclkImportFunc(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
    // TODO: 在此添加控件通知处理程序代码
    m_nCurrentSelect = EN_IMPORTFUNC;
    m_nRow = pNMItemActivate->iItem;//行
    m_nColumn = pNMItemActivate->iSubItem;//列


    *pResult = 0;
}


void CPEresolverDlg::OnNMClickImportDll(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
    // TODO: 在此添加控件通知处理程序代码
    m_nCurrentSelect = EN_IMPORTDLL;
    m_nRow = pNMItemActivate->iItem;//行
    

    //编辑导入表FUNC部分，并显示出来
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
    
    //添加新标题
    m_lstResource.InsertColumn(0, _T("成员"), LVCFMT_LEFT, m_rectResource.right / 4);
    m_lstResource.InsertColumn(1, _T("偏移量"), LVCFMT_LEFT, m_rectResource.right / 4);
    m_lstResource.InsertColumn(2, _T("大小"), LVCFMT_LEFT, m_rectResource.right / 4);
    m_lstResource.InsertColumn(3, _T("值"), LVCFMT_LEFT, m_rectResource.right / 4);

    char szTemp[100] = { 0 };
    DWORD dwType = dwData & 0Xf0000000;
    dwData = dwData & 0X0fffffff;
    //添加项
    if (dwType == 0X10000000)//数据
    {
        //标题
        m_lstResource.InsertItem(0, "OffsetToData");
        m_lstResource.InsertItem(1, "Size");
        m_lstResource.InsertItem(2, "CodePage");
        m_lstResource.InsertItem(3, "Reserved");

        //偏移量
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

        //大小
        m_lstResource.SetItemText(0, 2, "DWORD");
        m_lstResource.SetItemText(1, 2, "DWORD");
        m_lstResource.SetItemText(2, 2, "DWORD");
        m_lstResource.SetItemText(3, 2, "DWORD");

        //值
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
    else if (dwType == 0X20000000)//资源目录项
    {
        m_lstResource.InsertItem(0, "Name");
        m_lstResource.InsertItem(1, "OffsetToData");

        //偏移量
        DWORD dwOffset = dwData - (DWORD)m_pView;
        wsprintf(szTemp, "0X%p", dwOffset);
        m_lstResource.SetItemText(0, 1, szTemp);

        dwOffset = dwOffset + sizeof(DWORD);
        wsprintf(szTemp, "0X%p", dwOffset);
        m_lstResource.SetItemText(1, 1, szTemp);

        //大小
        m_lstResource.SetItemText(0, 2, "DWORD");
        m_lstResource.SetItemText(1, 2, "DWORD");

        //值
        DWORD* pdwData = (DWORD*)dwData;
        wsprintf(szTemp, "%p", *pdwData);
        m_lstResource.SetItemText(0, 3, szTemp);

        pdwData++;
        wsprintf(szTemp, "%p", *pdwData);
        m_lstResource.SetItemText(1, 3, szTemp);

    }
    else if (dwType == 0X40000000)//资源目录
    {
        //标题
        m_lstResource.InsertItem(0, "Characteristics");
        m_lstResource.InsertItem(1, "TimeDateStamp");
        m_lstResource.InsertItem(2, "MajorVersion");
        m_lstResource.InsertItem(3, "MinorVersion");
        m_lstResource.InsertItem(4, "NumberOfNamedEntries");
        m_lstResource.InsertItem(5, "NumberOfIdEntries");

        //偏移量
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

        //大小
        m_lstResource.SetItemText(0, 2, "DWORD");
        m_lstResource.SetItemText(1, 2, "DWORD");
        m_lstResource.SetItemText(2, 2, "WORD");
        m_lstResource.SetItemText(3, 2, "WORD");
        m_lstResource.SetItemText(4, 2, "WORD");
        m_lstResource.SetItemText(5, 2, "WORD");

        //值
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
