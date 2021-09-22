#include "stdafx.h"
#include "Struct.h"

DOSHEADER DosHeader[] =
{
2, "e_magic"      , "Magic number",
2, "e_cblp"       , "Bytes on last page of file",
2, "e_cp"         , "Pages in file",
2, "e_crlc"       , "Relocations",
2, "e_cparhdr"    , "Size of header in paragraphs",
2, "e_minalloc"   , "Minimum extra paragraphs needed",
2, "e_maxalloc"   , "Maximum extra paragraphs needed",
2, "e_ss"         , "Initial(relative) SS value",
2, "e_sp"         , "Initial SP value",
2, "e_csum"       , "Checksum",
2, "e_ip"         , "Initial IP value",
2, "e_cs"         , "Initial(relative) CS value",
2, "e_lfarlc"     , "File address of relocation table",
2, "e_ovno"       , "Overlay number",
2, "e_res[0]"     , "Reserved words",
2, "e_res[1]"     , "Reserved words",
2, "e_res[2]"     , "Reserved words",
2, "e_res[3]"     , "Reserved words",
2, "e_oemid"      , "OEM identifier(for e_oeminfo)",
2, "e_oeminfo"    , "OEM information; e_oemid specific",
2, "e_res2[0]"    , "Reserved words",
2, "e_res2[1]"    , "Reserved words",
2, "e_res2[2]"    , "Reserved words",
2, "e_res2[3]"    , "Reserved words",
2, "e_res2[4]"    , "Reserved words",
2, "e_res2[5]"    , "Reserved words",
2, "e_res2[6]"    , "Reserved words",
2, "e_res2[7]"    , "Reserved words",
2, "e_res2[8]"    , "Reserved words",
2, "e_res2[9]"    , "Reserved words",
4, "e_lfanew"     , "File address of new exe header"
};
DOSHEADER* g_pDosHeader = DosHeader;




MACHINE Machine[] =
{
    0       , "",
    0x014c  , "Intel 386.",
    0x0162  , "MIPS little-endian, 0x160 big-endian",
    0x0166  , "MIPS little-endian",
    0x0168  , "MIPS little-endian",
    0x0169  , "MIPS little-endian WCE v2",
    0x0184  , "Alpha_AXP",
    0x01a2  , "SH3 little-endian",
    0x01a3  , "",
    0x01a4  , "SH3E little-endian",
    0x01a6  , "SH4 little-endian",
    0x01a8  , "SH5",
    0x01c0  , "ARM Little-Endian",
    0x01c2  , "ARM Thumb/Thumb-2 Little-Endian",
    0x01c4  , "ARM Thumb-2 Little-Endian",
    0x01d3  , "",
    0x01F0  , "IBM PowerPC Little-Endian",
    0x01f1  , "",
    0x0200  , "Intel 64",
    0x0266  , "MIPS",
    0x0284  , "ALPHA64",
    0x0366  , "MIPS",
    0x0466  , "MIPS",
    0x0520  , "Infineon",
    0x0CEF  , "",
    0x0EBC  , "EFI Byte Code",
    0x8664  , "AMD64 (K8)",
    0x9041  , "M32R little-endian",
    0xC0EE  , ""
};

MACHINE* g_pMachine = Machine;

CHARACTERISTICS character[] =
{
     0x0001  , "Relocation info stripped from file.",
     0x0002  , "File is executable  (i.e. no unresolved external references).",
     0x0004  , "Line nunbers stripped from file.",
     0x0008  , "Local symbols stripped from file.",
     0x0010  , "Aggressively trim working set",
     0x0020  , "App can handle >2gb addresses",
     0x0080  , "Bytes of machine word are reversed.",
     0x0100  , "32 bit word machine.",
     0x0200  , "Debugging info stripped from file in .DBG file",
     0x0400  , "If Image is on removable media, copy and run from the swap file.",
     0x0800  , "If Image is on Net, copy and run from the swap file.",
     0x1000  , "System File.",
     0x2000  , "File is a DLL.",
     0x4000  , "File should only be run on a UP machine",
     0x8000  , "Bytes of machine word are reversed."
};


CHARACTERISTICS* g_pCharact = character;


OPTIONALPE optional[] =
{
    2, "Magic",
    1, "MajorLinkerVersion",
    1, "MinorLinkerVersion",
    4, "SizeOfCode",
    4, "SizeOfInitializedData",
    4, "SizeOfUninitializedData",
    4, "AddressOfEntryPoint",
    4, "BaseOfCode",
    4, "BaseOfData",
    4, "ImageBase",
    4, "SectionAlignment",
    4, "FileAlignment",
    2, "MajorOperatingSystemVersion",
    2, "MinorOperatingSystemVersion",
    2, "MajorImageVersion",
    2, "MinorImageVersion",
    2, "MajorSubsystemVersion",
    2, "MinorSubsystemVersion",
    4, "Win32VersionValue",
    4, "SizeOfImage",
    4, "SizeOfHeaders",
    4, "CheckSum",
    2, "Subsystem",
    2, "DllCharacteristics",
    4, "SizeOfStackReserve",
    4, "SizeOfStackCommit",
    4, "SizeOfHeapReserve",
    4, "SizeOfHeapCommit",
    4, "LoaderFlags",
    4, "NumberOfRvaAndSizes",
    8, "DataDirectory[0]",
    8, "DataDirectory[1]",
    8, "DataDirectory[2]",
    8, "DataDirectory[3]",
    8, "DataDirectory[4]",
    8, "DataDirectory[5]",
    8, "DataDirectory[6]",
    8, "DataDirectory[7]",
    8, "DataDirectory[8]",
    8, "DataDirectory[9]",
    8, "DataDirectory[10]",
    8, "DataDirectory[11]",
    8, "DataDirectory[12]",
    8, "DataDirectory[13]",
    8, "DataDirectory[14]",
    8, "DataDirectory[15]"
};

OPTIONALPE* g_pOptional = optional;

FILEHEADER peheader[] =
{
    2,  "Machine",
    2,  "NumberOfSections",
    4,  "TimeDateStamp",
    4,  "PointerToSymbolTable",
    4,  "NumberOfSymbols",
    2,  "SizeOfOptionalHeader",
    2,  "Characteristics"
};

FILEHEADER*g_pFileHeader = peheader;

SECTIONTABLE section[] = 
{
    8,   "Name",
    4,   "Misc",
    4,   "VirtualAddress",
    4,   "SizeOfRawData",
    4,   "PointerToRawData",
    4,   "PointerToRelocations",
    4,   "PointerToLinenumbers",
    2,   "NumberOfRelocations",
    2,   "NumberOfLinenumbers",
    4,   "Characteristics"
};

SECTIONTABLE* g_pSectionTable = section;


EXPORTTABLE exporttable[] =
{
    4,  "Characteristics", "",
    4,  "TimeDateStamp", "",
    2,  "MajorVersion", "",
    2,  "MinorVersion", "",
    4,  "Name", "导出表文件名(RVA)",
    4,  "Base", "导出函数起始序号",
    4,  "NumberOfFunctions", "导出函数总个数",
    4,  "NumberOfNames", "以函数名导出的函数个数",
    4,  "AddressOfFunctions", "导出函数地址表（RVA)",
    4,  "AddressOfNames", "导出函数名称表（RVA)",
    4,  "AddressOfNameOrdinals", "导出函数序号表（RVA)"
};

EXPORTTABLE* g_pExportTable = exporttable;