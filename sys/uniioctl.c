#include <ntddk.h>          // various NT definitions
#include <string.h>
#include <WinDef.h>

#include "uniioctl.h"

// Device driver routines
DRIVER_INITIALIZE DriverEntry;

__drv_dispatchType(IRP_MJ_CREATE)
DRIVER_DISPATCH ioctlCreate;

__drv_dispatchType(IRP_MJ_CLOSE)
DRIVER_DISPATCH ioctlClose;

__drv_dispatchType(IRP_MJ_DEVICE_CONTROL)
DRIVER_DISPATCH ioctlDeviceControl;

__drv_dispatchType(IRP_MJ_READ)
DRIVER_DISPATCH ioctlRead;

__drv_dispatchType(IRP_MJ_WRITE)
DRIVER_DISPATCH ioctlWrite;

DRIVER_UNLOAD ioctlUnloadDriver;


VOID PrintIrpInfo(PIRP Irp);

//Tag for memory pool
#define POOL_TAG    'mteN'
const int MEM_WIDTH = 4096;

void*	userMem = NULL;
HANDLE 	hsection;

//Allocate the pages for the routines
#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, DriverEntry)
#endif // ALLOC_PRAGMA



//	Driver entry procedure
NTSTATUS DriverEntry(__in PDRIVER_OBJECT DriverObject, __in PUNICODE_STRING RegistryPath)
{
    NTSTATUS        ntStatus;
    UNICODE_STRING  ntUnicodeString;    
    UNICODE_STRING  ntWin32NameString;    
    PDEVICE_OBJECT  deviceObject = NULL;    // pointer to the instanced device object
	PDEVICE_DESCRIPTION devDes;
	int i;
	int* test;
	
    UNREFERENCED_PARAMETER(RegistryPath);
        
    RtlInitUnicodeString( &ntUnicodeString, NT_DEVICE_NAME );

    ntStatus = IoCreateDevice(
        DriverObject,                   // The Driver Object
        0,                              // 
        &ntUnicodeString,               //
        FILE_DEVICE_UNKNOWN,            // Device type
        FILE_DEVICE_SECURE_OPEN,     	// Device characteristics
        FALSE,                          // Not exclusive
        &deviceObject );                // Returned pointer to the device

    if ( !NT_SUCCESS( ntStatus ) )
    {
        DbgPrint("UNIIOCTL.SYS: Couldn't create the device object\n");
        return ntStatus;
    }
	DbgPrint("UNIIOCTL.SYS: Driver loaded at address 0x%p \n",&deviceObject);
	
    // Init function pointers to major driver functions
    DriverObject->MajorFunction[IRP_MJ_CREATE] = ioctlCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = ioctlClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ioctlDeviceControl;
    DriverObject->DriverUnload = ioctlUnloadDriver;

    // Initialize a Unicode String containing the Win32 name
    // for our device.
    RtlInitUnicodeString( &ntWin32NameString, DOS_DEVICE_NAME );

    // Symlink creation
    ntStatus = IoCreateSymbolicLink(&ntWin32NameString, &ntUnicodeString );

    if ( !NT_SUCCESS( ntStatus ) )
    {
		//Clear all in case of not success
        DbgPrint("UNIIOCTL.SYS: Couldn't create symbolic link\n");
        IoDeleteDevice( deviceObject );
    }
/*
	pMdl = IoAllocateMdl(NULL,
						 4096,
						 FALSE,
						 FALSE,
						 NULL);
	if(!pMdl) {
		DbgPrintEx(DPFLTR_IHVVIDEO_ID, DPFLTR_INFO_LEVEL, "Error on IoAllocateMdl. Returning from driver early.\n");
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	MmBuildMdlForNonPagedPool(pMdl);
	userMemory = (void *)MmMapLockedPagesSpecifyCache(pMdl, UserMode, MmWriteCombined, NULL, FALSE, LowPagePriority);
	*/
	
	userMem = ExAllocatePoolWithTag(NonPagedPool,
									MEM_WIDTH, 
									POOL_TAG );
      
	try
	{
		test = (int*)userMem;

		for (i=0; i<4096; i++)
		{
			test[i]=i;	
		}
		
	}
	except(EXCEPTION_EXECUTE_HANDLER)
	{
		DbgPrint("Failed to test");
	}
    
	deviceObject->Flags |= DO_DIRECT_IO;
	
    return ntStatus;
}

NTSTATUS ioctlCreate(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	LARGE_INTEGER Li;
	UNICODE_STRING name;
	OBJECT_ATTRIBUTES oa;
	SIZE_T j;

    PIO_STACK_LOCATION  pStack;
	NTSTATUS ntStatus = STATUS_SUCCESS;
	
		PVOID sec_obj;
		PSECURITY_DESCRIPTOR secure_desc;
		BOOLEAN allocated;
		SIZE_T view_size = 1500;
		HANDLE sec_handle;
	
	DbgPrint("UNIOCTL.SYS: Opening device");
		
	UNREFERENCED_PARAMETER(DeviceObject);

	pStack = IoGetCurrentIrpStackLocation(Irp);

	
	
	Li.HighPart=0;
	Li.LowPart=MEM_WIDTH;
	j=MEM_WIDTH;

	DbgPrint("UNIOCTL.SYS: Mapping memory");
	RtlInitUnicodeString(&name, L"\\BaseNamedObjects\\Netmap");
	/*InitializeObjectAttributes(&oa, &name, OBJ_KERNEL_HANDLE | 
											OBJ_FORCE_ACCESS_CHECK | 
											OBJ_CASE_INSENSITIVE |
											OBJ_OPENIF,
											(HANDLE)NULL,
											(PSECURITY_DESCRIPTOR)NULL);*/
	InitializeObjectAttributes(&oa, &name, 0,0,NULL);
																			
	ZwCreateSection(&hsection, SECTION_ALL_ACCESS, &oa, &Li, PAGE_READWRITE, SEC_COMMIT, NULL);
	/*ZwMapViewOfSection(hsection, NtCurrentProcess(), 
						&userMem, 0, MEM_WIDTH, NULL,
						&j, ViewShare, 0, PAGE_READWRITE);*/
	/*
	ObReferenceObjectByHandle(hsection, SECTION_ALL_ACCESS, NULL, Irp->RequestorMode, &sec_obj, 0);
	ntStatus = ObGetObjectSecurity(sec_obj, &secure_desc, &allocated);
	if(!NT_SUCCESS(ntStatus))
	{
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	RtlSetDaclSecurityDescriptor(secure_desc,FALSE,NULL,FALSE);
	ObReleaseObjectSecurity(secure_desc, allocated);					
	ObDereferenceObject(sec_obj);					
	*/			
	ZwMapViewOfSection(hsection, ZwCurrentProcess(), 
						&userMem, 0, MEM_WIDTH, NULL,
						&j, ViewShare, 0, PAGE_READWRITE | PAGE_NOCACHE);					
	//ZwClose( hsection );
	

#if 0	
	ObReferenceObjectByHandle
		(
		&sec_handle,
		SECTION_ALL_ACCESS,
		NULL,
		KernelMode,
		&sec_obj,
		0
		);
	ObGetObjectSecurity(sec_obj, &secure_desc, &allocated);	
	RtlSetDaclSecurityDescriptor(secure_desc, FALSE, NULL, FALSE);
	ObReleaseObjectSecurity(secure_desc, allocated);
	ObDereferenceObject(sec_obj);	
	ntStatus = ZwMapViewOfSection
		(
		sec_handle,
		ZwCurrentProcess(),
		&userMem,
		0L,
		view_size,
		NULL,
		&view_size,
		ViewUnmap,
		0,
		PAGE_READWRITE
		);

	// It is safe to close the section object handle once mapping has
	// been established. This way, we do not have to maintain the handle
	// or cleanup on teardown.
	ZwClose(sec_handle);
#endif
	
    Irp->IoStatus.Status = ntStatus;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    return ntStatus;	
}

NTSTATUS ioctlClose(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    PIO_STACK_LOCATION  pStack;

    DbgPrint("UNIOCTL.SYS: Closing device");

	pStack = IoGetCurrentIrpStackLocation(Irp);
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
	//ExFreePool(userMemory);
	ZwClose(hsection);
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}
	
VOID ioctlUnloadDriver(__in PDRIVER_OBJECT DriverObject)
{
	PDEVICE_OBJECT deviceObject = DriverObject->DeviceObject;
    UNICODE_STRING uniWin32NameString;

    // Create counted string version of our Win32 device name.
    RtlInitUnicodeString( &uniWin32NameString, DOS_DEVICE_NAME );

    // Delete the link from our device name to a name in the Win32 namespace.
    IoDeleteSymbolicLink( &uniWin32NameString );

    if ( deviceObject != NULL )
    {
        IoDeleteDevice( deviceObject );
    }
	return;
}

NTSTATUS ioctlDeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	PIO_STACK_LOCATION  irpSp;
    NTSTATUS            NtStatus = STATUS_SUCCESS;
    ULONG               inBufLength; 
    ULONG               outBufLength; 
    PCHAR               inBuf, outBuf;
    PMDL                mdl = NULL;
    PCHAR       		buffer = NULL;
	
	char*				mem = NULL;
	
	DbgPrint("UNIIOCTL.SYS: ioctlDeviceControl\n");
	
    UNREFERENCED_PARAMETER(DeviceObject);

    irpSp = IoGetCurrentIrpStackLocation( Irp );
   /* inBufLength = irpSp->Parameters.DeviceIoControl.InputBufferLength;
    outBufLength = irpSp->Parameters.DeviceIoControl.OutputBufferLength;
	inBuf = Irp->AssociatedIrp.SystemBuffer;
	outBuf = Irp->AssociatedIrp.SystemBuffer;*/
	
	if (irpSp->Parameters.DeviceIoControl.IoControlCode == IOCTL_METHOD_IN_DIRECT)
	{	

	}else if (irpSp->Parameters.DeviceIoControl.IoControlCode == IOCTL_METHOD_OUT_DIRECT)
	{
		
	}else if (irpSp->Parameters.DeviceIoControl.IoControlCode == IOCTL_METHOD_IN_NEITHER)
	{
		
	}else if (irpSp->Parameters.DeviceIoControl.IoControlCode == IOCTL_METHOD_OUT_NEITHER){
		
	}else if (irpSp->Parameters.DeviceIoControl.IoControlCode == IOCTL_MMAP){
		inBufLength = irpSp->Parameters.DeviceIoControl.InputBufferLength;
		outBufLength = irpSp->Parameters.DeviceIoControl.OutputBufferLength;
		//inBuf = Irp->AssociatedIrp.SystemBuffer;
		//mapToUser( userMemory, MEM_WIDTH);
		Irp->AssociatedIrp.SystemBuffer=userMem;
		
		/*buffer = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);	
		RtlCopyMemory(buffer,
						userMemory,
						sizeof(userMemory));*/			
		Irp->IoStatus.Information = sizeof(void*);	
		//DbgPrint("Copied in buffer address 0x%p\n", userMemory);
	}else if (irpSp->Parameters.DeviceIoControl.IoControlCode == IOCTL_TEST_WRITTEN_DATA){
		DbgPrint("Address MEM = %p",userMem);
		mem=(char*)userMem;
		DbgPrint("Address MEM[0] = %p",mem[0]);
		DbgPrint("Value MEM[0] = %c",mem[0]);
		//DbgPrint("Written in [o] = %s",((char*) userMem)[0]);
	}else{
		NtStatus = STATUS_INVALID_DEVICE_REQUEST;		
	}
	Irp->IoStatus.Status = NtStatus;
	IoCompleteRequest( Irp, IO_NO_INCREMENT );
	return NtStatus;	
}

VOID PrintIrpInfo(PIRP Irp)
{
    PIO_STACK_LOCATION  irpSp;
    irpSp = IoGetCurrentIrpStackLocation( Irp );

    //PAGED_CODE();

    DbgPrint("UNIIOCTL.SYS: \tIrp->AssociatedIrp.SystemBuffer = 0x%p\n",
        Irp->AssociatedIrp.SystemBuffer);
    DbgPrint("UNIIOCTL.SYS: Irp->UserBuffer = 0x%p\n", Irp->UserBuffer);
    DbgPrint("UNIIOCTL.SYS: irpSp->Parameters.DeviceIoControl.Type3InputBuffer = 0x%p\n",
        irpSp->Parameters.DeviceIoControl.Type3InputBuffer);
    DbgPrint("UNIIOCTL.SYS: irpSp->Parameters.DeviceIoControl.InputBufferLength = %d\n",
        irpSp->Parameters.DeviceIoControl.InputBufferLength);
    DbgPrint("UNIIOCTL.SYS: irpSp->Parameters.DeviceIoControl.OutputBufferLength = %d\n",
        irpSp->Parameters.DeviceIoControl.OutputBufferLength );
    return;
}
/*
PVOID CreateAndMapMemory(PMDL* PMemMdl,PVOID* UserVa)
{
    PMDL  mdl;
    PVOID userVAToReturn;
    PHYSICAL_ADDRESS lowAddress,
    PHYSICAL_ADDRESS highAddress,
    SIZE_T totalBytes;

 //
 // Initialize the Physical addresses need for MmAllocatePagesForMdl
 //
 lowAddress.QuadPart = 0;
 MAX_MEM(highAddress.QuadPart);
 totalBytes.QuadPart = PAGE_SIZE;
 
    //
    // Allocate a 4K buffer to share with the application
    //
    mdl = MmAllocatePagesForMdl(lowAddress,highAddress,lowAddress,totalBytes);
 
    if(!mdl) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // The preferred way to map the buffer into user space
    //
    userVAToReturn =
         MmMapLockedPagesSpecifyCache(mdl,          // MDL
                                      UserMode,     // Mode
                                      MmCached,     // Caching
                                      NULL,         // Address
                                      FALSE,        // Bugcheck?
                                      NormalPagePriority); // Priority

    //
    // If we get NULL back, the request didn't work.
    // I'm thinkin' that's better than a bug check anyday.
    //
    if(!userVAToReturn)  {

        MmFreePagesFromMdl(mdl);       
        IoFreeMdl(mdl);
        return STATUS_INSUFFICIENT_RESOURCES;
    }


    //
    // Return the allocated pointers
    //
    *UserVa = userVAToReturn;
    *PMemMdl = mdl;

    DbgPrint("UserVA = 0x%0x\n", userVAToReturn);

    return STATUS_SUCCESS;
}
*/
/*
void UnMapAndFreeMemory(PMDL PMdl,PVOID UserVa)
{
 //
 // Make sure we have an MDL to free
//
 if(!PMdl) {
  return;
 }
 
 //
 // Return the allocated resources.
 //
 MmUnmapLockedPages(UserVa, PMdl);
 
 MmFreePagesFromMdl(PMdl);       
 IoFreeMdl(PMdl);
 
 
}
*/

void *mapToUser( PHYSICAL_ADDRESS physicalAddress, ULONG memlen )
{
    UNICODE_STRING      memName;
    OBJECT_ATTRIBUTES   objectAttributes;
    HANDLE              hPhysMem;
    void                *physMemSect;
    PHYSICAL_ADDRESS    viewBase;
    UCHAR               *virtualAddress;
    ULONG               length;
    NTSTATUS            status;

   /* if( userAddrIdx == N_USER_ADDRESSES )
        return 0; // No more room to store address for unmapping at exit.*/

    memset( &objectAttributes, 0, sizeof( objectAttributes ));
    RtlInitUnicodeString( &memName, L"\\Device\\PhysicalMemory" );
    InitializeObjectAttributes( 
      &objectAttributes, 
      &memName,
      OBJ_CASE_INSENSITIVE, 
      (HANDLE)0, 
      (PSECURITY_DESCRIPTOR)0 );
    
    virtualAddress = 0;
    status = ZwOpenSection( 
      &hPhysMem, 
      SECTION_ALL_ACCESS,
      &objectAttributes );
    if( status == STATUS_SUCCESS )
    {   // ZwOpenSection OK.
        status = ObReferenceObjectByHandle( 
          hPhysMem,
          SECTION_ALL_ACCESS, 
          (POBJECT_TYPE)0, 
          KernelMode, 
          &physMemSect,
          (POBJECT_HANDLE_INFORMATION)0 );
        if( status == STATUS_SUCCESS )
        {   // ObReferenceObjectByHandle OK.
            viewBase = physicalAddress;
            length = memlen;
            status = ZwMapViewOfSection(
              hPhysMem,         // IN HANDLE SectionHandle
              (HANDLE)-1,       // IN HANDLE ProcessHandle (Any/current?).
              &virtualAddress,  // IN OUT PVOID *BaseAddress
              0L,               // IN ULONG ZeroBits
              length,           // IN ULONG CommitSize
              &viewBase,        // IN OUT PLARGE_INTEGER SectionOffset
              &length,          // IN OUT PULONG ViewSize
              ViewShare,        // IN SECTION_INHERIT InheritDisposition
              0,                // IN ULONG AllocationType
              PAGE_READWRITE ); // IN ULONG Protect.
            if( status == STATUS_SUCCESS )
            {   // ZwMapViewOfSection OK.
                virtualAddress += physicalAddress.LowPart - viewBase.LowPart;
               // userAddresses[ userAddrIdx++ ] = virtualAddress;  // Save for unmapping.
            }
            else
                virtualAddress = 0;
        }   // ObReferenceObjectByHandle OK.
        ZwClose( hPhysMem );
    }   // ZwOpenSection OK.
    return virtualAddress;
};