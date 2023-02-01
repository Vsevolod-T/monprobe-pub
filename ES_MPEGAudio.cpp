

#include "ES_MPEGAudio.h"
#include "bitstream.h"

#include "tanalizeres.h"
#include "tdecode.h"

#include "bitstream.h"


const uint16_t FrequencyTable[3] = { 44100, 48000, 32000 };
const uint16_t BitrateTable[2][3][15] =
{
  {
    {0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448 },
    {0, 32, 48, 56, 64,  80,  96,  112, 128, 160, 192, 224, 256, 320, 384 },
    {0, 32, 40, 48, 56,  64,  80,  96,  112, 128, 160, 192, 224, 256, 320 }
  },
  {
    {0, 32, 48, 56, 64,  80,  96,  112, 128, 144, 160, 176, 192, 224, 256},
    {0, 8,  16, 24, 32,  40,  48,  56,  64,  80,  96,  112, 128, 144, 160},
    {0, 8,  16, 24, 32,  40,  48,  56,  64,  80,  96,  112, 128, 144, 160}
  }
};


void ES_MPEG2Audio::Parse(TAnalizerES *pes)
{
    if (!pes) return;

    unsigned len=pes->es.pes_header_length;
   if (len < 3) return;

    uint8_t *buf = pes->es.pes_header;
    _stream_info *pinfo = &pes->es.stream_info;

    TBitstream bs(buf, 3 * 8);
    bs.skipBits(3);

    int mpegVersion = bs.readBits(2);
    if (mpegVersion == 1)
      return;
    int mpeg2  = !(mpegVersion & 1);
    int mpeg25 = !(mpegVersion & 3);

    int layer = bs.readBits(2); //layer '11'=1 '10'=2 '01'=3
    if (layer == 0)
      return;
    layer = 4 - layer;   //layer1==1 layer2==2 layer3==3

    bs.skipBits(1); // protetion bit
    int bitrate_index = bs.readBits(4);
    if (bitrate_index == 15 || bitrate_index == 0)
      return;
    int m_BitRate  = BitrateTable[mpeg2][layer - 1][bitrate_index] * 1000;

    int sample_rate_index = bs.readBits(2);
    if (sample_rate_index == 3)
      return;
    int m_SampleRate = FrequencyTable[sample_rate_index] >> (mpeg2 + mpeg25);

    /* int padding = */ bs.readBits1();
    bs.skipBits(1); // private bit
    int channel_mode = bs.readBits(2);

    pinfo->mpeg = mpegVersion;
    pinfo->layer = layer;
    pinfo->BitRate = m_BitRate;
    pinfo->SampleRate = m_SampleRate;
    pinfo->mode = channel_mode;

}


void ES_MPEG2Audio::GetText(TAnalizerES *pes,string &decode_desc)
{
    if (!pes) return;

    _stream_info *pinfo = &pes->es.stream_info;

    //decode_desc += GetStreamCodecName(pes->es.stream_info.type_stream);

    switch (pinfo->mpeg) {
    case 0:  decode_desc += " mpeg2.5"; break;
    case 1:  decode_desc += " mpeg?"; break;
    case 2:  decode_desc += " mpeg2"; break;
    case 3:  decode_desc += " mpeg1"; break;
    }

    switch (pinfo->layer) {
    case 0:  decode_desc += " layer?"; break;
    case 1:  decode_desc += " layer1"; break;
    case 2:  decode_desc += " layer2"; break;
    case 3:  decode_desc += " layer3"; break;
    }

    char buf[256]{};
    sprintf(buf," %s %.2fkHz",format_traffic(pinfo->BitRate/8).c_str(),(float(pinfo->SampleRate) / 1000.0));
    decode_desc += string(buf);

    switch (pinfo->mode) {
    case 0:  decode_desc += " stereo"; break;
    case 1:  decode_desc += " joint stereo"; break;
    case 2:  decode_desc += " dual channel"; break;
    case 3:  decode_desc += " mono"; break;
    }

    decode_desc += " ";
    decode_desc += string(pinfo->language);
}
