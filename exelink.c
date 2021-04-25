#include <Windows.h>
#include <Shlwapi.h>
#include <strsafe.h>
#include <Pathcch.h>
#include <stdio.h>

#define PATH_SIZE 32768

static BOOL WINAPI ConsoleCtrlHandler(const DWORD dwCtrlType)
{
	(void)dwCtrlType;
	return TRUE;
}

static BOOL GetTargetProcessPath(PWSTR pszPathOut, const size_t cchPathOut)
{
	static const WCHAR cszTargetDirectory[PATH_SIZE] = L"..\\";

	WCHAR szFilePath[PATH_SIZE];
	DWORD dwFilePathLength = _countof(szFilePath);
	if (!QueryFullProcessImageNameW(GetCurrentProcess(), 0, szFilePath, &dwFilePathLength))
		return FALSE;

	WCHAR szFileName[PATH_SIZE];
	if (FAILED(StringCchCopyW(szFileName, _countof(szFileName), PathFindFileNameW(szFilePath))))
		return FALSE;

	if (FAILED(PathCchRemoveFileSpec(szFilePath, _countof(szFilePath))))
		return FALSE;

	WCHAR szTargetFileDir[PATH_SIZE];
	if (FAILED(PathCchCombine(szTargetFileDir, _countof(szTargetFileDir), szFilePath, cszTargetDirectory)))
		return FALSE;

	if (FAILED(PathCchCombine(pszPathOut, cchPathOut, szTargetFileDir, szFileName)))
		return FALSE;
	return TRUE;
}

static BOOL AddPathToEnvironmentVariables(const PCWSTR pszDirectory)
{
	WCHAR szPathVariable[32768];
	if (FAILED(StringCchCopyW(szPathVariable, _countof(szPathVariable), pszDirectory)))
		return FALSE;

	size_t cchDirectoryLength = 0;
	if (FAILED(StringCchLengthW(szPathVariable, _countof(szPathVariable), &cchDirectoryLength)))
		return FALSE;

	if (cchDirectoryLength + 1 >= _countof(szPathVariable))
		return FALSE;

	szPathVariable[cchDirectoryLength++] = L';';

	if (GetEnvironmentVariableW(
		L"Path", szPathVariable + cchDirectoryLength, (DWORD)(_countof(szPathVariable) - cchDirectoryLength)) == 0)
		return FALSE;

	return SetEnvironmentVariableW(L"Path", szPathVariable);
}

void ErrorExit()
{
	const DWORD dwErrorCode = GetLastError();

	fputws(L"Couldn't start target process.", stderr);

	if (dwErrorCode != 0)
	{
		PWSTR pszBuffer = NULL;
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
			NULL,
			dwErrorCode,
			0,
			(PWSTR)&pszBuffer,
			0,
			NULL);
		fwprintf(stderr, L" Error message: %s", pszBuffer);
		LocalFree(pszBuffer);

		ExitProcess(dwErrorCode);
	}

	ExitProcess(1u);
}

int main()
{
	WCHAR szTargetFilePath[PATH_SIZE];
	if (!GetTargetProcessPath(szTargetFilePath, _countof(szTargetFilePath)))
		ErrorExit();

	// Target process command line
	WCHAR szTargetCommandLine[32768];
	const PWSTR pszArgs = PathGetArgsW(GetCommandLineW());
	if (FAILED(StringCchPrintfW(
		szTargetCommandLine, _countof(szTargetCommandLine), L"\"%s\" %s", szTargetFilePath, pszArgs)))
		ErrorExit();

	// Setup environment variables
	if (FAILED(PathCchRemoveFileSpec(szTargetFilePath, _countof(szTargetFilePath))))
		ErrorExit();

	if (!AddPathToEnvironmentVariables(szTargetFilePath))
		ErrorExit();

	// Disable CTRL+C/CTRL+BREAK interrupt
	if (!SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE))
		ErrorExit();

	// Run the target process
	STARTUPINFOW si = {sizeof(si)};
	PROCESS_INFORMATION pi = {0};

	if (!CreateProcessW(NULL, szTargetCommandLine, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
		ErrorExit();

	// Waiting for the target process
	WaitForSingleObject(pi.hProcess, INFINITE);

	DWORD dwExitCode = (DWORD)-1;
	GetExitCodeProcess(pi.hProcess, &dwExitCode);

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	return dwExitCode;
}
