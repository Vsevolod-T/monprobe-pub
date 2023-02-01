#pragma once

#include "theader.h"


class TAnalizerES;


class ES_hevc
{
     const unsigned NAL_SPS_NUT  = 0x21;

   // int             Width;
    //int             Height;

  public:
    void Parse(TAnalizerES *pes);
    void GetText(TAnalizerES *pes,string &decode_desc);
};



