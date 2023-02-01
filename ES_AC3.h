#pragma once

#include "theader.h"



class TAnalizerES;


class ES_AC3
{

  public:

    void Parse(TAnalizerES *pes);
    void GetText(TAnalizerES *pes,string &decode_desc);
};



