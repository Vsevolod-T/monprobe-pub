#pragma once

#include "theader.h"

class  TGlobal
{
private:

    string      m_current_folder; // без последнего /
    string      m_current_name; // имя программы
    string      m_current_ini;  // name.ini
    string      m_log_folder;

    bool        m_af_mode;

    void        OutCPUName();
    void        OutListInterfaces();

    unsigned     m_time_alive;

    bool        m_runmode;




public:

    int         GetCPUCores();

    TIPAddress  GetInterfaceIp(const char *name_if);
    unsigned    GetInterfaceIndex(const char * addr);
    string      GetInterfaceName(const char * addr);
    int         GetInterfaceIrq(const char * addr);
    bool        ExistsInterface(TIPAddress addr);
    bool        GetNameIf_IpIf(const char *inp, string &out_name_if, TIPAddress &out_ip_if);

    atomic_uint      channels_ready;
    atomic_uint      th_running;

    pthread_mutex_t  channels_lock;
    pthread_cond_t   channels_cond_end;

    atomic_uint      files_count_worked;
    atomic_uint      files_ready;

    pthread_mutex_t  files_lock;
    pthread_cond_t   files_cond_end;

    bool ignore_audio_cc=false;

    string area;
    string place;


public:

    void        Initialize(const char *name_program);
    void        OutSystemInfo();

    const string& getCurrentPath() { return m_current_folder; }
    const string& getCurrentName() { return m_current_name; }
    const string& getCurrentIni()  { return m_current_ini; }

    void  setLogFolder(const string& log_folder) { m_log_folder = log_folder; }
    const string& getLogFolder() { return m_log_folder; }


    void        set_AF_Mode(bool mode)  { m_af_mode = mode; }
    bool        AF_Mode()               { return m_af_mode; }

    void        set_TimeAlive(unsigned val) { m_time_alive = val; }
    unsigned    getTimeAlive()          { return m_time_alive; }


    bool RunMode() { return m_runmode; }
    void setRunMode(bool mode) { m_runmode=mode; }

 bool  IsRootPermission() { if(getuid()) return false; else return true; }

};
extern TGlobal g;
