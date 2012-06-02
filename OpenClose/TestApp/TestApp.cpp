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

	char lpBuffer[100];
	DWORD  lpNumberOfBytes;
	

	hDevice = CreateFile("\\\\.\\CleanUp",
		GENERIC_WRITE | GENERIC_READ,
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
/*

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
				__leave;
		}

		DuplicateHandle(GetCurrentProcess(),hDevice, &ProcessInfo, NULL,NULL, FALSE, NULL);
		
		printf ("Processo criado com sucesso\n");


	printf ("Tecle algo para duplicar o handle\n");

		_getch();
		if (!DuplicateHandle(GetCurrentProcess()s,hDevice, ProcessInfo.hProcess, NULL,NULL, FALSE, NULL)) {
			printf("Erro ao duplicar o Handle #%d\n", GetLastError());
			__leave;
		}
*/

		char *msg = "Ola mundo";

		if(!WriteFile(hDevice, msg,strlen(msg), &lpNumberOfBytes, NULL)) {
			printf("Erro ao write device #%d\n", GetLastError());
			__leave;			
		}

		if (!ReadFile(hDevice, lpBuffer, 100, &lpNumberOfBytes, NULL)) {
			printf("Erro ao ler device #%d\n", GetLastError());
			__leave;
		}

		printf("[%s], lido do device\n", lpBuffer);

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

	printf ("Tecle algo para finalizar\n");
	_getch();
	return 0;
}
