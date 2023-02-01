#pragma once

#include "theader.h"

class TTime
{
   static const unsigned COLLECTOR_PERIOD = 60*5; // 5 min

    string      m_current_date;
    string      m_current_time;
    string      m_current_usec;
    string      m_measure_period_date;
    string      m_measure_period_time;


public:
    TTime() {}


    void Start();   // from tmain init


    time_t  current_time_in_seconds;
    time_t  start_measured_time;
    bool    m_measure_start_flag = false;

    bool statistic_local=true;
    unsigned statistic_every=0;
    unsigned count_seconds=0;

    bool IsStatisticLocal() { return statistic_local; }

    // FIRST call from main_loop -> fill all string
    void ReadSystemDateTime();

    const string &getCurrentDate() { return m_current_date; }
    const string &getCurrentTime() { return m_current_time; }
    const string &getCurrentUsec() { return m_current_usec; }
    const string &getStartMeasureDate() { return m_measure_period_date; }
    const string &getStartMeasureTime() { return m_measure_period_time; }

    bool IsMeasureEnd()           { return m_measure_start_flag;   }
    unsigned GetMeasureSecond()   { return count_seconds; }
    // end main_loop
    void MeasureEnd()   {
        m_measure_start_flag = false;
        count_seconds=0;
        }
};

extern TTime Time;
