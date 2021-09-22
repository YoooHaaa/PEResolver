
// PEresolver.h : PROJECT_NAME 应用程序的主头文件
//

#pragma once

#ifndef __AFXWIN_H__
	#error "在包含此文件之前包含“stdafx.h”以生成 PCH 文件"
#endif

#include "resource.h"		// 主符号


// CPEresolverApp: 
// 有关此类的实现，请参阅 PEresolver.cpp
//

class CPEresolverApp : public CWinApp
{
public:
	CPEresolverApp();

// 重写
public:
	virtual BOOL InitInstance();

// 实现

	DECLARE_MESSAGE_MAP()
};

extern CPEresolverApp theApp;



////3. 创建映射视图,把文件映射到内存
//LPVOID pView = MapViewOfFile(
//    hFileMap,
//    FILE_MAP_WRITE, //可读可写
//    0,
//    0x10000, //从0x10000映射进内存
//    0x1000); //映射一个分页
//if (pView == NULL)
//{
//    AfxMessageBox(_T("文件映射失败"));
//    CloseHandle(hFile);
//    CloseHandle(hFileMap);
//    return;
//}