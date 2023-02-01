#include "tanalizeres.h"


#include "ES_Subtitle.h"
#include "tdecode.h"



void ES_Subtitle::Parse(TAnalizerES *pes)
{
    if (!pes) return;

    uint8_t *pes_data = pes->es.pes_header;
    unsigned pes_len=pes->es.pes_header_length;

   if (pes_len < 2) return;

  if (pes_data[0] != 0x20 || pes_data[1] != 0x00)
  {
    return;
  }
}


void ES_Subtitle::GetText(TAnalizerES *pes,string &decode_desc)
{
    if (!pes) return;
    _stream_info *pinfo = &pes->es.stream_info;
    decode_desc += " subtitle";
    decode_desc += " ";
    decode_desc += string(pinfo->language);
}
