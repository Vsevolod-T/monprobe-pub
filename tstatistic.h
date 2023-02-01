#pragma once

#include "theader.h"



class TStatistic
{
    void OutStatistic(const string &filename);
    void OutStateChannelsToLog();

public:

    void Start();
    void RunFirst();
    void Run();
    void Stop();

    void Reset();
};

extern TStatistic Statistic;
