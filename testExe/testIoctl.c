#include <windows.h>
#include <winioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strsafe.h>
#include "..\sys\uniioctl.h"

BOOLEAN ManageDriver(__in LPCTSTR  DriverName, __in LPCTSTR  ServiceName, __in USHORT   Function);
BOOLEAN SetupDriverName(__inout_bcount_full(BufferLength) PCHAR DriverLocation,__in ULONG BufferLength);
void SendChars(HANDLE hDevice,char* InputBuffer, char* OutputBuffer);
void ReceiveChars(HANDLE hDevice,char* InputBuffer, char* OutputBuffer);
void BulkCopyTo(HANDLE hDevice, char* InputBuffer, char* OutputBuffer);
void BulkCopyFrom(HANDLE hDevice, char* InputBuffer, char* OutputBuffer);
int TestWrite();

int _cdecl main(int argc, CHAR* argv[])
{	
	HANDLE hDevice;	 
	DWORD errNum = 0;
	TCHAR driverLocation[MAX_PATH];
	char OutputBuffer[100];
	char InputBuffer[100];
	int bRetur  = 0;
	BOOL transactionResult;
	LPCVOID mapping;
	SYSTEM_INFO *sysInfo;
	
	char* sharedMem = NULL;

	printf("Start!\n");
	sysInfo = malloc(sizeof(SYSTEM_INFO));
	ZeroMemory(sysInfo, sizeof(SYSTEM_INFO));
	GetSystemInfo(sysInfo);
	printf("Granularity: %i",sysInfo->dwAllocationGranularity);
	if (sysInfo == NULL)
	{
		return 0;
	}
	//If at least one parameter unload a locked driver
	if (argc>1)
	{
		printf("Try to unload!\n");
		if ((hDevice = CreateFile( "\\\\.\\UnipiIoctl",
                            GENERIC_READ | GENERIC_WRITE,
                            0,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL)) == INVALID_HANDLE_VALUE) 
		{
			printf("\nClosing handle...:\n");
			CloseHandle ( hDevice );
			printf("\Unloading driver...:\n");
			ManageDriver(DRIVER_NAME,
						 driverLocation,
						 DRIVER_FUNC_REMOVE
						 );
		}else{
			errNum = GetLastError();
			printf("Failed to unload driver: %i",errNum);
			
			if (!SetupDriverName(driverLocation, sizeof(driverLocation))) {	
            return ;
			}	
			ManageDriver(DRIVER_NAME,
						 driverLocation,
						 DRIVER_FUNC_REMOVE
						 );
			return;
		}
		return 0;
	}
	
	if ((hDevice = CreateFile( "\\\\.\\UnipiIoctl",
                            GENERIC_READ | GENERIC_WRITE,
                            0,
                            NULL,
                            CREATE_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL)) == INVALID_HANDLE_VALUE) {

        errNum = GetLastError();

        if (errNum != ERROR_FILE_NOT_FOUND) {
            printf("CreateFile failed!  ERROR_FILE_NOT_FOUND = %d\n", errNum);
            return ;
			}else{
			printf("CreateFile ok!\n");		
        }
		
        if (!SetupDriverName(driverLocation, sizeof(driverLocation))) {	
            return ;
        }

        if (!ManageDriver(DRIVER_NAME,
                          driverLocation,
                          DRIVER_FUNC_INSTALL
                          )) {
            printf("Unable to install driver. \n");
            ManageDriver(DRIVER_NAME,
                         driverLocation,
                         DRIVER_FUNC_REMOVE
                         );
            return;
        }
        hDevice = CreateFile( "\\\\.\\UnipiIoctl",
                            GENERIC_READ | GENERIC_WRITE,
                            0,
                            NULL,
                            CREATE_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);

        if ( hDevice == INVALID_HANDLE_VALUE ){
            printf ( "Error: CreatFile Failed : %d\n", GetLastError());
            return;
        }
		printf("CreateFile ok!\n");
		printf("Press enter to start...\n");
		system("PAUSE");
		TestWrite();
		
		transactionResult = DeviceIoControl ( hDevice,
							(DWORD) IOCTL_TEST_WRITTEN_DATA,
							0,
							0,
							0,
							0,
							&bRetur,
							NULL
							);
		
		printf("\nClosing handle...:\n");
		CloseHandle ( hDevice );
		printf("\Unloading driver...:\n");
		ManageDriver(DRIVER_NAME,
					 driverLocation,
					 DRIVER_FUNC_REMOVE
					 );
	}
	
    return 0;
}

int TestWrite()
{
	HANDLE hMapFile;
	char* pBuf = NULL;
	int lastError=0;
	int i = 0;
		
	//pBuf = malloc(4096);
	

	hMapFile = OpenFileMapping(
					FILE_MAP_ALL_ACCESS,          // read/write access
					FALSE,                      
					"Global\\Netmap");                 // name of mapping object*/
	/*hMapFile = CreateFileMapping(
                 INVALID_HANDLE_VALUE,    // use paging file
                 NULL,                    // default security
                 PAGE_READWRITE,          // read/write access
                 0,                       // maximum object size (high-order DWORD)
                 4096,                // maximum object size (low-order DWORD)
                 "Global\\SharedMemory");                 // name of mapping object*/

	lastError = GetLastError();
	if (hMapFile == NULL)
	{
		printf("Could not create file mapping object (%d).\n" ,GetLastError());
		return 1;
	}
	pBuf = (char*)MapViewOfFile(hMapFile,   // handle to map object
								FILE_MAP_ALL_ACCESS, // read/write permission
								0,
								0,
								4096);

	if (pBuf == NULL)
	{
		printf("Could not map view of file (%d).\n", GetLastError());
		CloseHandle(hMapFile);
		return 1;
	}
	
	printf("Print array\n");
	for (i=0;i<4096;i++)
	{
		if (pBuf[i]!=0)
		{
			printf("pBuf[%i]: %i\n",i,pBuf[i]);
		}
	}
	
	/*printf("pBuf[0]: %i\n",pBuf[0]);
	printf("pBuf[1]: %i\n",pBuf[1]);
	printf("pBuf[2]: %i\n",pBuf[2]);*/
	
	
	pBuf[0] = 'c';
	pBuf[1] = '\n';
	pBuf[4095] = '\n';
	
	CloseHandle(hMapFile);
	return 0;
}

void SendChars(HANDLE hDevice, char* InputBuffer, char* OutputBuffer)
{
	int bRetur  = 0;
	BOOL transactionResult;
	//printf("\nInside SendChars...\n");	
	//printf("Inbuf: %s\n", InputBuffer);					
	//printf("Outbuf: %s\n", OutputBuffer);
	
	transactionResult = DeviceIoControl ( hDevice,
							(DWORD) IOCTL_METHOD_IN_NEITHER,
							//(DWORD) IOCTL_METHOD_IN_DIRECT,
							InputBuffer,
							(DWORD) strlen ( InputBuffer )+1,
							OutputBuffer,
							(DWORD) strlen ( OutputBuffer )+1,
							&bRetur,
							NULL
							);
}
void ReceiveChars(HANDLE hDevice, char* InputBuffer, char* OutputBuffer)
{
	int bRetur  = 0;
	BOOL transactionResult;
	//printf("\nInside ReceiveChars...\n");	
	transactionResult = DeviceIoControl ( hDevice,
							(DWORD) IOCTL_METHOD_OUT_DIRECT,
							InputBuffer,
							(DWORD) strlen ( InputBuffer )+1,
							OutputBuffer,
							100,
							&bRetur,
							NULL
							);
}

void BulkCopyTo(HANDLE hDevice, char* InputBuffer, char* OutputBuffer)
{
	LARGE_INTEGER frequency;        // ticks per second
    LARGE_INTEGER t1, t2;  
	double elapsedTime;
	BOOLEAN performanceOnEveryTransfer = FALSE;
	int i = 0;
	int maxCycles = 1000000;
	LARGE_INTEGER performance;
	LARGE_INTEGER totalTime;
	totalTime.QuadPart=0;
	
	QueryPerformanceFrequency(&frequency);
	memset(OutputBuffer, 0, 100);
	StringCbCopy(InputBuffer, 100, "Stringa cosi' per vedere se cambia qualcosa in termini di performance");
	if (!performanceOnEveryTransfer)
	{
		QueryPerformanceCounter(&t1);
	}
	for (i=0;i<maxCycles;i++)
	{
		if (performanceOnEveryTransfer)
		{
			QueryPerformanceCounter(&t1);
		}
		SendChars(hDevice, InputBuffer, OutputBuffer);
		if (performanceOnEveryTransfer)
		{
			QueryPerformanceCounter(&t2);
			performance.QuadPart=(t2.QuadPart - t1.QuadPart);
			performance.QuadPart *= 1000000;
			performance.QuadPart /= frequency.QuadPart;
			totalTime.QuadPart+=performance.QuadPart;
		}
		
		//Sleep(1);
		if (i%100000==0)
		{
			printf("Packet %i\n",i);
		}
	}
	if (!performanceOnEveryTransfer)
	{
		QueryPerformanceCounter(&t2);
		performance.QuadPart=(t2.QuadPart - t1.QuadPart);
		performance.QuadPart *= 1000000;
		performance.QuadPart /= frequency.QuadPart;
		totalTime.QuadPart+=performance.QuadPart;
	}
	printf("Total time %d microseconds\n",totalTime.QuadPart);
	printf("Mean time %d nanoseconds\n",((totalTime.QuadPart*1000)/maxCycles));
}

void BulkCopyFrom(HANDLE hDevice, char* InputBuffer, char* OutputBuffer)
{
	int i = 0;
	for (i=0;i<10;i++)
	{
		memset(InputBuffer, 0, 100);
		memset(OutputBuffer, 0, 100);
		ReceiveChars(hDevice, InputBuffer, OutputBuffer);				
		printf("%i: Outbuf: %s\n",i, OutputBuffer);		
	}
}