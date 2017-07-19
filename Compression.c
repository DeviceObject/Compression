#include "Compression.h"

RTLCOMPRESSBUFFER g_RtlCompressBuffer = NULL;
RTLGETCOMPRESSIONWORKSPACESIZE g_RtlGetCompressionWorkSpaceSize = NULL;

BOOLEAN GetFileDat(PCHAR pFileName,PCHAR* pDat,PULONG ulSize)
{
	BOOLEAN bRet;
	HANDLE hFile;
	ULONG ulRetBytesSize;
	LARGE_INTEGER lSize;

	bRet = FALSE;
	lSize.QuadPart = 0;

	do 
	{
		hFile = CreateFileA(pFileName, \
			FILE_READ_ACCESS, \
			FILE_SHARE_READ | FILE_SHARE_WRITE, \
			NULL, \
			OPEN_EXISTING, \
			FILE_ATTRIBUTE_NORMAL, \
			NULL);
		if (INVALID_HANDLE_VALUE == hFile)
		{
			break;
		}
		bRet = GetFileSizeEx(hFile,&lSize);
		if (FALSE == bRet)
		{
			break;
		}
		*pDat = VirtualAlloc(NULL,lSize.LowPart,MEM_RESERVE | MEM_COMMIT,PAGE_READWRITE);
		if (NULL == *pDat)
		{
			break;
		}
		RtlZeroMemory(*pDat,lSize.LowPart);
		*ulSize = lSize.LowPart;
		bRet = ReadFile(hFile,*pDat,lSize.LowPart,&ulRetBytesSize,NULL);
		if (FALSE == bRet)
		{
			break;
		}
		bRet = TRUE;
	} while (0);
	if (hFile)
	{
		CloseHandle(hFile);
	}
	return bRet;
}

int main(int nArgc,PCHAR pArgv[])
{
	PCHAR pFileDat;
	ULONG ulFileSize;
	PCHAR pCompressedDat;
	HMODULE hNtdll;
	ULONG ulCompressBufferWorkSpaceSize;
	ULONG ulCompressFragmentWorkSpaceSize;
	ULONG ulFinalCompressedSize;
	ULONG ulStatus;
	PCHAR pWorkSpace;
	HANDLE hFile;

	pFileDat = NULL;
	ulFileSize = 0;
	pCompressedDat = NULL;
	hNtdll = NULL;
	ulCompressBufferWorkSpaceSize = 0;
	ulCompressFragmentWorkSpaceSize = 0;
	ulFinalCompressedSize = 0;
	ulStatus = 0;
	pWorkSpace = NULL;
	hFile = INVALID_HANDLE_VALUE;

	switch (nArgc)
	{
	case 3:
		if (GetFileDat(pArgv[1],&pFileDat,&ulFileSize))
		{
			hNtdll = LoadLibraryA("ntdll.dll");
			g_RtlGetCompressionWorkSpaceSize = (RTLGETCOMPRESSIONWORKSPACESIZE)GetProcAddress(hNtdll,"RtlGetCompressionWorkSpaceSize");
			g_RtlCompressBuffer = (RTLCOMPRESSBUFFER)GetProcAddress(hNtdll,"RtlCompressBuffer");
			if (g_RtlCompressBuffer && g_RtlGetCompressionWorkSpaceSize)
			{
				do 
				{
					pCompressedDat = (PCHAR)VirtualAlloc(NULL,ulFileSize,MEM_COMMIT | MEM_RESERVE,PAGE_READWRITE);
				} while (NULL == pCompressedDat);
				RtlZeroMemory(pCompressedDat,ulFileSize);
				ulStatus = g_RtlGetCompressionWorkSpaceSize(COMPRESSION_FORMAT_LZNT1,&ulCompressBufferWorkSpaceSize,&ulCompressFragmentWorkSpaceSize);
				if (ulStatus == 0)
				{
					do 
					{
						pWorkSpace = (PCHAR)VirtualAlloc(NULL,ulCompressBufferWorkSpaceSize,MEM_COMMIT | MEM_RESERVE,PAGE_READWRITE);
					} while (NULL == pWorkSpace);
					RtlZeroMemory(pWorkSpace,ulCompressBufferWorkSpaceSize);
					ulStatus = g_RtlCompressBuffer(COMPRESSION_FORMAT_LZNT1, \
						(PUCHAR)pFileDat, \
						ulFileSize, \
						(PUCHAR)pCompressedDat, \
						ulFileSize, \
						4096, \
						&ulFinalCompressedSize, \
						pWorkSpace);
					if (ulStatus == 0)
					{
						hFile = CreateFileA(pArgv[2], \
							FILE_ALL_ACCESS, \
							FILE_SHARE_READ | FILE_SHARE_WRITE, \
							NULL, \
							OPEN_ALWAYS, \
							FILE_ATTRIBUTE_NORMAL, \
							NULL);
						if (INVALID_HANDLE_VALUE == hFile)
						{
							break;
						}
						if (WriteFile(hFile,pCompressedDat,ulFinalCompressedSize,&ulFinalCompressedSize,NULL) == FALSE)
						{
							break;
						}
					}

				}
			}
		}
		break;
	default:
		break;
	}
	if (hFile)
	{
		CloseHandle(hFile);
	}
	if (pWorkSpace)
	{
		VirtualFree(pWorkSpace,ulCompressBufferWorkSpaceSize,MEM_RESERVE);
	}
	if (pCompressedDat)
	{
		VirtualFree(pCompressedDat,ulFileSize,MEM_RESERVE);
	}
	return 0;
}

