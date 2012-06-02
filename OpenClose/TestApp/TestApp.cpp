#include <stdio.h>
#include <conio.h>
#include <Windows.h>

int __cdecl main(int argc,
	char* argv[])
{
	HANDLE  hDevice;
	TCHAR szNotepadPath[MAX_PATH];
	STARTUPINFO StartupInfo = {0};
	PROCESS_INFORMATION ProcessInfo = {0};

	hDevice = CreateFile("\\\\.\\CleanUp",
		GENERIC_READ,
		0,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);

	__try
	{
		if (hDevice == INVALID_HANDLE_VALUE)
		{
			printf("Erro #%d ao abrir device.\n",
				GetLastError());
		}

		printf("Device aberto com sucesso!\n");
		GetWindowsDirectory(szNotepadPath,sizeof(szNotepadPath));

		strcat_s(szNotepadPath,sizeof(szNotepadPath),"\\notepad.exe");

		if (!CreateProcess(NULL, 
			szNotepadPath, 
			NULL,
			NULL,
			FALSE,
			0,
			NULL,
			NULL, 
			&StartupInfo, 
			&ProcessInfo)) {
				printf("Erro ao criar o processo #%d\n", GetLastError());
		}

		printf ("Processo criado com sucesso\n");
		printf ("Tecle algo para duplicar o handle\n");

		_getch();
	}
	__finally
	{
		if (ProcessInfo.hProcess)
			CloseHandle(ProcessInfo.hProcess);

		if (ProcessInfo.hThread)
			CloseHandle(ProcessInfo.hThread);

		if (hDevice)
			CloseHandle(hDevice);
	}

	return 0;
}
