#include"DiskOperation.h"
const GUID EFI_SYSTEM_PARTITION_GUID = { 0xC12A7328, 0xF81F, 0x11D2, {0xBA, 0x4B, 0x00, 0xA0, 0xC9, 0x3E, 0xC9, 0x3B} };
const GUID PARTITION_BASIC_DATA_GUID = { 0xEBD0A0A2, 0xB9E5, 0x4433, {0x87, 0xC0, 0x68, 0xB6, 0xB7, 0x26, 0x99, 0xC7} };
using namespace std;
struct MBR {
	unsigned char codeArea[446]; // ������������
	struct Partition {
		unsigned char status;    // ����״̬
		unsigned char start_head; // ��ʼ��ͷ��
		unsigned char start_sec;  // ��ʼ������
		unsigned char start_cyl;  // ��ʼ�����
		unsigned char type;       // ��������
		unsigned char end_head;   // ������ͷ��
		unsigned char end_sec;    // ����������
		unsigned char end_cyl;    // ���������
		unsigned int start_lba;   // ������ʼ��ַ��LBA
		unsigned int size;        // ������С������������ʾ
	} partition[4];
	unsigned short signature; // MBRǩ����0xAA55��
};
BOOL SCSIBuild16CDB(PSCSI_PASS_THROUGH_DIRECT srb, ULONGLONG offset, ULONG length, BOOLEAN Write) {
	if (!srb || offset >= 0x20000000000 || length < 1)
		return FALSE;
	LPBYTE cdb = srb->Cdb;
	if (Write == FALSE) {
		cdb[0] = SCSIOP_READ16;
		cdb[1] = 0;
	}
	else {
		cdb[0] = SCSIOP_WRITE16;
		cdb[1] = 0;
	}
	ULONGLONG LBA = offset / PHYSICAL_SECTOR_SIZE;
	cdb[2] = ((LPBYTE)&LBA)[7];
	cdb[3] = ((LPBYTE)&LBA)[6];
	cdb[4] = ((LPBYTE)&LBA)[5];
	cdb[5] = ((LPBYTE)&LBA)[4];
	cdb[6] = ((LPBYTE)&LBA)[3];
	cdb[7] = ((LPBYTE)&LBA)[2];
	cdb[8] = ((LPBYTE)&LBA)[1];
	cdb[9] = ((LPBYTE)&LBA)[0];

	UINT32 CDBTLen = length / PHYSICAL_SECTOR_SIZE;
	cdb[10] = ((LPBYTE)&CDBTLen)[3];
	cdb[11] = ((LPBYTE)&CDBTLen)[2];
	cdb[12] = ((LPBYTE)&CDBTLen)[1];
	cdb[13] = ((LPBYTE)&CDBTLen)[0];
	cdb[14] = 0x00;
	cdb[15] = 0x00;
	return TRUE;
}

BOOL SCSISectorIO(HANDLE hDrive, ULONGLONG offset, LPBYTE buffer, UINT buffSize, BOOLEAN write) {
	SCSI_PASS_THROUGH_DIRECT srb = { 0 };
	DWORD bytesReturned = 0;
	IO_SCSI_CAPABILITIES scap = { 0 };
	DWORD maxTransfLen = 8192;//4kb
	DWORD curSize = buffSize;
	LPBYTE tempBuff = NULL;
	static bool OneShotLog = false;

	BOOL retVal = 0;
	DWORD lastErr = 0;
	if (!buffer || !buffSize) return FALSE;

	retVal = DeviceIoControl(hDrive, IOCTL_SCSI_GET_CAPABILITIES, NULL, 0, &scap, sizeof(scap), &bytesReturned, NULL);
	lastErr = GetLastError();
	if (retVal)
		maxTransfLen = scap.MaximumTransferLength;

	RtlZeroMemory(&srb, sizeof(SCSI_PASS_THROUGH_DIRECT));
	srb.Length = sizeof(SCSI_PASS_THROUGH_DIRECT);
	srb.CdbLength = 0x10;
	srb.SenseInfoLength = 0;
	srb.SenseInfoOffset = sizeof(SCSI_PASS_THROUGH_DIRECT);
	if (write)
		srb.DataIn = SCSI_IOCTL_DATA_OUT;
	else
		srb.DataIn = SCSI_IOCTL_DATA_IN;
	srb.TimeOutValue = 0x100;

	while (curSize) {
		if (curSize > maxTransfLen)
			srb.DataTransferLength = maxTransfLen;
		else {

			if ((curSize % PHYSICAL_SECTOR_SIZE) != 0)

				curSize = curSize + (PHYSICAL_SECTOR_SIZE - (curSize % PHYSICAL_SECTOR_SIZE));
			srb.DataTransferLength = curSize;
		}

		srb.DataBuffer = buffer;
		retVal = SCSIBuild16CDB(&srb, offset, srb.DataTransferLength, write);
		retVal = DeviceIoControl(hDrive, IOCTL_SCSI_PASS_THROUGH_DIRECT, (LPVOID)&srb, sizeof(SCSI_PASS_THROUGH_DIRECT),
			NULL, 0, &bytesReturned, NULL);
		lastErr = GetLastError();

		if (!retVal) break;
		else lastErr = 0;
		buffer += srb.DataTransferLength;
		curSize -= srb.DataTransferLength;
		offset += srb.DataTransferLength;
	}

	if (lastErr != ERROR_SUCCESS) {
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}

/*��ȡ���������*/
BOOL GetPhysicalDriveHandle(HANDLE& handle, int DriveNumber)
{
	char drivestr[512];
	_snprintf_s(drivestr, 512, _TRUNCATE,_c("\\\\.\\PhysicalDrive%d"), DriveNumber);
	//open it
	handle = CreateFileA(drivestr, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING | FILE_FLAG_RANDOM_ACCESS, NULL);
	if (handle == INVALID_HANDLE_VALUE) {
		return FALSE;
	}
	return true;
}
/*SCSI��д����*/
BOOL DiskSectorIO(HANDLE hDrive, int Id, int num, unsigned char* buffer, BOOL write) {
	if (!hDrive || !buffer || num < 1) return FALSE;

	LARGE_INTEGER liOffset = { 0 };
	ULONGLONG ullStartSector = (ULONGLONG)Id;
	liOffset.QuadPart = ullStartSector * PHYSICAL_SECTOR_SIZE;

	BOOL bRet = SCSISectorIO(hDrive, liOffset.QuadPart, buffer, num, write);
	return bRet;
}

/*��ȡ��������������*/
int GetPhysicalDriveNumber()
{
	int drivenumber;
	HANDLE hFile = NULL;
	char drivestr[512] = { 0 };
	for (drivenumber = 0;; drivenumber++)
	{
		_snprintf_s(drivestr, 512, _TRUNCATE, _c("\\\\.\\PhysicalDrive%d"), drivenumber);
		hFile = CreateFileA(drivestr, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING | FILE_FLAG_RANDOM_ACCESS, NULL);
		if (hFile == INVALID_HANDLE_VALUE) {
			return drivenumber;
			break;
		}
		CloseHandle(hFile);
	}
	return drivenumber;
}
/*��ȡӲ�̷�������*/
const char* GetPartitiontype(HANDLE hDevice)
{
		int nDiskBufferSize = sizeof(PARTITION_INFORMATION_EX);
		PARTITION_INFORMATION_EX* lpDiskPartinfo_ex = (PARTITION_INFORMATION_EX*)malloc(nDiskBufferSize);
		DWORD nDiskBytesRead = 0;
		memset(lpDiskPartinfo_ex, 0, sizeof(lpDiskPartinfo_ex));
		BOOL Ret = FALSE;
		Ret = DeviceIoControl(hDevice, IOCTL_DISK_GET_PARTITION_INFO_EX, NULL, 0, lpDiskPartinfo_ex, nDiskBufferSize, &nDiskBytesRead, NULL);

		if (!Ret)
		{
			int e = GetLastError();
			return strerror(e);
		}
		if (lpDiskPartinfo_ex->PartitionStyle == PARTITION_STYLE_MBR)
		{
			return "MBR";
		}
		else if (lpDiskPartinfo_ex->PartitionStyle == PARTITION_STYLE_GPT)
		{
			return "GPT";
		}
		else if (lpDiskPartinfo_ex->PartitionStyle == PARTITION_STYLE_RAW)
		{
			return "RAW";
		}
		free(lpDiskPartinfo_ex);
		lpDiskPartinfo_ex = NULL;
}
int GetSystemDiskPhysicalNumber(void)
{
	CHAR sysPath[256];
	CHAR diskLetter;
	HANDLE hDevice;
	BOOL result;
	DWORD readed;
	STORAGE_DEVICE_NUMBER number;
	int diskNumber;

	DWORD ret = GetSystemDirectory(sysPath, sizeof(sysPath));
	if (ret == 0)
	{
		return -1;
	}

	diskLetter = sysPath[0];
	CHAR path[256];
	sprintf(path, "\\\\.\\%c:", diskLetter);
	hDevice = CreateFileA(path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (hDevice == INVALID_HANDLE_VALUE)
	{
		return -1;
	}

	result = DeviceIoControl(hDevice, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0, &number, sizeof(number), &readed, NULL);
	if (!result)
	{
		(void)CloseHandle(hDevice);
		return -1;
	}

	(void)CloseHandle(hDevice);
	diskNumber = number.DeviceNumber;
	return diskNumber;
}
int SetDriveLetterToEFI(const char* devicePath, char driveLetter)
{
	HANDLE hDevice;
	PARTITION_INFORMATION_EX partitionInfo;
	DWORD bytesReturned;
	char volume_name[MAX_PATH];

	// �򿪷����豸
	hDevice = CreateFileA(devicePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (hDevice == INVALID_HANDLE_VALUE)
	{
		return GetLastError();
	}

	// ��ȡ������Ϣ
	if (!DeviceIoControl(hDevice,
		IOCTL_DISK_GET_PARTITION_INFO_EX,
		NULL,
		0,
		&partitionInfo,
		sizeof(PARTITION_INFORMATION_EX),
		&bytesReturned,
		NULL))
	{
		CloseHandle(hDevice);
		return GetLastError();
	}

	SET_PARTITION_INFORMATION_EX setPartitionInfo;
	setPartitionInfo.PartitionStyle = PARTITION_STYLE_GPT;
	setPartitionInfo.Gpt.Attributes = 0;
	setPartitionInfo.Gpt.PartitionType = PARTITION_BASIC_DATA_GUID;
	setPartitionInfo.Gpt.PartitionId = partitionInfo.Gpt.PartitionId;
	if (!DeviceIoControl(hDevice, IOCTL_DISK_SET_PARTITION_INFO_EX, &setPartitionInfo, sizeof(setPartitionInfo), nullptr, 0, &bytesReturned, nullptr)) {
		CloseHandle(hDevice);
		return GetLastError();
	}

	// ��ȡ�����������
	if (!DeviceIoControl(hDevice,
		IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
		NULL,
		0,
		&volume_name,
		MAX_PATH,
		&bytesReturned,
		NULL))
	{
		CloseHandle(hDevice);
		return GetLastError();
	}
	// ��ʽ������GUID
	WCHAR guidStr[MAX_PATH] = { 0 };
	StringCchPrintfW(guidStr, MAX_PATH, L"%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
		partitionInfo.Gpt.PartitionId.Data1,
		partitionInfo.Gpt.PartitionId.Data2,
		partitionInfo.Gpt.PartitionId.Data3,
		partitionInfo.Gpt.PartitionId.Data4[0],
		partitionInfo.Gpt.PartitionId.Data4[1],
		partitionInfo.Gpt.PartitionId.Data4[2],
		partitionInfo.Gpt.PartitionId.Data4[3],
		partitionInfo.Gpt.PartitionId.Data4[4],
		partitionInfo.Gpt.PartitionId.Data4[5],
		partitionInfo.Gpt.PartitionId.Data4[6],
		partitionInfo.Gpt.PartitionId.Data4[7]);

	// �������·��
	WCHAR path[MAX_PATH] = { 0 };
	swprintf_s(path, MAX_PATH, L"\\\\?\\Volume{%s}\\", guidStr);

	// ������������
	std::wstring wstrDriveLetter(1, static_cast<wchar_t>(driveLetter));
	wstrDriveLetter += L":\\";
	DWORD error = SetVolumeMountPointW(wstrDriveLetter.c_str(), path);
	if (error == 0)
	{
		CloseHandle(hDevice);
		return GetLastError();
	}

	CloseHandle(hDevice);
	return 0;
}