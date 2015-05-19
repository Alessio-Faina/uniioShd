#include <ntddk.h>          // various NT definitions
#include <string.h>
#include <WinDef.h>

#include "uniioctl.h"

#define ALLOCATE_NON_PAGED_MEMORY 1
#define DEBUG 1

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
#define POOL_TAG    'looP'
const int MEM_WIDTH = 4096*1024*16;

void*	userMem = NULL;
HANDLE 	hsection;
int AttachedProcesses = 0;


//Allocate the pages for the routines
#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, DriverEntry)
#endif // ALLOC_PRAGMA



//	Driver entry procedure
NTSTATUS DriverEntry(__in PDRIVER_OBJECT DriverObject, __in PUNICODE_STRING RegistryPath)
{
    NTSTATUS        		ntStatus;
    UNICODE_STRING  		ntUnicodeString;    
    UNICODE_STRING  		ntWin32NameString;    
    PDEVICE_OBJECT  		deviceObject = NULL;    // pointer to the instanced device object
	PDEVICE_DESCRIPTION 	devDes;
	PMDL					pMdl;
	
    UNREFERENCED_PARAMETER(RegistryPath);
    UNREFERENCED_PARAMETER(deviceObject);
		
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
#if !ALLOCATE_NON_PAGED_MEMORY
	userMem = ExAllocatePoolWithTag(NonPagedPool,
									MEM_WIDTH, 
									POOL_TAG );
																
    memset(userMem,0,4096);
	#if DEBUG
		((char*)userMem)[0]='K';	
	#endif //DEBUG

#endif


	
	deviceObject->Flags |= DO_DIRECT_IO;
	
    return ntStatus;
}

NTSTATUS ioctlCreate(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
#if ALLOCATE_NON_PAGED_MEMORY
	//PVOID   						virtualAddress = NULL;
	SIZE_T 							view_size = MEM_WIDTH;
	PHYSICAL_ADDRESS				phys_addr;
#endif //ALLOCATE_NON_PAGED_MEMORY	
	LARGE_INTEGER 					Li;
	UNICODE_STRING 					name;
	OBJECT_ATTRIBUTES 				oa;
    PIO_STACK_LOCATION  			pStack;
	NTSTATUS 						ntStatus = STATUS_SUCCESS;
	PMDL							Mdl;

	DbgPrint("UNIOCTL.SYS: Opening device");
	UNREFERENCED_PARAMETER(DeviceObject);

	pStack = IoGetCurrentIrpStackLocation(Irp);	
	AttachedProcesses++;
	
#if ALLOCATE_NON_PAGED_MEMORY
	if (AttachedProcesses==1)
	{
		Li.HighPart=0;
		Li.LowPart=MEM_WIDTH;
		
		DbgPrint("UNIOCTL.SYS: Mapping memory");
		RtlInitUnicodeString(&name, L"\\BaseNamedObjects\\Netmap");
		
		InitializeObjectAttributes(&oa, &name, OBJ_CASE_INSENSITIVE, (HANDLE)0, (PSECURITY_DESCRIPTOR)0);
																				
		ZwCreateSection(&hsection, 
						SECTION_ALL_ACCESS, 
						&oa, 
						&Li, 
						PAGE_READWRITE, 
						SEC_COMMIT, 
						0);	
		
		ZwMapViewOfSection(hsection, 						//SectionHandle 
							(HANDLE)-1,		 				//ProcessHandle 	
							&userMem,						//BaseAddress 
							0L, 							//ZeroBits 
							MEM_WIDTH, 						//CommitSize 
							NULL,							//SectionOffset 					
							&view_size, 					//ViewSize 
							ViewShare, 						//InheritDisposition 
							0, 								//AllocationType 
							PAGE_READWRITE | PAGE_NOCACHE);	//Win32Protect 
							
						
#if DEBUG							
		((char*)userMem)[0] = 't';
		((char*)userMem)[1] = 'e';
		((char*)userMem)[2] = 's';
		((char*)userMem)[3] = 't';
#endif DEBUG
		
		DbgPrint("userMem: %p\n",userMem);		
	}
#endif //ALLOCATE_NON_PAGED_MEMORY = TRUE
    Irp->IoStatus.Status = ntStatus;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    return ntStatus;	
}

NTSTATUS ioctlClose(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    PIO_STACK_LOCATION  pStack;
#if DEBUG
	char* memChar = NULL;
#endif //DEBUG
	
    DbgPrint("UNIOCTL.SYS: Closing device");
	pStack = IoGetCurrentIrpStackLocation(Irp);
	
#if DEBUG	
	memChar = (char*)userMem;
	DbgPrint("Address MEM[0] = %p",memChar[0]);
	DbgPrint("Value MEM[0] = %c",memChar[0]);
#endif //DEBUG			
			
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
	AttachedProcesses--;
	
#if ALLOCATE_NON_PAGED_MEMORY
	if (AttachedProcesses==0)
	{
		ZwClose(hsection);
	}
#endif //ALLOCATE_NON_PAGED_MEMORY
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}
	
VOID ioctlUnloadDriver(__in PDRIVER_OBJECT DriverObject)
{
#if DEBUG
	char* memChar = NULL;	
#endif //DEBUG	
	PDEVICE_OBJECT deviceObject = DriverObject->DeviceObject;
    UNICODE_STRING uniWin32NameString;
#if DEBUG
	memChar = (char*)userMem;
	DbgPrint("Address MEM[0] = %p",memChar[0]);
	DbgPrint("Value MEM[0] = %c",memChar[0]);
#endif //DEBUG	
	
	UNREFERENCED_PARAMETER(deviceObject);
#if !ALLOCATE_NON_PAGED_MEMORY
	ExFreePoolWithTag(userMem, POOL_TAG);
#endif	
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
    PMDL 				mdl = NULL;
	
    PVOID       		buffer = NULL;
	
	MEMORY_ENTRY		returnedValue;
	
	int*				mem = NULL;
	char*				memChar = NULL;
	void* 				UserVirtualAddress = NULL;
	
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
		#if !ALLOCATE_NON_PAGED_MEMORY
			NtStatus = STATUS_INVALID_DEVICE_REQUEST;	
			break;
		#endif
		inBufLength = irpSp->Parameters.DeviceIoControl.InputBufferLength;
		outBufLength = irpSp->Parameters.DeviceIoControl.OutputBufferLength;
		
		try
		{
			buffer = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
			#if DEBUG		
			DbgPrint("1-AssBuff: %c\n", ((char*)buffer)[0]);
			DbgPrint("1-userMem: %c\n", ((char*)userMem)[0]);
			#endif //DEBUG
			mdl = IoAllocateMdl( userMem,
									MEM_WIDTH, 
									FALSE, 
									FALSE, 
									NULL );
			MmBuildMdlForNonPagedPool(mdl);
			UserVirtualAddress = MmMapLockedPagesSpecifyCache(
											  mdl,
											  UserMode, 
											  MmNonCached,
											  NULL,
											  FALSE, 
											  NormalPagePriority);
			returnedValue.pBuffer = UserVirtualAddress;
			RtlCopyMemory(buffer,
						  &returnedValue,
						  sizeof(PVOID));
			#if DEBUG
			DbgPrint("2-AssBuff: %c\n", ((char*)buffer)[0]);
			DbgPrint("2-userMem: %c\n", ((char*)userMem)[0]);
			DbgPrint("2-UserVirtualAddress: %c\n", ((char*)UserVirtualAddress)[0]);						  
			#endif //DEBUG
			Irp->IoStatus.Information = sizeof(void*);	
		}
		except(EXCEPTION_EXECUTE_HANDLER)
		{
			NtStatus = STATUS_INSUFFICIENT_RESOURCES;
			Irp->IoStatus.Information = 0;
			DbgPrint("Failed to allocate memory!!!!!");
		}		
	}else if (irpSp->Parameters.DeviceIoControl.IoControlCode == IOCTL_TEST_WRITTEN_DATA){
		DbgPrint("Address MEM = %p",userMem);
		try
		{
			memChar = (char*)userMem;
			DbgPrint("Address MEM[0] = %p",memChar[0]);
			DbgPrint("Value MEM[0] = %c",memChar[0]);
			//DbgPrint("Written in [o] = %s",((char*) userMem)[0]);
			
			mem = (int*)userMem;
			DbgPrint("Address MEM[0] = %p",mem[0]);
			DbgPrint("Value MEM[0] = %i",mem[0]);
			
			/*DbgPrint("VirtualAddress[0] = %p",virtualAddress[0]);
			DbgPrint("VirtualAddress[1] = %p",virtualAddress[1]);
			DbgPrint("VirtualAddress[2] = %p",virtualAddress[2]);
			DbgPrint("VirtualAddress[3] = %p",virtualAddress[3]);*/
		}
		except(EXCEPTION_EXECUTE_HANDLER)
		{
			DbgPrint("Failed to read memory");
		}		
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

