#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strsafe.h>
#include "uniioctl.h"

BOOLEAN
InstallDriver(
    __in SC_HANDLE  SchSCManager,
    __in LPCTSTR    DriverName,
    __in LPCTSTR    ServiceExe
    );


BOOLEAN
RemoveDriver(
    __in SC_HANDLE  SchSCManager,
    __in LPCTSTR    DriverName
    );

BOOLEAN
StartDriver(
    __in SC_HANDLE  SchSCManager,
    __in LPCTSTR    DriverName
    );

BOOLEAN
StopDriver(
    __in SC_HANDLE  SchSCManager,
    __in LPCTSTR    DriverName
    );

BOOLEAN
InstallDriver(
    __in SC_HANDLE  SchSCManager,
    __in LPCTSTR    DriverName,
    __in LPCTSTR    ServiceExe
    )
{
    SC_HANDLE   schService;
    DWORD       err;
    schService = CreateService(SchSCManager,           
                               DriverName,             
                               DriverName,             
                               SERVICE_ALL_ACCESS,     
                               SERVICE_KERNEL_DRIVER,  
                               SERVICE_DEMAND_START,   
                               SERVICE_ERROR_NORMAL,   
                               ServiceExe,             
                               NULL,                   
                               NULL,                   
                               NULL,                   
                               NULL,                   
                               NULL                    
                               );

    if (schService == NULL) {

        err = GetLastError();

        if (err == ERROR_SERVICE_EXISTS) {
            return TRUE;
        } else {
            printf("CreateService failed!  Error = %d \n", err );
            return  FALSE;
        }
    }

    if (schService) {

        CloseServiceHandle(schService);
    }
    return TRUE;
}

BOOLEAN
ManageDriver(
    __in LPCTSTR  DriverName,
    __in LPCTSTR  ServiceName,
    __in USHORT   Function
    )
{
    SC_HANDLE   schSCManager;
    BOOLEAN rCode = TRUE;
    if (!DriverName || !ServiceName) {
        printf("Invalid Driver or Service provided to ManageDriver() \n");
        return FALSE;
    }

    schSCManager = OpenSCManager(NULL,                   
                                 NULL,                   
                                 SC_MANAGER_ALL_ACCESS   
                                 );
    if (!schSCManager) {
        printf("Open SC Manager failed! Error = %d \n", GetLastError());
        return FALSE;
    }

    switch( Function ) {
        case DRIVER_FUNC_INSTALL:
            if (InstallDriver(schSCManager,
                              DriverName,
                              ServiceName
                              )) {
                rCode = StartDriver(schSCManager,
                                    DriverName
                                    );
            } else {
                rCode = FALSE;
            }
            break;
        case DRIVER_FUNC_REMOVE:
            StopDriver(schSCManager,
                       DriverName
                       );
            RemoveDriver(schSCManager,
                         DriverName
                         );
            rCode = TRUE;
            break;
        default:
            printf("Unknown ManageDriver() function. \n");
            rCode = FALSE;
            break;
    }
    if (schSCManager) {
        CloseServiceHandle(schSCManager);
    }
    return rCode;

} 

BOOLEAN
RemoveDriver(
    __in SC_HANDLE    SchSCManager,
    __in LPCTSTR      DriverName
    )
{
    SC_HANDLE   schService;
    BOOLEAN     rCode;
    schService = OpenService(SchSCManager,
                             DriverName,
                             SERVICE_ALL_ACCESS
                             );
    if (schService == NULL) {
        printf("OpenService failed!  Error = %d \n", GetLastError());
        return FALSE;
    }
    if (DeleteService(schService)) {
        rCode = TRUE;
    } else {
        printf("DeleteService failed!  Error = %d \n", GetLastError());
        rCode = FALSE;
    }
	if (schService) {

        CloseServiceHandle(schService);
    }
    return rCode;
} 

BOOLEAN
StartDriver(
    __in SC_HANDLE    SchSCManager,
    __in LPCTSTR      DriverName
    )
{
    SC_HANDLE   schService;
    DWORD       err;
	schService = OpenService(SchSCManager,
                             DriverName,
                             SERVICE_ALL_ACCESS
                             );
    if (schService == NULL) {
        printf("OpenService failed!  Error = %d \n", GetLastError());
        return FALSE;
    }
    if (!StartService(schService,     
                      0,              
                      NULL            
                      )) {
        err = GetLastError();
        if (err == ERROR_SERVICE_ALREADY_RUNNING) {
            return TRUE;
        } else {
            printf("StartService failure! Error = %d \n", err );
            return FALSE;
        }
    }

    if (schService) {
        CloseServiceHandle(schService);
    }
    return TRUE;
} 

BOOLEAN
StopDriver(
    __in SC_HANDLE    SchSCManager,
    __in LPCTSTR      DriverName
    )
{
    BOOLEAN         rCode = TRUE;
    SC_HANDLE       schService;
    SERVICE_STATUS  serviceStatus;

    schService = OpenService(SchSCManager,
                             DriverName,
                             SERVICE_ALL_ACCESS
                             );
    if (schService == NULL) {
        printf("OpenService failed!  Error = %d \n", GetLastError());
        return FALSE;
    }

    if (ControlService(schService,
                       SERVICE_CONTROL_STOP,
                       &serviceStatus
                       )) {
        rCode = TRUE;
    } else {
        printf("ControlService failed!  Error = %d \n", GetLastError() );
        rCode = FALSE;
    }
    if (schService) {

        CloseServiceHandle (schService);
    }
	return rCode;
}

BOOLEAN
SetupDriverName(
    __inout_bcount_full(BufferLength) PCHAR DriverLocation,
    __in ULONG BufferLength
    )
{
    HANDLE fileHandle;
    DWORD driverLocLen = 0;
    driverLocLen = GetCurrentDirectory(BufferLength,
                                       DriverLocation
                                       );
    if (driverLocLen == 0) {
        printf("GetCurrentDirectory failed!  Error = %d \n", GetLastError());
        return FALSE;
    }
    if (FAILED( StringCbCat(DriverLocation, BufferLength, "\\"DRIVER_NAME".sys") )) {
        return FALSE;
    }

    if ((fileHandle = CreateFile(DriverLocation,
                                 GENERIC_READ,
                                 0,
                                 NULL,
                                 OPEN_EXISTING,
                                 FILE_ATTRIBUTE_NORMAL,
                                 NULL
                                 )) == INVALID_HANDLE_VALUE) {

        printf("%s.sys is not loaded.\n", DRIVER_NAME);
        return FALSE;
    }
    if (fileHandle) {

        CloseHandle(fileHandle);
    }

    return TRUE;
} 