#pragma once
#include <windows.h>

struct DOSHEADER//DOS头
{
    int nSize;
    char* pInfo;
    char* pDescription;
};

extern  DOSHEADER* g_pDosHeader;


struct FILEHEADER//PE头
{
    int nSize;
    char* pInfo;
};

extern  FILEHEADER* g_pFileHeader;


struct MACHINE//机器型号
{
    WORD wData;
    char* pInfo;
};

extern  MACHINE* g_pMachine;


struct CHARACTERISTICS//文件属性
{
    WORD wData;
    char* pInfo;
};
extern  CHARACTERISTICS* g_pCharact;

struct OPTIONALPE//拓展PE头结构
{
    int nSize;
    char* pInfo;
};
extern  OPTIONALPE* g_pOptional;


struct SECTIONTABLE//节表
{
    int nSize;
    char* pInfo;
};
extern  SECTIONTABLE* g_pSectionTable;

struct EXPORTTABLE//导出表
{
    int nSize;
    char* pInfo;
    char* pDescription;
};
extern  EXPORTTABLE* g_pExportTable;