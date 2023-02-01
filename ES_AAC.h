#pragma once

#include "theader.h"


class TAnalizerES;

class ES_AAC
{
   // int         SampleRate;
   // int         Channels;
   // int         BitRate;


  public:
    void Parse(TAnalizerES *pes);
    void GetText(TAnalizerES *pes,string &decode_desc);
};

