
// PEresolver.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CPEresolverApp: 
// �йش����ʵ�֣������ PEresolver.cpp
//

class CPEresolverApp : public CWinApp
{
public:
	CPEresolverApp();

// ��д
public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CPEresolverApp theApp;



////3. ����ӳ����ͼ,���ļ�ӳ�䵽�ڴ�
//LPVOID pView = MapViewOfFile(
//    hFileMap,
//    FILE_MAP_WRITE, //�ɶ���д
//    0,
//    0x10000, //��0x10000ӳ����ڴ�
//    0x1000); //ӳ��һ����ҳ
//if (pView == NULL)
//{
//    AfxMessageBox(_T("�ļ�ӳ��ʧ��"));
//    CloseHandle(hFile);
//    CloseHandle(hFileMap);
//    return;
//}