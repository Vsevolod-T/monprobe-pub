
#include "tanalizeres.h"


#include "ES_Teletext.h"
#include "tdecode.h"


void ES_Teletext::Parse(TAnalizerES *pes)
{
    if (!pes) return;

    uint8_t *pes_data = pes->es.pes_header;
    unsigned pes_len=pes->es.pes_header_length;

   if (pes_len < 1) return;

  if (pes_data[0] < 0x10 || pes_data[0] > 0x1F)
  {
    return;
  }


}


void ES_Teletext::GetText(TAnalizerES *pes,string &decode_desc)
{
    if (!pes) return;
    _stream_info *pinfo = &pes->es.stream_info;
    decode_desc += " teletext";
    decode_desc += " ";
    decode_desc += string(pinfo->language);
}
