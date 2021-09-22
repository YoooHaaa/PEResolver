
// PEresolverDlg.h : ͷ�ļ�
//

#pragma once
#include "afxcmn.h"
#include "Struct.h"
#include "afxwin.h"

#define FILENAMESIZE 30

/************************************************************/
//virtual void OnOK();����麯��һ��Ҫ��д����Ϊ�˷�ֹ�س�֮�� �Ի�����ʧ
//
//
//
//
//
/************************************************************/



// CPEresolverDlg �Ի���
class CPEresolverDlg : public CDialogEx
{
// ����
public:
	CPEresolverDlg(CWnd* pParent = NULL);	// ��׼���캯��
    ~CPEresolverDlg();

// �Ի�������
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_PERESOLVER_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��

/**********************************************************************************************/

    bool CreateFileMap();      //��ȡ�ļ��������ļ��������ļ�ӳ�䣬��ȡ�ļ���С
    bool InitTreeControl();    //��ʼ�����ؼ�

    void ClearListControl(CListCtrl& MyList);       //���list�ؼ�

    void GetProcessName();     //��ȡ������

    void ShowDosHeader();      //��ʾDOSͷ
    void ShowPESign();         //PE��ʶ
    void ShowFileHeader();     //��ʾPEͷ
    void ShowOptionalHeader(); //��ʾ��ѡPEͷ
    void ShowSectionTable();   //��ʾ�ڱ�
    void ShowImport();         //��ʾ������Ϣ
    void ShowImportFunc();     //��ʾ���뺯��
    void ShowExport();         //�Գ�������Ϣ
    void ShowExportFunc();     //��ʾ����������Ϣ
    void ShowRelocation();     //��ʾ�ض�λ��
    void ShowTLs();            //��ʾTLs��
    void ShowResource();       //��ʾ��Դ��

    //��ȡ��Դ��д��Դ���ؼ�,����1�����ڵ���������2��DWORD����һ����ԴĿ¼�������ݵ��ļ�ƫ��,����4����ǰ����
    //����2Ҫ���������λΪ1-����Դ���ݡ� ���λΪ2-����ԴĿ¼� ���λΪ4-����ԴĿ¼
    void ReadResource(HTREEITEM hParent, DWORD dwNext, int nType, int nNum);



    //�����������ı�,���غ���ʾ��ؿؼ�
    void OperateChange();    

    //RVA����������OFFSET
    DWORD RVAtoOFFSET(DWORD dwRVA);
    void* RVAtoMapAddr(DWORD dwRVA);
/**********************************************************************************************/
// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()


public:
    HANDLE m_hFile = 0;
    HANDLE m_hFileMap = 0;
    LPVOID m_pView = 0;


    //�ļ�·��
    CString m_csFilePath;
    char m_szPath[MAX_PATH] = { 0 };
    char m_szProcessName[FILENAMESIZE] = { 0 };//������

    RECT  m_rect;//PEͷlist�ؼ��Ĵ�С
    RECT  m_rectImportDll; //�����DLLlist�ؼ��Ĵ�С
    RECT  m_rectImportFunc;//�����FUNClist�ؼ��Ĵ�С
    RECT  m_rectExportDll; //������DLLlist�ؼ��Ĵ�С
    RECT  m_rectExportFunc;//������FUNClist�ؼ��Ĵ�С
    RECT  m_rectResource;  //��Դ��ؼ��Ĵ�С

    //�ڴ�ӳ���и��ṹ��ĵ�ַ
    _IMAGE_DOS_HEADER* m_pDOS = nullptr;
    _IMAGE_NT_HEADERS* m_pNT = nullptr;
    IMAGE_FILE_HEADER* m_pFILE = nullptr;
    IMAGE_OPTIONAL_HEADER32* m_pOptional = nullptr;
    IMAGE_SECTION_HEADER* m_pSection = nullptr;
    //�����
    IMAGE_IMPORT_DESCRIPTOR* m_pImport = nullptr;
    //������
    IMAGE_EXPORT_DIRECTORY* m_pExport = nullptr;
    //�ض�λ��
    IMAGE_BASE_RELOCATION* m_pRelocation = nullptr;
    //TLs��
    IMAGE_TLS_DIRECTORY32* m_pTLs = nullptr;
    //��Դ��
    IMAGE_RESOURCE_DIRECTORY_ENTRY* m_pResource = nullptr;
/********************************************************************/
    //�ṹ�����ļ���ƫ��
    int m_nOffsetDos = 0;
    int m_nOffsetNT = 0;
    int m_nOffsetFile = 0;
    int m_nOffsetOptional = 0;
    int m_nOffsetSection = 0;
    int m_nOffsetFirstSection = 0;//��һ�������ļ���ƫ��

    //��������ļ���ƫ��
    int m_nOffsetImport = 0;

    //���������ļ���ƫ��
    int m_nOffsetExport = 0;

    //�ض�λ�����ļ���ƫ�ƺͳߴ�
    int m_nOffsetRelocation = 0;
    int m_nSizeOfRelocation = 0;

    //TLs����ļ�ƫ��
    int m_nOffsetTls = 0;

    //��Դ����ļ�ƫ��
    int m_nOffsetResource = 0;
    int m_nSizeResource = 0;


    //�ڴ澵���ַ
    int m_nImageBase = 0;

    //����Ŀ¼�����
    int m_nNumberOfData = 0;

    //�ڱ�����
    int m_nSectionTableNum = 0;

    // �ļ���С
    DWORD m_FileSize = 0;
    //�ļ������С
    DWORD m_nFileAlignment = 0;
    //�ڴ�����С
    DWORD m_nSectionAlignment = 0;

    //���������ܸ���
    DWORD m_dwNumOfFunctions = 0;
    //��������������
    DWORD m_dwNumOfNames = 0;
    //����������ַ��RVA
    DWORD m_dwAddrOfFunction = 0;
    //�����������Ʊ�RVA
    DWORD m_dwAddrOfNames = 0;
    //����������ű�RVA
    DWORD m_dwAddrOfOrdinal = 0;
    //����������ʼ���
    DWORD m_dwBase = 0;



/********************************************************************/
    //��ǰ����������
    enum 
    {
        EN_RESNONE,
        EN_DERECTY, //��ԴĿ¼
        EN_ENTRY,   //��ԴĿ¼��
        EN_DATA     //����
    };




    //��ǰ�����Ľṹ
    enum 
    {
        EN_NONE,
        EN_DOS,
        EN_NT,
        EN_FILE,
        EN_OPTIONAL,
        EN_SECTION,
        EN_ADDRCHANGE,
        EN_IMPORTDLL,     //�����DLL����
        EN_IMPORTFUNC,    //�����FUNC����
        EN_EXPORTDLL,     //������
        EN_RELOCATION,    //�ض�λ��
        EN_TLS,           //TLs��
        EN_RESOURCE       //��Դ��
    };

    //��ǰ�����б�ؼ�������
    int m_nRow = -1;
    int m_nColumn = -1;

    //��ǰѡ��Ĳ���
    int m_nCurrentSelect = EN_NONE;


public:
/********************************************************************/
    //��Ŀ¼
    CTreeCtrl m_treeFile;

    //��ʾPE�ṹ��ؿؼ�
    CListCtrl m_lstData;
    CEdit m_edtInput;

    //��ַת����ؿؼ�
    CStatic m_changeStaVA;
    CStatic m_changeStaRVA;
    CStatic m_changeStaOffset;
    CEdit m_changeEdtVA;
    CEdit m_changeEdtRVA;
    CEdit m_changeEdtOffset;
    CButton m_changeBtnOK;
    
    //�������ؿؼ�
    CListCtrl m_lstImportDLL;
    CListCtrl m_lstImportFunc;

    //��������ؿؼ�
    CListCtrl m_lstExportDll;
    CListCtrl m_lstExportFunc;

    //��Դ��ؼ�
    CTreeCtrl m_treeResource;
    CListCtrl m_lstResource;

/********************************************************************/

    afx_msg void OnBnClickedOk();//��ַ��ȷ����ť

    afx_msg void OnTvnSelchangedFile(NMHDR *pNMHDR, LRESULT *pResult);//������ؼ�Ŀ¼��

    afx_msg void OnNMDblclkData(NMHDR *pNMHDR, LRESULT *pResult);//˫��PEͷ�б�ؼ�
    
    afx_msg void OnEnKillfocusEdit1();//�༭��ʧȥ����
    
    virtual void OnOK();//�պ���

    afx_msg void OnBnClickedBtnOk();//��ַת��ȷ����ť

    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);//����Ҽ�����

    afx_msg void On12();//����Ҽ��˵��½��ڿ�

    //�û�����굥�������DLL���ֵ�ĳһ��--��ʾFUNC��
    afx_msg void OnNMClickImportDll(NMHDR *pNMHDR, LRESULT *pResult);

    //�û��ڿؼ���˫��--�༭DLL��
    afx_msg void OnNMDblclkImportDll(NMHDR *pNMHDR, LRESULT *pResult);

    //�û��ڿؼ���˫��--�༭FUNC��
    afx_msg void OnNMDblclkImportFunc(NMHDR *pNMHDR, LRESULT *pResult);
    
    //�û������Դ�����ؼ�����
    afx_msg void OnTvnSelchangedResource(NMHDR *pNMHDR, LRESULT *pResult);
};


