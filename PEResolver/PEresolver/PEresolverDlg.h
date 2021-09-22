
// PEresolverDlg.h : 头文件
//

#pragma once
#include "afxcmn.h"
#include "Struct.h"
#include "afxwin.h"

#define FILENAMESIZE 30

/************************************************************/
//virtual void OnOK();这个虚函数一定要重写，是为了防止回车之后 对话框消失
//
//
//
//
//
/************************************************************/



// CPEresolverDlg 对话框
class CPEresolverDlg : public CDialogEx
{
// 构造
public:
	CPEresolverDlg(CWnd* pParent = NULL);	// 标准构造函数
    ~CPEresolverDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_PERESOLVER_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持

/**********************************************************************************************/

    bool CreateFileMap();      //获取文件名，打开文件，创建文件映射，获取文件大小
    bool InitTreeControl();    //初始化树控件

    void ClearListControl(CListCtrl& MyList);       //清空list控件

    void GetProcessName();     //获取进程名

    void ShowDosHeader();      //显示DOS头
    void ShowPESign();         //PE标识
    void ShowFileHeader();     //显示PE头
    void ShowOptionalHeader(); //显示可选PE头
    void ShowSectionTable();   //显示节表
    void ShowImport();         //显示导入信息
    void ShowImportFunc();     //显示导入函数
    void ShowExport();         //显出导出信息
    void ShowExportFunc();     //显示导出函数信息
    void ShowRelocation();     //显示重定位表
    void ShowTLs();            //显示TLs表
    void ShowResource();       //显示资源表

    //读取资源填写资源树控件,参数1：父节点句柄，参数2：DWORD，下一个资源目录或者数据的文件偏移,参数4：当前项数
    //参数2要做处理，最高位为1-》资源数据。 最高位为2-》资源目录项。 最高位为4-》资源目录
    void ReadResource(HTREEITEM hParent, DWORD dwNext, int nType, int nNum);



    //操作对象发生改变,隐藏和显示相关控件
    void OperateChange();    

    //RVA————》OFFSET
    DWORD RVAtoOFFSET(DWORD dwRVA);
    void* RVAtoMapAddr(DWORD dwRVA);
/**********************************************************************************************/
// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()


public:
    HANDLE m_hFile = 0;
    HANDLE m_hFileMap = 0;
    LPVOID m_pView = 0;


    //文件路径
    CString m_csFilePath;
    char m_szPath[MAX_PATH] = { 0 };
    char m_szProcessName[FILENAMESIZE] = { 0 };//进程名

    RECT  m_rect;//PE头list控件的大小
    RECT  m_rectImportDll; //导入表DLLlist控件的大小
    RECT  m_rectImportFunc;//导入表FUNClist控件的大小
    RECT  m_rectExportDll; //导出表DLLlist控件的大小
    RECT  m_rectExportFunc;//导出表FUNClist控件的大小
    RECT  m_rectResource;  //资源表控件的大小

    //内存映射中各结构体的地址
    _IMAGE_DOS_HEADER* m_pDOS = nullptr;
    _IMAGE_NT_HEADERS* m_pNT = nullptr;
    IMAGE_FILE_HEADER* m_pFILE = nullptr;
    IMAGE_OPTIONAL_HEADER32* m_pOptional = nullptr;
    IMAGE_SECTION_HEADER* m_pSection = nullptr;
    //导入表
    IMAGE_IMPORT_DESCRIPTOR* m_pImport = nullptr;
    //导出表
    IMAGE_EXPORT_DIRECTORY* m_pExport = nullptr;
    //重定位表
    IMAGE_BASE_RELOCATION* m_pRelocation = nullptr;
    //TLs表
    IMAGE_TLS_DIRECTORY32* m_pTLs = nullptr;
    //资源表
    IMAGE_RESOURCE_DIRECTORY_ENTRY* m_pResource = nullptr;
/********************************************************************/
    //结构体在文件的偏移
    int m_nOffsetDos = 0;
    int m_nOffsetNT = 0;
    int m_nOffsetFile = 0;
    int m_nOffsetOptional = 0;
    int m_nOffsetSection = 0;
    int m_nOffsetFirstSection = 0;//第一个节在文件的偏移

    //导入表在文件的偏移
    int m_nOffsetImport = 0;

    //导出表在文件的偏移
    int m_nOffsetExport = 0;

    //重定位表在文件的偏移和尺寸
    int m_nOffsetRelocation = 0;
    int m_nSizeOfRelocation = 0;

    //TLs表的文件偏移
    int m_nOffsetTls = 0;

    //资源表的文件偏移
    int m_nOffsetResource = 0;
    int m_nSizeResource = 0;


    //内存镜像基址
    int m_nImageBase = 0;

    //计算目录项个数
    int m_nNumberOfData = 0;

    //节表数量
    int m_nSectionTableNum = 0;

    // 文件大小
    DWORD m_FileSize = 0;
    //文件对齐大小
    DWORD m_nFileAlignment = 0;
    //内存对齐大小
    DWORD m_nSectionAlignment = 0;

    //导出函数总个数
    DWORD m_dwNumOfFunctions = 0;
    //导出函数名个数
    DWORD m_dwNumOfNames = 0;
    //导出函数地址表RVA
    DWORD m_dwAddrOfFunction = 0;
    //导出函数名称表RVA
    DWORD m_dwAddrOfNames = 0;
    //导出函数序号表RVA
    DWORD m_dwAddrOfOrdinal = 0;
    //导出函数起始序号
    DWORD m_dwBase = 0;



/********************************************************************/
    //当前操作的类型
    enum 
    {
        EN_RESNONE,
        EN_DERECTY, //资源目录
        EN_ENTRY,   //资源目录项
        EN_DATA     //数据
    };




    //当前操作的结构
    enum 
    {
        EN_NONE,
        EN_DOS,
        EN_NT,
        EN_FILE,
        EN_OPTIONAL,
        EN_SECTION,
        EN_ADDRCHANGE,
        EN_IMPORTDLL,     //导入表DLL部分
        EN_IMPORTFUNC,    //导入表FUNC部分
        EN_EXPORTDLL,     //导出表
        EN_RELOCATION,    //重定位表
        EN_TLS,           //TLs表
        EN_RESOURCE       //资源表
    };

    //当前操作列表控件的行列
    int m_nRow = -1;
    int m_nColumn = -1;

    //当前选择的操作
    int m_nCurrentSelect = EN_NONE;


public:
/********************************************************************/
    //主目录
    CTreeCtrl m_treeFile;

    //显示PE结构相关控件
    CListCtrl m_lstData;
    CEdit m_edtInput;

    //地址转换相关控件
    CStatic m_changeStaVA;
    CStatic m_changeStaRVA;
    CStatic m_changeStaOffset;
    CEdit m_changeEdtVA;
    CEdit m_changeEdtRVA;
    CEdit m_changeEdtOffset;
    CButton m_changeBtnOK;
    
    //导入表相关控件
    CListCtrl m_lstImportDLL;
    CListCtrl m_lstImportFunc;

    //导出表相关控件
    CListCtrl m_lstExportDll;
    CListCtrl m_lstExportFunc;

    //资源表控件
    CTreeCtrl m_treeResource;
    CListCtrl m_lstResource;

/********************************************************************/

    afx_msg void OnBnClickedOk();//地址栏确定按钮

    afx_msg void OnTvnSelchangedFile(NMHDR *pNMHDR, LRESULT *pResult);//点击树控件目录项

    afx_msg void OnNMDblclkData(NMHDR *pNMHDR, LRESULT *pResult);//双击PE头列表控件
    
    afx_msg void OnEnKillfocusEdit1();//编辑框失去焦点
    
    virtual void OnOK();//空函数

    afx_msg void OnBnClickedBtnOk();//地址转换确定按钮

    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);//鼠标右键按下

    afx_msg void On12();//点击右键菜单新建节块

    //用户将鼠标单击导入表DLL部分的某一项--显示FUNC表
    afx_msg void OnNMClickImportDll(NMHDR *pNMHDR, LRESULT *pResult);

    //用户在控件内双击--编辑DLL表
    afx_msg void OnNMDblclkImportDll(NMHDR *pNMHDR, LRESULT *pResult);

    //用户在控件内双击--编辑FUNC表
    afx_msg void OnNMDblclkImportFunc(NMHDR *pNMHDR, LRESULT *pResult);
    
    //用户点击资源表树控件的项
    afx_msg void OnTvnSelchangedResource(NMHDR *pNMHDR, LRESULT *pResult);
};


