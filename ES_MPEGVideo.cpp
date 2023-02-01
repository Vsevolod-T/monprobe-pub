
#include "ES_MPEGVideo.h"
#include "tanalizeres.h"
#include "tdecode.h"

#include "bitstream.h"

void ES_MPEG2Video::Parse(TAnalizerES *pes)
{
    if (!pes) return;

   if (pes->es.pes_header_length < 8) return;

    unsigned len=pes->es.pes_header_length;
   if (len < 8) return;

    uint8_t *buf = pes->es.pes_header;
    _stream_info *pinfo = &pes->es.stream_info;

    TBitstream bs(buf, 8 * 8);

     unsigned width  = bs.readBits(12);
     unsigned heigh  = bs.readBits(12);
     unsigned aspect = bs.readBits(4);
     unsigned framerate = bs.readBits(4);
     unsigned bitrate = bs.readBits(18);

   if (bitrate==0x3FFFF)
       bitrate=0;  //vbr
   else
       bitrate *= 400;

   pinfo->Width = width;
   pinfo->Heigh = heigh;
   pinfo->aspect = aspect;
   pinfo->frate  = framerate;
   pinfo->BitRate = bitrate;
}


void ES_MPEG2Video::GetText(TAnalizerES *pes,string &decode_desc)
{
    if (!pes) return;

     _stream_info *pinfo = &pes->es.stream_info;

    char buf[256]{};
    sprintf(buf," %s %dx%d",GetStreamCodecName(pes->es.stream_info.type_stream),pinfo->Width,pinfo->Heigh );
    decode_desc += string(buf);

    switch (pinfo->aspect)
    {
     case 0:  decode_desc += " (0)"; break;
     case 1:  decode_desc += " 1:1"; break;
     case 2:  decode_desc += " 4:3"; break;
     case 3:  decode_desc += " 16:9"; break;
     case 4:  decode_desc += " 2.21:1"; break;
     default: decode_desc += " ?" + to_string(pinfo->aspect); break;
     }

     switch (pinfo->frate)
     {
     case 0:  decode_desc += " (0)"; break;
     case 1:  decode_desc += " 23.976"; break;
     case 2:  decode_desc += " 24.00"; break;
     case 3:  decode_desc += " 25.00"; break;
     case 4:  decode_desc += " 29.97"; break;
     case 5:  decode_desc += " 30"; break;
     case 6:  decode_desc += " 50"; break;
     case 7:  decode_desc += " 59.94"; break;
     case 8:  decode_desc += " 60"; break;
     default: decode_desc += " ?" + to_string(pinfo->frate); break;
     }
     decode_desc += " ";
     if (pinfo->BitRate==0)
        decode_desc += "vbr";
     else
        decode_desc += format_traffic(pinfo->BitRate/8);
}

