#include "tlog.h"
#include "tglobal.h"
#include "ttime.h"


TLog            Log;


void TLog::Initialize()
{
    m_service_mode = false;

    string tmp =  Time.getCurrentDate() + "-" + Time.getCurrentTime();
    replace(tmp.begin(),tmp.end(), '.', '_');
    replace(tmp.begin(),tmp.end(), ':', '_');
    m_log_current_filename = g.getLogFolder() + "/run_"+  tmp + ".log";


//printf("++++Log=%s\n",m_log_current_filename.c_str());

    struct stat st{};  //memset(&st,0,sizeof(st));
    if (stat(g.getLogFolder().c_str(), &st) == -1) {
        if (mkdir(g.getLogFolder().c_str(), 0777)!=0) {
            perror("mkdir");
            exit(1);  //exit if bad log init
            }
        }
    remove(m_log_current_filename.c_str());
}

void TLog::Scr(const char* pformat, ... )
{
    if (m_service_mode) return;
    va_list ap;
    va_start(ap, pformat);
    vprintf(pformat, ap);
    va_end(ap);
}


void TLog::File(const char* format, ... )
{
    FILE *tmp_out_file=fopen(m_log_current_filename.c_str(),"a+");

    if(tmp_out_file) {
        va_list ap;
        va_start(ap, format);
        vfprintf(tmp_out_file,format, ap);
        va_end(ap);
        fclose(tmp_out_file);
        chmod(m_log_current_filename.c_str(),0666);
        }
}


void TLog::ScrFile(const char* format, ... )
{


    if (!m_service_mode) {
        va_list ap;
        va_start(ap, format);
        vprintf(format, ap);
        va_end(ap);
    }

    FILE *tmp_out_file=fopen(m_log_current_filename.c_str(),"a+");
    if(tmp_out_file) {
        va_list ap;
        va_start(ap, format);
        vfprintf(tmp_out_file,format, ap);
        va_end(ap);
        fclose(tmp_out_file);
        chmod(m_log_current_filename.c_str(),0666);
        }

}





