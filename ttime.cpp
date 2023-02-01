#include "ttime.h"
#include "tglobal.h"
#include "tinterface.h"
#include "tini.h"

TTime Time;

static thread       std_thread_timer {};


void thread_timer_1sec()
{
    time_t currsec; time(&currsec);

    while(g.th_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        time_t dt; time(&dt);
        if (currsec==dt) continue;
        currsec=dt;

        for (auto & pInterface : pInterfacesArray)
            pInterface->flag_timer=1;       //__atomic_store_n(&interfaces[i].flag_timer,1,__ATOMIC_RELAXED);
        }
}





void TTime::Start()
{
    statistic_every=Ini.GetUInt("Common","statistic_every",0);

    if (statistic_every==0) {
         statistic_local=false;
         statistic_every = COLLECTOR_PERIOD;
        }
    else {
        statistic_local=true;
        }

    std_thread_timer = thread(thread_timer_1sec);
    std_thread_timer.detach();
}


void TTime::ReadSystemDateTime()
{
    char text_date[64];
    char text_time[64];

    time(&current_time_in_seconds);
    struct tm  tstruct;
    tstruct = *localtime(&current_time_in_seconds);

    strftime(&text_date[0],sizeof(text_date),"%d.%m.%Y",&tstruct);
    strftime(&text_time[0],sizeof(text_time),"%X",&tstruct);
    m_current_date = text_date;
    m_current_time = text_time;

    count_seconds++;

    if (statistic_local) {
        if (count_seconds >= statistic_every) {
            m_measure_start_flag  = true;
            start_measured_time   = current_time_in_seconds;
            m_measure_period_date = text_date;
            m_measure_period_time = text_time;
            }
        }
    else {
          unsigned ostatok = tstruct.tm_min % 5;   // 5 min
          if (ostatok==0 && tstruct.tm_sec==0) {
           m_measure_start_flag  = true;
           start_measured_time   = current_time_in_seconds;
           m_measure_period_date = text_date;
           m_measure_period_time = text_time;
           }
        }


}
