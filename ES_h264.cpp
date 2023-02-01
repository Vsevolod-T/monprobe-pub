#include "tanalizeres.h"

#include "tdecode.h"

#include "ES_h264.h"
#include "bitstream.h"

void ES_h264::Parse(TAnalizerES *pes)
{
    if (!pes) return;

    unsigned len=pes->es.pes_header_length;
   if (len < 8) return;

   _stream_info *pinfo = &pes->es.stream_info;
    uint8_t *buf = pes->es.pes_header;


  TBitstream bs(buf, len*8);
  unsigned int tmp, frame_mbs_only;
  //int cbpsize = -1;

  int profile_idc = bs.readBits(8);
  // constraint_set0_flag = bs.readBits1();
  // constraint_set1_flag = bs.readBits1();
  // constraint_set2_flag = bs.readBits1();
  // constraint_set3_flag = bs.readBits1();
  // reserved             = bs.readBits(4);
  bs.skipBits(8);
  /* int level_idc = */ bs.readBits(8);
  /* unsigned int seq_parameter_set_id = */  bs.readGolombUE(9);

  /*
  unsigned int i = 0;
  while (h264_lev2cpbsize[i][0] != -1)
  {
    if (h264_lev2cpbsize[i][0] >= level_idc)
    {
      cbpsize = h264_lev2cpbsize[i][1];
      break;
    }
    i++;
  }
  if (cbpsize < 0)
    return;
*/
  //memset(&m_streamData.sps[seq_parameter_set_id], 0, sizeof(h264_private::SPS));
  //m_streamData.sps[seq_parameter_set_id].cbpsize = cbpsize * 125; // Convert from kbit to bytes

  if( profile_idc == 100 || profile_idc == 110 ||
      profile_idc == 122 || profile_idc == 244 || profile_idc == 44 ||
      profile_idc == 83 || profile_idc == 86 || profile_idc == 118 ||
      profile_idc == 128 )
  {
    int chroma_format_idc = bs.readGolombUE(9); // chroma_format_idc
    if(chroma_format_idc == 3)
      bs.skipBits(1);           // residual_colour_transform_flag
    bs.readGolombUE();          // bit_depth_luma - 8
    bs.readGolombUE();          // bit_depth_chroma - 8
    bs.skipBits(1);             // transform_bypass
    if (bs.readBits1())         // seq_scaling_matrix_present
    {
      for (int i = 0; i < ((chroma_format_idc != 3) ? 8 : 12); i++)
      {
        if (bs.readBits1())     // seq_scaling_list_present
        {
          int last = 8, next = 8, size = (i<6) ? 16 : 64;
          for (int j = 0; j < size; j++)
          {
            if (next)
              next = (last + bs.readGolombSE()) & 0xff;
            last = !next ? last: next;
          }
        }
      }
    }
  }

  /* int log2_max_frame_num_minus4 = */ bs.readGolombUE();           // log2_max_frame_num - 4
  //m_streamData.sps[seq_parameter_set_id].log2_max_frame_num = log2_max_frame_num_minus4 + 4;
  int pic_order_cnt_type = bs.readGolombUE(9);
  //m_streamData.sps[seq_parameter_set_id].pic_order_cnt_type = pic_order_cnt_type;
  if (pic_order_cnt_type == 0)
  {
    /*int log2_max_pic_order_cnt_lsb_minus4 = */ bs.readGolombUE();         // log2_max_poc_lsb - 4
    //m_streamData.sps[seq_parameter_set_id].log2_max_pic_order_cnt_lsb = log2_max_pic_order_cnt_lsb_minus4 + 4;
  }
  else if (pic_order_cnt_type == 1)
  {
    /* m_streamData.sps[seq_parameter_set_id].delta_pic_order_always_zero_flag = */ bs.readBits1();
    bs.readGolombSE();         // offset_for_non_ref_pic
    bs.readGolombSE();         // offset_for_top_to_bottom_field
    tmp = bs.readGolombUE();   // num_ref_frames_in_pic_order_cnt_cycle
    for (unsigned int i = 0; i < tmp; i++)
      bs.readGolombSE();       // offset_for_ref_frame[i]
  }
  else if(pic_order_cnt_type != 2)
  {
    // Illegal poc
    return;
  }

  bs.readGolombUE(9);          // ref_frames
  bs.skipBits(1);             // gaps_in_frame_num_allowed
  unsigned m_Width   = bs.readGolombUE() + 1;// mbs
  unsigned m_Height  = bs.readGolombUE() + 1;// mbs
  frame_mbs_only     = bs.readBits1();
  //m_streamData.sps[seq_parameter_set_id].frame_mbs_only_flag = frame_mbs_only;
  //DBG(DEMUX_DBG_PARSE, "H.264 SPS: pic_width:  %u mbs\n", (unsigned) m_Width);
  //DBG(DEMUX_DBG_PARSE, "H.264 SPS: pic_height: %u mbs\n", (unsigned) m_Height);
  //DBG(DEMUX_DBG_PARSE, "H.264 SPS: frame only flag: %d\n", frame_mbs_only);

  m_Width  *= 16;
  m_Height *= 16 * (2-frame_mbs_only);

  if (!frame_mbs_only)
  {
    if (bs.readBits1()) {    // mb_adaptive_frame_field_flag
      ;//DBG(DEMUX_DBG_PARSE, "H.264 SPS: MBAFF\n");
      }
  }
  bs.skipBits(1);           // direct_8x8_inference_flag
  if (bs.readBits1())       // frame_cropping_flag
  {
    uint32_t crop_left   = bs.readGolombUE();
    uint32_t crop_right  = bs.readGolombUE();
    uint32_t crop_top    = bs.readGolombUE();
    uint32_t crop_bottom = bs.readGolombUE();
   // DBG(DEMUX_DBG_PARSE, "H.264 SPS: cropping %d %d %d %d\n", crop_left, crop_top, crop_right, crop_bottom);

    m_Width -= 2*(crop_left + crop_right);
    if (frame_mbs_only)
      m_Height -= 2*(crop_top + crop_bottom);
    else
      m_Height -= 4*(crop_top + crop_bottom);
  }

  pinfo->Width = m_Width;
  pinfo->Heigh = m_Height;
  //pinfo->aspect = aspect;
  //pinfo->frate  = framerate;
  //pinfo->BitRate = bitrate;

  /*
  // VUI parameters
  m_PixelAspect.num = 0;
  if (bs.readBits1())    // vui_parameters_present flag
  {
    if (bs.readBits1())  // aspect_ratio_info_present
    {
      uint32_t aspect_ratio_idc = bs.readBits(8);
      DBG(DEMUX_DBG_PARSE, "H.264 SPS: aspect_ratio_idc %d\n", aspect_ratio_idc);

      if (aspect_ratio_idc == 255 )// Extended_SAR
      {
        m_PixelAspect.num = bs.readBits(16); // sar_width
        m_PixelAspect.den = bs.readBits(16); // sar_height
        //DBG(DEMUX_DBG_PARSE, "H.264 SPS: -> sar %dx%d\n", m_PixelAspect.num, m_PixelAspect.den);
      }
      else
      {
        static const mpeg_rational_t aspect_ratios[] =
        { // page 213:
          // 0: unknown
          {0, 1},
          // 1...16:
          { 1,  1}, {12, 11}, {10, 11}, {16, 11}, { 40, 33}, {24, 11}, {20, 11}, {32, 11},
          {80, 33}, {18, 11}, {15, 11}, {64, 33}, {160, 99}, { 4,  3}, { 3,  2}, { 2,  1}
        };

        if (aspect_ratio_idc < sizeof(aspect_ratios)/sizeof(aspect_ratios[0]))
        {
          //memcpy(&m_PixelAspect, &aspect_ratios[aspect_ratio_idc], sizeof(mpeg_rational_t));
          //DBG(DEMUX_DBG_PARSE, "H.264 SPS: PAR %d / %d\n", m_PixelAspect.num, m_PixelAspect.den);
        }
        else
        {
          //DBG(DEMUX_DBG_PARSE, "H.264 SPS: aspect_ratio_idc out of range !\n");
        }
      }
    }
    if (bs.readBits1()) // overscan
    {
      bs.readBits1(); // overscan_appropriate_flag
    }
    if (bs.readBits1()) // video_signal_type_present_flag
    {
      bs.readBits(3); // video_format
      bs.readBits1(); // video_full_range_flag
      if (bs.readBits1()) // colour_description_present_flag
      {
        bs.readBits(8); // colour_primaries
        bs.readBits(8); // transfer_characteristics
        bs.readBits(8); // matrix_coefficients
      }
    }

    if (bs.readBits1()) // chroma_loc_info_present_flag
    {
      bs.readGolombUE(); // chroma_sample_loc_type_top_field
      bs.readGolombUE(); // chroma_sample_loc_type_bottom_field
    }

    if (bs.readBits1()) // timing_info_present_flag
    {
//      uint32_t num_units_in_tick = bs.readBits(32);
//      uint32_t time_scale = bs.readBits(32);
//      int fixed_frame_rate = bs.readBits1();
//      if (num_units_in_tick > 0)
//        m_FPS = time_scale / (num_units_in_tick * 2);
    }
  }
 // DBG(DEMUX_DBG_PARSE, "H.264 SPS: -> video size %dx%d, aspect %d:%d\n", m_Width, m_Height, m_PixelAspect.num, m_PixelAspect.den);
*/
}








void ES_h264::GetText(TAnalizerES *pes,string &decode_desc)
{
    if (!pes) return;

    _stream_info *pinfo = &pes->es.stream_info;

    decode_desc += GetStreamCodecName(pinfo->type_stream);

    char buf[256]{};
    sprintf(buf," %dx%d",pinfo->Width,pinfo->Heigh);
    decode_desc += string(buf);
}
