
#include "ES_AC3.h"
#include "tanalizeres.h"



#include <algorithm>      // for max

#include "tdecode.h"

#include "bitstream.h"


#define AC3_HEADER_SIZE 7

/* Channel mode (audio coding mode) */
enum AC3ChannelMode
{
  AC3_CHMODE_DUALMONO = 0,
  AC3_CHMODE_MONO,
  AC3_CHMODE_STEREO,
  AC3_CHMODE_3F,
  AC3_CHMODE_2F1R,
  AC3_CHMODE_3F1R,
  AC3_CHMODE_2F2R,
  AC3_CHMODE_3F2R
};

/* possible frequencies */
const uint16_t AC3SampleRateTable[3] = { 48000, 44100, 32000 };

/* possible bitrates */
const uint16_t AC3BitrateTable[19] = {
    32, 40, 48, 56, 64, 80, 96, 112, 128,
    160, 192, 224, 256, 320, 384, 448, 512, 576, 640
};

const uint8_t AC3ChannelsTable[8] = {
    2, 1, 2, 3, 3, 4, 4, 5
};

const uint16_t AC3FrameSizeTable[38][3] = {
    { 64,   69,   96   },
    { 64,   70,   96   },
    { 80,   87,   120  },
    { 80,   88,   120  },
    { 96,   104,  144  },
    { 96,   105,  144  },
    { 112,  121,  168  },
    { 112,  122,  168  },
    { 128,  139,  192  },
    { 128,  140,  192  },
    { 160,  174,  240  },
    { 160,  175,  240  },
    { 192,  208,  288  },
    { 192,  209,  288  },
    { 224,  243,  336  },
    { 224,  244,  336  },
    { 256,  278,  384  },
    { 256,  279,  384  },
    { 320,  348,  480  },
    { 320,  349,  480  },
    { 384,  417,  576  },
    { 384,  418,  576  },
    { 448,  487,  672  },
    { 448,  488,  672  },
    { 512,  557,  768  },
    { 512,  558,  768  },
    { 640,  696,  960  },
    { 640,  697,  960  },
    { 768,  835,  1152 },
    { 768,  836,  1152 },
    { 896,  975,  1344 },
    { 896,  976,  1344 },
    { 1024, 1114, 1536 },
    { 1024, 1115, 1536 },
    { 1152, 1253, 1728 },
    { 1152, 1254, 1728 },
    { 1280, 1393, 1920 },
    { 1280, 1394, 1920 },
};

const uint8_t EAC3Blocks[4] = {
  1, 2, 3, 6
};

enum EAC3FrameType {
  EAC3_FRAME_TYPE_INDEPENDENT = 0,
  EAC3_FRAME_TYPE_DEPENDENT,
  EAC3_FRAME_TYPE_AC3_CONVERT,
  EAC3_FRAME_TYPE_RESERVED
};




void ES_AC3::Parse(TAnalizerES *pes)
{
    if (!pes) return;

    unsigned pes_len=pes->es.pes_header_length;

   if (pes_len < 9) return;

    int  m_FrameSize=0;

  uint8_t *buf_ptr = pes->es.pes_header;

  if ((buf_ptr[0] == 0x0b && buf_ptr[1] == 0x77))
  {
    TBitstream bs(buf_ptr + 2, AC3_HEADER_SIZE * 8);

    // read ahead to bsid to distinguish between AC-3 and E-AC-3
    int bsid = bs.showBits(29) & 0x1F;
    if (bsid > 16)
      return;

    _stream_info *p = &pes->es.stream_info;

    if (bsid <= 10)
    {
      // Normal AC-3
      bs.skipBits(16);
      int fscod       = bs.readBits(2);
      int frmsizecod  = bs.readBits(6);
      bs.skipBits(5); // skip bsid, already got it
      bs.skipBits(3); // skip bitstream mode
      int acmod       = bs.readBits(3);

      if (fscod == 3 || frmsizecod > 37)
        return;




      if (acmod == AC3_CHMODE_STEREO)
      {
        bs.skipBits(2); // skip dsurmod
      }
      else
      {
        if ((acmod & 1) && acmod != AC3_CHMODE_MONO)
          bs.skipBits(2);
        if (acmod & 4)
          bs.skipBits(2);
      }
      int lfeon = bs.readBits(1);

      int srShift   = std::max(bsid, 8) - 8;
      p->SampleRate  = AC3SampleRateTable[fscod] >> srShift;
      p->BitRate     = (AC3BitrateTable[frmsizecod>>1] * 1000) >> srShift;
      p->Channels    = AC3ChannelsTable[acmod] + lfeon;
      m_FrameSize   = AC3FrameSizeTable[frmsizecod][fscod] * 2;
    }
    else
    {
      // Enhanced AC-3
      int frametype = bs.readBits(2);
      if (frametype == EAC3_FRAME_TYPE_RESERVED)
        return;

       bs.readBits(3); // int substreamid

      m_FrameSize = (bs.readBits(11) + 1) << 1;
      if (m_FrameSize < AC3_HEADER_SIZE)
        return;

      int numBlocks = 6;
      int sr_code = bs.readBits(2);
      if (sr_code == 3)
      {
        int sr_code2 = bs.readBits(2);
        if (sr_code2 == 3)
          return;
        p->SampleRate = AC3SampleRateTable[sr_code2] / 2;
      }
      else
      {
        numBlocks = EAC3Blocks[bs.readBits(2)];
        p->SampleRate = AC3SampleRateTable[sr_code];
      }

      int channelMode = bs.readBits(3);
      int lfeon = bs.readBits(1);

      p->BitRate  = (uint32_t)(8.0 * m_FrameSize * p->SampleRate / (numBlocks * 256.0));
      p->Channels = AC3ChannelsTable[channelMode] + lfeon;
    }

  }

}


void ES_AC3::GetText(TAnalizerES *pes,string &decode_desc)
{
    if (!pes) return;

    _stream_info *p = &pes->es.stream_info;
    //decode_desc += GetStreamCodecName(p->type_stream);

    char buf[256]{};
    sprintf(buf,"AC3 SampleRate=%d Channels=%d BitRate=%d %s",p->SampleRate,p->Channels,p->BitRate,p->language);
    decode_desc += string(buf);

}
