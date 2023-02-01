#pragma once

#include "theader.h"

class TAnalizerES;

class ES_MPEG2Audio
  {

  public:
    void Parse2(TAnalizerES *pes);

    void Parse(TAnalizerES *pes);
    void GetText(TAnalizerES *pes,string &decode_desc);
};
