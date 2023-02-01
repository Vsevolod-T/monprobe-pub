#include "tanalizeres.h"
#include "ES_AAC.h"
#include "bitstream.h"
#include "tdecode.h"
#include "bitstream.h"


static int aac_sample_rates[16] =
{
  96000, 88200, 64000, 48000, 44100, 32000,
  24000, 22050, 16000, 12000, 11025, 8000, 7350,
  0, 0, 0
};




void ES_AAC::Parse(TAnalizerES *pes)
{
    if (!pes) return;

   unsigned buf_size=pes->es.pes_header_length;
   if (buf_size < 16) return;

   uint8_t *buf_ptr = pes->es.pes_header;
   _stream_info *p = &pes->es.stream_info;

  if (p->type_stream == STREAM_TYPE_AUDIO_AAC)
  {
    if (buf_ptr[0] == 0xFF && (buf_ptr[1] & 0xF0) == 0xF0)
      p->type_stream = STREAM_TYPE_AUDIO_AAC_ADTS;
    else if (buf_ptr[0] == 0x56 && (buf_ptr[1] & 0xE0) == 0xE0)
      p->type_stream = STREAM_TYPE_AUDIO_AAC_LATM;
  }


  // STREAM_TYPE_AUDIO_AAC_LATM
  if (p->type_stream == STREAM_TYPE_AUDIO_AAC_LATM)
  {
    return;
  }




  // STREAM_TYPE_AUDIO_AAC_ADTS
  else if (p->type_stream == STREAM_TYPE_AUDIO_AAC_ADTS)
  {
    if(buf_ptr[0] == 0xFF && (buf_ptr[1] & 0xF0) == 0xF0)
    {
      // need at least 7 bytes for header
      if (buf_size < 7)
        return;

      TBitstream bs(buf_ptr, 9 * 8);
      bs.skipBits(15);

      // check if CRC is present, means header is 9 byte long
      int noCrc = bs.readBits(1);
      if (!noCrc && (buf_size < 9))
        return;

      bs.skipBits(2); // profile
      int SampleRateIndex = bs.readBits(4);
      bs.skipBits(1); // private
      p->Channels = bs.readBits(3);
      bs.skipBits(4);

      p->SampleRate    = aac_sample_rates[SampleRateIndex & 0x0F];

     // int duration = 1024 * 90000 / (!p->SampleRate ? aac_sample_rates[4] : p->SampleRate);

//Audio: ac3 (AC-3 / 0x332D4341), 48000 Hz, 5.1(side), fltp, 384 kb/s
     //uhd  SampleRate=48000 BitRate=2304000 depth=24 orig=393216

      int bit_depth = 24;//((p->Channels * 90000) / p->SampleRate) * 8;
       p->BitRate = p->Channels * p->SampleRate * bit_depth;

//Log.Scr("SampleRate=%d BitRate=%u depth=%u orig=%u\n",p->SampleRate,p->BitRate,bit_depth,384*1024);
      return;
    }
  }
  return;
}



void ES_AAC::GetText(TAnalizerES *pes,string &decode_desc)
{
    if (!pes) return;

    _stream_info *p = &pes->es.stream_info;

    //decode_desc += GetStreamCodecName(p->type_stream);

    switch(p->type_stream) {
    case STREAM_TYPE_AUDIO_AAC:
        decode_desc += " AAC";
        break;
    case STREAM_TYPE_AUDIO_AAC_ADTS:
        decode_desc += " AAC-ADTS";
        break;
    case STREAM_TYPE_AUDIO_AAC_LATM:
        decode_desc += " AAC-LATM";
        break;
    default:
        decode_desc += " AAC-?";
        break;
    }

    char buf[256]{};
    sprintf(buf," SampleRate=%d Channels=%d BitRate=%d %s",p->SampleRate,p->Channels,p->BitRate,p->language);
    decode_desc += string(buf);

}
