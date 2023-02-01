
#include "tanalizeres.h"


#include "ES_hevc.h"
#include "bitstream.h"
#include "tdecode.h"


void ES_hevc::Parse(TAnalizerES *pes)
{
    if (!pes) return;

    unsigned len=pes->es.pes_header_length;
   if (len < 8) return;

   _stream_info *pinfo = &pes->es.stream_info;
    uint8_t *buf = pes->es.pes_header;

    unsigned nal_unit_type;
    uint16_t header;
      header = (buf[0] << 8) | buf[1];
      if (header & 0x8000) // ignore forbidden_bit == 1
        return;
      nal_unit_type   = (header & 0x7e00) >> 9;

      if (nal_unit_type != NAL_SPS_NUT)  // 0x21
      {
          return;
      }

  TBitstream bs(buf, len*8, true);
  unsigned int i;
  int sub_layer_profile_present_flag[8], sub_layer_level_present_flag[8];

  bs.skipBits(4); // sps_video_parameter_set_id

  unsigned int sps_max_sub_layers_minus1 = bs.readBits(3);
  bs.skipBits(1); // sps_temporal_id_nesting_flag

  // skip over profile_tier_level
  bs.skipBits(8 + 32 + 4 + 43 + 1 +8);
  for (i=0; i<sps_max_sub_layers_minus1; i++)
  {
    sub_layer_profile_present_flag[i] = bs.readBits(1);
    sub_layer_level_present_flag[i] = bs.readBits(1);
  }
  if (sps_max_sub_layers_minus1 > 0)
  {
    for (i=sps_max_sub_layers_minus1; i<8; i++)
      bs.skipBits(2);
  }
  for (i=0; i<sps_max_sub_layers_minus1; i++)
  {
    if (sub_layer_profile_present_flag[i])
      bs.skipBits(8 + 32 + 4 + 43 + 1);
    if (sub_layer_level_present_flag[i])
      bs.skipBits(8);
  }
  // end skip over profile_tier_level

  bs.readGolombUE(); // sps_seq_parameter_set_id
  unsigned int chroma_format_idc = bs.readGolombUE();

  if (chroma_format_idc == 3)
    bs.skipBits(1); // separate_colour_plane_flag

  pinfo->Width  = bs.readGolombUE();
  pinfo->Heigh = bs.readGolombUE();
  //pinfo->aspect = aspect;
  //pinfo->frate  = framerate;
  //pinfo->BitRate = bitrate;
}



void ES_hevc::GetText(TAnalizerES *pes,string &decode_desc)
{
    if (!pes) return;

    _stream_info *pinfo = &pes->es.stream_info;
    decode_desc += GetStreamCodecName(pinfo->type_stream);

    char buf[256]{};
    sprintf(buf," %dx%d",pinfo->Width,pinfo->Heigh);
    decode_desc += string(buf);
}

