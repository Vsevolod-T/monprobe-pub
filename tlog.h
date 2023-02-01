#pragma once

#include "theader.h"



class TLog
{
    string m_log_current_filename;
    bool   m_service_mode;

public:

    void Initialize();
    void Scr(const char* fmt, ... )  __attribute__ (( format( printf , 2, 3 )) );
    void File(const char* fmt, ... ) __attribute__ (( format( printf , 2, 3 )) );
    void ScrFile(const char* fmt, ... ) __attribute__ (( format( printf , 2, 3 )) );

    void setServiceMode(bool mode) { m_service_mode = mode; }
    bool ServiceMode()             { return m_service_mode; }

};
extern TLog Log;

