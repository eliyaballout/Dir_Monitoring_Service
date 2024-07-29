#include <windows.h>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <filesystem>

#define SERVICE_NAME "DirMonService"



SERVICE_STATUS ServiceStatus = { 0 };
SERVICE_STATUS_HANDLE hStatus = NULL;
HANDLE hDirectory = INVALID_HANDLE_VALUE;
std::string directoryToMonitor;
std::string logFilePath;
HANDLE hStopEvent = NULL;
HANDLE hPauseEvent = NULL;
HANDLE hResumeEvent = NULL;



void WriteToLog(const std::string& message) {
	std::ofstream logFile;
	logFile.open(logFilePath, std::ios_base::app);
	if (logFile.is_open()) {
		SYSTEMTIME time;
		GetLocalTime(&time);
		logFile << time.wYear << "-"
			<< time.wMonth << "-"
			<< time.wDay << " "
			<< time.wHour << ":"
			<< time.wMinute << ":"
			<< time.wSecond << " - "
			<< message << std::endl;
		logFile.close();
	}
}


void MonitorDirectory() {
	char buffer[1024];
	DWORD bytesReturned;
	FILE_NOTIFY_INFORMATION* pNotify;
	std::string action;
	std::wstring tmpFileName;

	std::wstring directoryPath = std::wstring(directoryToMonitor.begin(), directoryToMonitor.end());

	while (WaitForSingleObject(hStopEvent, 0) == WAIT_TIMEOUT) {
		if (ServiceStatus.dwCurrentState == SERVICE_PAUSED) {
			// If paused, wait until the event is reset
			WaitForSingleObject(hResumeEvent, INFINITE);
			continue;
		}

		if (ReadDirectoryChangesW(hDirectory, buffer, sizeof(buffer), TRUE,
			FILE_NOTIFY_CHANGE_FILE_NAME |
			FILE_NOTIFY_CHANGE_DIR_NAME |
			FILE_NOTIFY_CHANGE_ATTRIBUTES |
			FILE_NOTIFY_CHANGE_SIZE |
			FILE_NOTIFY_CHANGE_LAST_WRITE |
			FILE_NOTIFY_CHANGE_LAST_ACCESS |
			FILE_NOTIFY_CHANGE_CREATION |
			FILE_NOTIFY_CHANGE_SECURITY,
			&bytesReturned, NULL, NULL)) {
			if (ServiceStatus.dwCurrentState != SERVICE_RUNNING)
				continue;
			
			pNotify = (FILE_NOTIFY_INFORMATION*)buffer;
			do {
				std::string action;
				bool isDirectory = false;
				bool isFile = true;

				int fileNameLength = pNotify->FileNameLength / sizeof(WCHAR);
				std::wstring fileName(pNotify->FileName, fileNameLength);

				// Construct the full path
				std::wstring fullPath = directoryPath + L"\\" + fileName;		
				
				// Detect if it's a directory action
				DWORD attributes = GetFileAttributesW(fullPath.c_str());

				if (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY)) {
					isDirectory = true;
					isFile = false;
				}
				else {
					isDirectory = false;
					isFile = true;
				}

				switch (pNotify->Action) {
				case FILE_ACTION_ADDED:
					action = (isDirectory && !isFile) ? "Directory added: " : "File added: ";
					break;
				
				case FILE_ACTION_REMOVED:
					action = (isDirectory && !isFile) ? "Directory removed: " : "File removed: ";
					break;
				
				case FILE_ACTION_MODIFIED:
					action = (isDirectory && !isFile) ? "" : "File modified: ";
					break;
				
				case FILE_ACTION_RENAMED_OLD_NAME:
					tmpFileName = fileName;
					break;
				
				case FILE_ACTION_RENAMED_NEW_NAME:
					if (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY)) {
						WriteToLog("Directory renamed from: " + std::string(tmpFileName.begin(), tmpFileName.end()));
						WriteToLog("Directory renamed to: " + std::string(fileName.begin(), fileName.end()));
					}
					else {
						WriteToLog("File renamed from: " + std::string(tmpFileName.begin(), tmpFileName.end()));
						WriteToLog("File renamed to: " + std::string(fileName.begin(), fileName.end()));
					}
					break;
				
				default:
					action = "Unknown action: ";
					break;
				}

				if (!action.empty() && pNotify->Action != FILE_ACTION_RENAMED_OLD_NAME && pNotify->Action != FILE_ACTION_RENAMED_NEW_NAME) {
					WriteToLog(action + std::string(fileName.begin(), fileName.end()));
				}

				pNotify = pNotify->NextEntryOffset
					? (FILE_NOTIFY_INFORMATION*)((LPBYTE)pNotify + pNotify->NextEntryOffset)
					: NULL;
			} while (pNotify);
		}
	}
}


void WINAPI ServiceCtrlHandler(DWORD controlCode) {
	switch (controlCode) {
	case SERVICE_CONTROL_STOP:
	case SERVICE_CONTROL_SHUTDOWN:
		if (ServiceStatus.dwCurrentState != SERVICE_RUNNING && ServiceStatus.dwCurrentState != SERVICE_PAUSED)
			break;

		ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
		SetServiceStatus(hStatus, &ServiceStatus);
		SetEvent(hStopEvent);

		ServiceStatus.dwCurrentState = SERVICE_STOPPED;
		SetServiceStatus(hStatus, &ServiceStatus);
		return;

	case SERVICE_CONTROL_PAUSE:
		if (ServiceStatus.dwCurrentState != SERVICE_RUNNING)
			break;

		// Report pause pending status
		ServiceStatus.dwCurrentState = SERVICE_PAUSE_PENDING;
		SetServiceStatus(hStatus, &ServiceStatus);

		// resetting the resume event to false
		ResetEvent(hResumeEvent);

		// setting the pause event to true
		SetEvent(hPauseEvent);

		// closing the handle for the directory 
		CloseHandle(hDirectory);

		// Report paused status
		ServiceStatus.dwCurrentState = SERVICE_PAUSED;
		SetServiceStatus(hStatus, &ServiceStatus);
		return;

	case SERVICE_CONTROL_CONTINUE:
		if (ServiceStatus.dwCurrentState != SERVICE_PAUSED)
			break;

		// Report continue pending status
		ServiceStatus.dwCurrentState = SERVICE_CONTINUE_PENDING;
		SetServiceStatus(hStatus, &ServiceStatus);

		hDirectory = CreateFile(directoryToMonitor.c_str(), FILE_LIST_DIRECTORY,
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL);

		if (hDirectory == INVALID_HANDLE_VALUE) {
			CloseHandle(hStopEvent);
			CloseHandle(hPauseEvent);
			CloseHandle(hResumeEvent);
			ServiceStatus.dwCurrentState = SERVICE_STOPPED;
			ServiceStatus.dwWin32ExitCode = GetLastError();
			SetServiceStatus(hStatus, &ServiceStatus);
			return;
		}

		// resetting the pause event to false
		ResetEvent(hPauseEvent);

		// setting the resume event to true
		SetEvent(hResumeEvent);

		// Report running status
		ServiceStatus.dwCurrentState = SERVICE_RUNNING;
		SetServiceStatus(hStatus, &ServiceStatus);
		return;

	default:
		break;
	}

	SetServiceStatus(hStatus, &ServiceStatus);
}


void WINAPI ServiceMain(DWORD argc, LPTSTR* argv) {
	hStatus = RegisterServiceCtrlHandler(SERVICE_NAME, ServiceCtrlHandler);

	if (hStatus == NULL) {
		return;
	}

	ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	ServiceStatus.dwControlsAccepted = 0;
	ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
	ServiceStatus.dwWin32ExitCode = NO_ERROR;
	ServiceStatus.dwWaitHint = 3000;
	SetServiceStatus(hStatus, &ServiceStatus);

	// stop event
	hStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (hStopEvent == NULL) {
		ServiceStatus.dwCurrentState = SERVICE_STOPPED;
		ServiceStatus.dwWin32ExitCode = GetLastError();
		SetServiceStatus(hStatus, &ServiceStatus);
		return;
	}

	// pause event
	hPauseEvent = CreateEvent(NULL, TRUE, FALSE, NULL); // Initially set
	if (hPauseEvent == NULL) {
		CloseHandle(hStopEvent);
		ServiceStatus.dwCurrentState = SERVICE_STOPPED;
		ServiceStatus.dwWin32ExitCode = GetLastError();
		SetServiceStatus(hStatus, &ServiceStatus);
		return;
	}

	// resume event
	hResumeEvent = CreateEvent(NULL, TRUE, FALSE, NULL); // Initially set
	if (hPauseEvent == NULL) {
		CloseHandle(hStopEvent);
		CloseHandle(hPauseEvent);
		ServiceStatus.dwCurrentState = SERVICE_STOPPED;
		ServiceStatus.dwWin32ExitCode = GetLastError();
		SetServiceStatus(hStatus, &ServiceStatus);
		return;
	}

	if (argc < 3) {
		CloseHandle(hStopEvent);
		CloseHandle(hPauseEvent);
		CloseHandle(hResumeEvent);
		ServiceStatus.dwCurrentState = SERVICE_STOPPED;
		ServiceStatus.dwWin32ExitCode = ERROR_INVALID_PARAMETER;
		SetServiceStatus(hStatus, &ServiceStatus);
		return;
	}

	directoryToMonitor = argv[1];
	logFilePath = argv[2];

	hDirectory = CreateFile(directoryToMonitor.c_str(), FILE_LIST_DIRECTORY,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL);

	if (hDirectory == INVALID_HANDLE_VALUE) {
		CloseHandle(hStopEvent);
		CloseHandle(hPauseEvent);
		CloseHandle(hResumeEvent);
		ServiceStatus.dwCurrentState = SERVICE_STOPPED;
		ServiceStatus.dwWin32ExitCode = GetLastError();
		SetServiceStatus(hStatus, &ServiceStatus);
		return;
	}

	ServiceStatus.dwCurrentState = SERVICE_RUNNING;
	ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_PAUSE_CONTINUE;
	ServiceStatus.dwCheckPoint = 0;
	ServiceStatus.dwWaitHint = 0;
	SetServiceStatus(hStatus, &ServiceStatus);

	MonitorDirectory();

	CloseHandle(hDirectory);
	CloseHandle(hStopEvent);
	CloseHandle(hPauseEvent);
	CloseHandle(hResumeEvent);
	ServiceStatus.dwCurrentState = SERVICE_STOPPED;
	ServiceStatus.dwWin32ExitCode = NO_ERROR;
	SetServiceStatus(hStatus, &ServiceStatus);
}



int main() {
	SERVICE_TABLE_ENTRY ServiceTable[] = {
		{ (LPSTR)SERVICE_NAME, (LPSERVICE_MAIN_FUNCTION)ServiceMain },
		{ NULL, NULL }
	};

	StartServiceCtrlDispatcher(ServiceTable);

	return 0;
}