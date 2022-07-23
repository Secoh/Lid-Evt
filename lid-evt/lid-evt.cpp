// Copyright 2022 (c) Secoh
//
// This program detects the event of closing lid (of a laptop) - feature dearly missed since Windows Vista. It can just
// lock the screen, or run a script in response to the closed lid, like the event manager in Windows.
// This program is for Windows platform only, so it is MSVC-only environment.
//
// Based on Laplock (c) 2011, Etienne Dechamps (e-t172) <e-t172@akegroup.org>
// Reference: https://github.com/dechamps/laplock
//
// This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later
// version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
// details. You should have received a copy of the GNU General Public License along with this program.  If not, see
// http://www.gnu.org/licenses/
//

#define _CRT_SECURE_NO_WARNINGS

#include <chrono>
#include <ctime>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <iomanip>

#define NOMINMAX
#include <windows.h>

#include "SKLib/include/cmdpar.hpp"


SKLIB_DECLARE_CMD_PARAMS(lidparam_type, wchar_t)
{
    SKLIB_OPTION_STRING(run);       // full path of script to run or nothing
    SKLIB_OPTION_SWITCH(lock);      // check to lock screen
    SKLIB_OPTION_STRING(log);       // full path of the log file or turn off
    SKLIB_OPTION_SWITCH(kill);      // kill running instance of itself
    SKLIB_OPTION_HELP(help);        // print help
};

// ------------- Globals ---------------

lidparam_type Param;
std::wofstream LogFile;

static constexpr wchar_t Myself[]           = L"LID-EVT";
static const     auto    MyselfClassName    = Myself;
static constexpr wchar_t MyselfWindowName[] = L"SK-APP-LID-EVT-INSTANCE";
static constexpr wchar_t MyselfMutexName[]  = L"SK-APP-LID-EVT";

static constexpr UINT KILL_SIGNAL = WM_APP + 0x0ABC;
static constexpr UINT PING_SIGNAL = WM_APP + 0x0123;
static LRESULT CALLBACK windowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// ------------- Logging ---------------

class LogLine
{
public:
    LogLine()
    {
        if (LogFile.is_open())
        {
            auto cur_clock = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            tm local_time;
            if (localtime_s(&local_time, &cur_clock) == 0) LogFile << std::put_time(&local_time, L"%F %T ");
        }
    }

    ~LogLine()
    {
        if (LogFile.is_open()) LogFile << std::endl;   // end line AND flush
    }

    template<class T>
    LogLine& operator<< (T value)
    {
        if (LogFile.is_open()) LogFile << value;
        return *this;
    }

    LogLine& operator<< (const GUID& guid)
    {
        const int buf_size = 128;
        wchar_t str[buf_size] = { 0 };
        StringFromGUID2(guid, str, buf_size);
        return operator<< (str);
    }
};

// ------------- Error Reporting -------

static void systemError(const wchar_t* what)
{
    const DWORD error = GetLastError();
    LogLine() << "Error " << error << " during: " << what;
    static wchar_t buffer[1024];
    const wchar_t* errorMessage = buffer;

    if (!FormatMessage((FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS),
                       0, error, 0, buffer, sizeof(buffer), 0))
    {
        errorMessage = L"(cannot format error message)";
    }
    else
    {
        LogLine() << "System error message: " << errorMessage;
    }

    std::wstringstream message;
    message << L"A system error occured within " << Myself << ".\n";
    message << L"Operation: " << what << "\n";
    message << L"System message: " << errorMessage;
    MessageBox(NULL, message.str().c_str(), (std::wstring(Myself)+L" error").c_str(), MB_OK | MB_ICONERROR);

    exit(EXIT_FAILURE);
}

// ------------- Startup ---------------

static void registerWindowClass(HINSTANCE instance)
{
    LogLine() << "Registering window class";

    WNDCLASSEX windowClass = { 0 };
    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.lpfnWndProc = &windowProcedure;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = MyselfClassName;

    if (!RegisterClassEx(&windowClass)) systemError(L"registering window class");
}

static HWND createWindow(HINSTANCE instance)
{
    LogLine() << "Creating instance window";

    HWND hWnd = CreateWindow(MyselfClassName, MyselfWindowName, 0, CW_USEDEFAULT, CW_USEDEFAULT,
                             CW_USEDEFAULT, CW_USEDEFAULT, HWND_MESSAGE, NULL, instance, NULL);
    if (!hWnd) systemError(L"creating window");

    return hWnd;
}

static void registerNotification(HWND window)
{
    LogLine() << "Registering GUID_MONITOR_POWER_ON (GUID: " << GUID_MONITOR_POWER_ON << ")";
    if (!RegisterPowerSettingNotification(window, &GUID_MONITOR_POWER_ON, DEVICE_NOTIFY_WINDOW_HANDLE))
    {
        systemError(L"cannot register GUID_MONITOR_POWER_ON power setting notification");
    }

    LogLine() << "Registering GUID_LIDSWITCH_STATE_CHANGE (GUID: " << GUID_LIDSWITCH_STATE_CHANGE << ")";
    if (!RegisterPowerSettingNotification(window, &GUID_LIDSWITCH_STATE_CHANGE, DEVICE_NOTIFY_WINDOW_HANDLE))
    {
        systemError(L"cannot register GUID_LIDSWITCH_STATE_CHANGE power setting notification");
    }
}

// ------------- Running Loop ----------

static LRESULT CALLBACK windowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == PING_SIGNAL)
    {
        LogLine() << "Attempt to run second instance detected";
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
    if (uMsg == KILL_SIGNAL)
    {
        LogLine() << "Received: KILL signal";
        PostMessage(hWnd, WM_QUIT, 0, 0);
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    if (uMsg != WM_POWERBROADCAST || wParam != PBT_POWERSETTINGCHANGE)
    {
        LogLine() << "Received: irrelevant message " << uMsg << "; WP=" << wParam << "; LP=" << lParam;
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    if (GetSystemMetrics(SM_REMOTESESSION))
    {
        LogLine() << "Message from remote session ignored";
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    const POWERBROADCAST_SETTING* setting = reinterpret_cast<const POWERBROADCAST_SETTING*>(lParam);
    bool monitorPowerUpdate = (setting->PowerSetting == GUID_MONITOR_POWER_ON);
    bool lidSwitchUpdate = (setting->PowerSetting == GUID_LIDSWITCH_STATE_CHANGE);
    if (!monitorPowerUpdate && !lidSwitchUpdate)
    {
        LogLine() << "Received: irrelevant POWER BROADCAST message " << setting->PowerSetting;
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    bool disable = !*reinterpret_cast<const DWORD*>(&setting->Data);
    LogLine() << "Received message: " << (lidSwitchUpdate ? "Lid" : "Monitor") << " is "
              << (lidSwitchUpdate ? (disable ? "Closed" : "Open") : (disable ? "Off" : "On"));

    if (!disable)
    {
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    if (Param.run.present())
    {
        LogLine() << "Executing: " << Param.run;
        ShellExecute(NULL, NULL, Param.run, L"", NULL, 0);
    }

    if (Param.lock.present() || Param.parser_status.is_empty())
    {
        if (!LockWorkStation()) systemError(L"locking workstation");
        else LogLine() << "Locked";
    }

    return 0;
}

static WPARAM messageLoop()
{
    LogLine() << "Starting message pump";
    while (true)
    {
        MSG message;
        BOOL result = GetMessage(&message, NULL, 0, 0);
        if (result < 0) systemError(L"getting window message");
        if (!result) return message.wParam;

        DispatchMessage(&message);
    }
}

// ------------- Print Help ------------

bool printText(const wchar_t* text)
{
    if (!AllocConsole()) return false;
//    if (!AttachConsole(ATTACH_PARENT_PROCESS)) return false;   //sk Writes but doesn't restore state of the parent console

    HANDLE hcon = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hcon == INVALID_HANDLE_VALUE) return false;

    WriteConsole(hcon, text, sklib::strlen<DWORD>(text), NULL, NULL);

    MSG message;
    GetMessage(&message, NULL, 0, 0);
    FreeConsole();
    return true;
}

void printHelp()
{
    printText(L"\
LID-EVT [-lock] [-run script_path_name] [-log logfile] [-kill]\n\
LID-EVT -kill\n\
LID-EVT -help\n\
  -lock    lock screen on lid close\n\
  -run     run script on lid close (needs full path and name to executable file)\n\
       The script can be any executable Windows file. Starting directory is undefined.\n\
  -log     write log of related events into a file\n\
  -kill    terminate previously launched LID-EVT (no effect if not present)\n\
  -help    show this text\n\
Remark: LID-EVT without parameters is equivalent to LID-EVT -lock\n\
        LID-EVT will not run twice. Will not run with conflicting parameters.\n");
}

// ------------- Previous Instance -----

HWND LockInstance()  // returns handle to previous instance, or NULL if not found
{
    if (!OpenMutex(MUTEX_ALL_ACCESS, 0, MyselfMutexName))
    {
        CreateMutex(0, 0, MyselfMutexName);
        return NULL;
    }

    return FindWindow(MyselfClassName, MyselfWindowName);
}

// ------------- Entry Point -----------

int CALLBACK wWinMain(HINSTANCE instance, HINSTANCE prevInstance, LPWSTR commandLine, int cmdShow)
{
    int ArgN = 0;
    LPWSTR* ArgcW = CommandLineToArgvW(commandLine, &ArgN);
    if (!*commandLine) ArgN = 0;

    if (!Param.parser_run(ArgN, ArgcW, 0) || Param.parser_status.is_help())
    {
        printHelp();
        exit(EXIT_SUCCESS);
    }

    HWND previous = LockInstance();
    if (previous)
    {
        SendMessage(previous, (Param.kill ? KILL_SIGNAL : PING_SIGNAL), 0, 0);
        exit(Param.kill ? EXIT_SUCCESS : EXIT_FAILURE);
    }

    if (Param.log.present())
    {
        LogFile.open(Param.log, std::ios_base::app);
    }

    LogLine() << Myself << " starting";

    registerWindowClass(instance);
    HWND window = createWindow(instance);
    registerNotification(window);

    WPARAM result = messageLoop();

    LogLine() << Myself << " terminating";
    return (int)result;
}

