#pragma once

#include "theader.h"

class TMain
{
public:
};


extern void ChannelsReadConfigSection(const char *section);
extern int  main_loop(bool console_mode);
extern int  main_stop(int signo=0);
//extern void WriteToConfig();
