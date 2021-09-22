#pragma once
#include <windows.h>

struct DOSHEADER//DOSͷ
{
    int nSize;
    char* pInfo;
    char* pDescription;
};

extern  DOSHEADER* g_pDosHeader;


struct FILEHEADER//PEͷ
{
    int nSize;
    char* pInfo;
};

extern  FILEHEADER* g_pFileHeader;


struct MACHINE//�����ͺ�
{
    WORD wData;
    char* pInfo;
};

extern  MACHINE* g_pMachine;


struct CHARACTERISTICS//�ļ�����
{
    WORD wData;
    char* pInfo;
};
extern  CHARACTERISTICS* g_pCharact;

struct OPTIONALPE//��չPEͷ�ṹ
{
    int nSize;
    char* pInfo;
};
extern  OPTIONALPE* g_pOptional;


struct SECTIONTABLE//�ڱ�
{
    int nSize;
    char* pInfo;
};
extern  SECTIONTABLE* g_pSectionTable;

struct EXPORTTABLE//������
{
    int nSize;
    char* pInfo;
    char* pDescription;
};
extern  EXPORTTABLE* g_pExportTable;