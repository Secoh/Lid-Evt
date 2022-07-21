// Copyright 2022 (c) Secoh
//
// This program detects the event of closing lid (of a laptop) -
// feature dearly missed since Windows Vista. It can just lock
// the screen, or run a script in response to the closed lid, like
// the event manager in Windows.
// This program is for Windows platform only, so it is MSVC-only
// environment.
//
// Original version was Laplock,
// Copyright 2011 (c) Etienne Dechamps (e-t172) <e-t172@akegroup.org>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.


//sk! verify

#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <memory>
#include <sstream>


#include <windows.h>

#include "SKLib/include/cmdpar.hpp"


SKLIB_DECLARE_CMD_PARAMS(param_t, wchar_t)
{
    SKLIB_OPTION_STRING(run);       // full path of script to run or nothing
    SKLIB_OPTION_SWITCH(lock);      // check to lock screen
    SKLIB_OPTION_STRING(log);       // full path of the log file or turn off
    SKLIB_OPTION_STRING(report);    // ?? why do we need it?
    SKLIB_OPTION_SWITCH(kill);      // kill running instance of itself
    SKLIB_OPTION_HELP(help);        // print help
}
Param;

std::wofstream LogFile;

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
        if (LogFile.is_open()) LogFile << std::endl;   // it flushes by C++ standard //sk TODO: verify that
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




int CALLBACK wWinMain(HINSTANCE instance, HINSTANCE prevInstance, LPWSTR commandLine, int cmdShow)
{
    int ArgN = 0;
    LPWSTR* ArgcW = CommandLineToArgvW(commandLine, &ArgN);

    Param.parser_run(ArgN, ArgcW, 0);

    //	if (*commandLine != 0)
    //		LogLine::logfile.reset(new std::wofstream(commandLine, std::ios_base::app));
    //	LogLine() << "laplock initializing";

    registerWindowClass(instance);

    HWND window = createWindow(instance);

    registerNotification(window);

    WPARAM result = messageLoop();
    LogLine() << "laplock terminating";
    return result;
}

