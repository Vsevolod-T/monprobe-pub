#include "trawes.h"

void TPcr::NewPcr(const uint8_t *pkt) {

    prev_pcr = new_pcr;

    // new_pcr = pcr_base * 300 + pcr_ext;
    new_pcr      = (uint64_t(pkt[2] << 25)  |  uint64_t(pkt[3] << 17) |  uint64_t(pkt[4] << 9) | uint64_t(pkt[5] << 1) | uint64_t(pkt[6] >> 7)) * uint64_t(300);
    new_pcr     |= (uint64_t(pkt[6] & 1) << 8 ) | uint64_t(pkt[7]);

    found_pcr++;
    if (found_pcr > 1) found_pcr=2;
    else return;

    if (new_pcr < prev_pcr)
        return;

    // formula
    //delta_time = (byte 2 - byte 1 ) / TS data rate
    //PCR_AC = time(pcr2 - pcr1) - delta_time

    /*

    uint64_t diff = new_pcr - prev_pcr;  // delta_time
    bitRate = double(40608000000ULL * count_pkt_pcr_pid) / double(diff);

    double pcr_delta = double(diff * 1000) / SYSTEM_CLOCK_FREQUENCY;  //  (ms) pcr2 - pcr1
    if(pcr_delta > 40)
        printf("2.3b - ERROR - PCR_error - PCR delta is greater than 40ms ( %.2f )\n",pcr_delta);

    double delta_time = double(count_pkt_pcr_pid * 188 * 8) /  bitRate; //0.026696
   //double delta_pcr = double(diff) / SYSTEM_CLOCK_FREQUENCY;
    //double pcr_ac = delta_pcr - delta_time;

   // pcr_delta /=1000.0;
    double pcr_ac = pcr_delta - delta_time;



    if(pcr_ac >0.05 || pcr_ac < -0.05)
        printf("2.4  - ERROR - PCR_accuracy_error - PCR jitter is greater than +- 500ns  %f\n",pcr_ac);

    printf("pcr_delta=%f  delta_time=%f\n",pcr_delta,delta_time);
    //printf("PCR: bitrate=%f  diff=%lu pcr_ac=%f count_packet=%u \n",bitRate,diff,pcr_ac,count_pkt_pcr_pid);
*/


    // formula
    //delta_time = (byte 2 - byte 1 ) / TS data rate
    //PCR_AC     = time(pcr2 - pcr1) - delta_time

    int64_t timediff = new_pcr - prev_pcr;  // in 27000000 tick's

    // 188 * 8 * 27000000 = 40608000000
    // 188 * 8 * 1000000   = 1504000000

    if (timediff==0) return;
    bitRate = (40608000000ULL * count_pkt_pcr_pid) / timediff;  // (2564000) bit/sec


 // calculate pcr_ac
    //double delta_time = double(count_pkt_pcr_pid * 188 * 8) /  bitRate; //
    //double delta_pcr = double(timediff) / 27000000.0;
    //[[maybe_unused]] double pcr_ac = delta_pcr - delta_time;

/*
    // --------------------------------
    double pcr_delta = double(timediff * 1000) / 27000000.0;  //  (ms) pcr2 - pcr1
    if(pcr_delta > 40) {
        printf("2.3b - ERROR - PCR_error - PCR delta is greater than 40ms ( %.2f )\n",pcr_delta);
        printf("PCR: timediff=%lu bitRate=%.3f delta_time=%f delta_pcr=%f pcr_ac=%f\n",timediff,bitRate,delta_time,delta_pcr,pcr_ac);
        exit(0);
    }
    // --------------------------------

printf("PCR: timediff=%lu bitRate=%.3f delta_time=%f delta_pcr=%f pcr_ac=%f\n",timediff,bitRate,delta_time,delta_pcr,pcr_ac);
*/
    count_pcr++;
    count_pkt_pcr_pid=0;
}



void pmt_set_stream_info(TRawES *p,uint8_t type)
{
    switch (type)
    {
      case 0x01:
        p->stream_info.type = VIDEO;
        p->stream_info.type_stream = STREAM_TYPE_VIDEO_MPEG1;
        return;
      case 0x02:
        p->stream_info.type = VIDEO;
        p->stream_info.type_stream = STREAM_TYPE_VIDEO_MPEG2;
        return;
      case 0x03:
        p->stream_info.type = AUDIO;
        p->stream_info.type_stream = STREAM_TYPE_AUDIO_MPEG1;
        return;
      case 0x04:
        p->stream_info.type = AUDIO;
        p->stream_info.type_stream = STREAM_TYPE_AUDIO_MPEG2;
        return;
      case 0x05:
        p->stream_info.type = DATA;
        p->stream_info.type_stream = STREAM_TYPE_PRIVATE_DATA;
        return;
     case 0x06:
       p->stream_info.type = AUDIO;
       p->stream_info.type_stream = STREAM_TYPE_AUDIO_AC3;
       return;

      case 0x0f:
      case 0x11:
        p->stream_info.type = AUDIO;
        p->stream_info.type_stream = STREAM_TYPE_AUDIO_AAC;
        return;
      case 0x10:
        p->stream_info.type = VIDEO;
        p->stream_info.type_stream = STREAM_TYPE_VIDEO_MPEG4;
        return;
      case 0x1b:
        p->stream_info.type = VIDEO;
        p->stream_info.type_stream = STREAM_TYPE_VIDEO_H264;
        return;
      case 0x24:
        p->stream_info.type = VIDEO;
        p->stream_info.type_stream = STREAM_TYPE_VIDEO_HEVC;
        return;
      case 0xea:
        p->stream_info.type = VIDEO;
        p->stream_info.type_stream = STREAM_TYPE_VIDEO_VC1;
        return;
      case 0x80:
        p->stream_info.type = AUDIO;
        p->stream_info.type_stream = STREAM_TYPE_AUDIO_LPCM;
        return;
      case 0x86:
        p->stream_info.type = DATA;   // scte-35
        p->stream_info.type_stream = STREAM_TYPE_UNKNOWN;
        return;
      case 0x81:
      case 0x83:
      case 0x84:
      case 0x87:
        p->stream_info.type = AUDIO;
        p->stream_info.type_stream = STREAM_TYPE_AUDIO_AC3;
        return;
      case 0x82:
      case 0x85:
      case 0x8a:
        p->stream_info.type = AUDIO;
        p->stream_info.type_stream = STREAM_TYPE_AUDIO_DTS;
        return;
    }
    p->stream_info.type = UNDEF;
    p->stream_info.type_stream = STREAM_TYPE_UNKNOWN;
    return;
}
/*
  // ======== SETUP TYP
    decode_typ[0x0001] = { VIDEO,    MPEG1, "ISO/IEC 11172-2 (MPEG-1 Video)" };
    decode_typ[0x0002] = { VIDEO,    MPEG2, "ISO/IEC 13818-2 (MPEG-2 Video)" };
    decode_typ[0x0003] = { AUDIO,    MPEG1, "ISO/IEC 11172-3 (MPEG-1 Audio)" };
    decode_typ[0x0004] = { AUDIO,    MPEG2, "ISO/IEC 13818-3 (MPEG-2 Audio)" };
    decode_typ[0x0005] = {  DATA,    UNDEF, "ISO/IEC 13818-1 (private section)" };
    decode_typ[0x0006] = { AUDIO,        AC3, "ISO/IEC 13818-1 PES (private data) AC3/VBI" }; // sub = UNDEF ?
    decode_typ[0x0007] = { UNDEF,    UNDEF, "ISO/IEC 13522 MHEG" };
    decode_typ[0x0008] = { UNDEF,    UNDEF, "ITU-T H.222.0 annex A DSM-CC" };
    decode_typ[0x0009] = { UNDEF,    UNDEF, "ITU-T H.222.1" };
    decode_typ[0x000a] = { UNDEF,    UNDEF, "ISO/IEC 13818-6 DSM-CC type A" };
    decode_typ[0x000b] = { UNDEF,    UNDEF, "ISO/IEC 13818-6 DSM-CC type B" };
    decode_typ[0x000c] = { UNDEF,    UNDEF, "ISO/IEC 13818-6 DSM-CC type C" };
    decode_typ[0x000d] = { UNDEF,    UNDEF, "ISO/IEC 13818-6 DSM-CC type D" };
    decode_typ[0x000e] = { UNDEF,    UNDEF, "ISO/IEC 13818-1 (auxiliary)" };
    decode_typ[0x000f] = { AUDIO,    UNDEF, "ISO/IEC 13818-7 (AAC Audio)" };
    decode_typ[0x0010] = { VIDEO,    MPEG4, "ISO/IEC 14496-2 (MPEG-4 Video)" };
    decode_typ[0x0011] = { AUDIO,    UNDEF, "ISO/IEC 14496-3 (AAC LATM Audio)" };
    decode_typ[0x001b] = { VIDEO,     H264, "ITU-T H.264 (h264 Video" };
    decode_typ[0x0024] = { VIDEO,     H265, "ITU-T H.265 (Ultra HD Video)" };
    decode_typ[0x0081] = { AUDIO,      AC3, "(AC3 Audio)" };
    decode_typ[0x0086] = {  DATA,    UNDEF, "(SCTE-35 Label)" };
    decode_typ[0x008a] = { AUDIO,      AC3, "(DTS Audio)" };
    decode_typ[0x00bd] = { AUDIO,    UNDEF, "(non-MPEG Audio, subpictures)" };
    decode_typ[0x00be] = { UNDEF,    UNDEF, "(padding stream)" };
    decode_typ[0x00bf] = { UNDEF,    UNDEF, "(navigation data)" };
    for (unsigned i=0x00c0; i<= 0x00d0; ++i) decode_typ[i] = { AUDIO,    UNDEF,   "(ID AUDIO stream)" };
    decode_typ[0x00d1] = { VIDEO,     H265, "(DIRAC Video) - UHD" };
    for (unsigned i=0x00d2; i<= 0x00df; ++i) decode_typ[i] = { AUDIO,    UNDEF,   "(ID AUDIO stream)" };
    for (unsigned i=0x00e0; i<= 0x00ef; ++i) decode_typ[i] = { VIDEO,    UNDEF,   "(ID VIDEO stream)" };
    decode_typ[0x00F0] = {  ECM,    UNDEF, "ECM" };
    decode_typ[0x00F1] = {  EMM,    UNDEF, "EMM" };
*/

void  pmt_parse_pes_descriptor(TRawES *pes,uint8_t * p, size_t len)
{
  const unsigned char* desc_end = p + len;

  while (p < desc_end)
  {
    uint8_t desc_tag = get8(p);
    uint8_t desc_len = get8(p);

    switch (desc_tag)
    {
      case 0x02:
      case 0x03:
        break;
      case 0x0a: /* ISO 639 language descriptor */
        if (desc_len >= 4)
        {
          pes->stream_info.language[0] = get8(p);
          pes->stream_info.language[1] = get8(p);
          pes->stream_info.language[2] = get8(p);
          pes->stream_info.language[3] = 0;
        }
        break;
      case 0x56: /* DVB teletext descriptor */
        pes->stream_info.type = VBI;
        pes->stream_info.type_stream = STREAM_TYPE_DVB_TELETEXT;
        break;
      case 0x6a: /* DVB AC3 */
      case 0x81: /* AC3 audio stream */
        pes->stream_info.type = AUDIO;
        pes->stream_info.type_stream = STREAM_TYPE_AUDIO_AC3;
        break;
      case 0x7a: /* DVB enhanced AC3 */
        pes->stream_info.type = AUDIO;
        pes->stream_info.type_stream = STREAM_TYPE_AUDIO_EAC3;
        break;
      case 0x7b: /* DVB DTS */
        pes->stream_info.type = AUDIO;
        pes->stream_info.type_stream = STREAM_TYPE_AUDIO_DTS;
        break;
      case 0x7c: /* DVB AAC */
        pes->stream_info.type = AUDIO;
        pes->stream_info.type_stream = STREAM_TYPE_AUDIO_AAC;
        break;
      case 0x59: /* subtitling descriptor */
        if (desc_len >= 8)
        {
          /*
           * Byte 4 is the subtitling_type field
           * av_rb8(p + 3) & 0x10 : normal
           * av_rb8(p + 3) & 0x20 : for the hard of hearing
           */
          pes->stream_info.type = VBI;
          pes->stream_info.type_stream = STREAM_TYPE_DVB_SUBTITLE;
          pes->stream_info.language[0] = get8(p);
          pes->stream_info.language[1] = get8(p);
          pes->stream_info.language[2] = get8(p);
          pes->stream_info.language[3] = 0;
         // si.composition_id = (int)av_rb16(p + 4);
         // si.ancillary_id = (int)av_rb16(p + 6);
        }
        break;
      case 0x05: /* registration descriptor */
      case 0x1E: /* SL descriptor */
      case 0x1F: /* FMC descriptor */
      case 0x52: /* stream identifier descriptor */
    default:
      break;
    }
    p += desc_len;
  }
}

/*
    decode_tag[0x0002] = { VIDEO,    UNDEF,  "video stream descriptor" };
    decode_tag[0x0003] = { AUDIO,    UNDEF,  "registration descriptor" };
    decode_tag[0x0004] = { UNDEF,    UNDEF,  "hierarchy descriptor" };
    decode_tag[0x0005] = { UNDEF,    UNDEF,  "audio stream descriptor" };
    decode_tag[0x0006] = { UNDEF,    UNDEF,  "data stream alignment descriptor" };
    decode_tag[0x0007] = { UNDEF,    UNDEF,  "target background grid descriptor" };
    decode_tag[0x0008] = { UNDEF,    UNDEF,  "video window descriptor" };
    decode_tag[0x0009] = { UNDEF,    UNDEF,  "CA descriptor" };
    decode_tag[0x000a] = { UNDEF,    UNDEF,  "ISO 639 lang descriptor" };
    decode_tag[0x000b] = { UNDEF,    UNDEF,  "system clock descriptor" };
    decode_tag[0x000c] = { UNDEF,    UNDEF,  "multiplex buffer utilization descriptor" };
    decode_tag[0x000d] = { UNDEF,    UNDEF,  "copyright descriptor" };
    decode_tag[0x000e] = { UNDEF,    UNDEF,  "maximum bitrate descriptor" };
    decode_tag[0x000f] = { UNDEF,    UNDEF,  "private data indicator descriptor" };
    decode_tag[0x0010] = { UNDEF,    UNDEF,  "smoothing buffer descriptor" };
    decode_tag[0x0011] = { UNDEF,    UNDEF,  "STD descriptor" };
    decode_tag[0x0012] = { UNDEF,    UNDEF,  "IBP descriptor" };
    decode_tag[0x0013] = { DATA,     UNDEF,   "" };
    decode_tag[0x001b] = { VIDEO,    MPEG4,  "MPEG-4 video descriptor" };
    decode_tag[0x001c] = { AUDIO,    MPEG4,  "MPEG-4 audio descriptor" };
    decode_tag[0x001d] = { UNDEF,    UNDEF,  "IOD descriptor" };
    decode_tag[0x001e] = { UNDEF,    UNDEF,  "SL descriptor" };
    decode_tag[0x001f] = { UNDEF,    UNDEF,  "FMC descriptor" };
    decode_tag[0x0020] = { UNDEF,    UNDEF,  "external ES ID descriptor" };
    decode_tag[0x0021] = { UNDEF,    UNDEF,  "MuxCode descriptor" };
    decode_tag[0x0022] = { UNDEF,    UNDEF,  "FmxBufferSize descriptor" };
    decode_tag[0x0023] = { UNDEF,    UNDEF,  "multiplexbuffer descriptor" };
    decode_tag[0x0024] = { UNDEF,    UNDEF,  "content labeling descriptor" };
    decode_tag[0x0025] = { UNDEF,    UNDEF,  "metadata pointer descriptor" };
    decode_tag[0x0026] = { UNDEF,    UNDEF,  "metadata descriptor" };
    decode_tag[0x0027] = { UNDEF,    UNDEF,  "metadata STD descriptor" };
    decode_tag[0x0028] = { VIDEO,    UNDEF,  "AVC video descriptor" };
    decode_tag[0x0029] = { UNDEF,    UNDEF,  "IPMP_descriptor (defined in ISO/IEC 13818-11, MPEG-2 IPMP)" };
    decode_tag[0x002a] = { UNDEF,    UNDEF,  "AVC timing and HRD descriptor" };
    decode_tag[0x002b] = { AUDIO,    MPEG2,  "MPEG-2 AAC audio descriptorr" };
    decode_tag[0x002c] = { UNDEF,    UNDEF,  "FlexMuxTiming descriptor" };
    decode_tag[0x0045] = {   VBI,    UNDEF,  "VBI data descriptor" };
    decode_tag[0x0046] = {   VBI,    UNDEF,  "VBI teletext descriptor" };
    decode_tag[0x0051] = { UNDEF,    UNDEF,  "mosaic descriptor" };
    decode_tag[0x0052] = { UNDEF,    UNDEF,  "" };
    decode_tag[0x0056] = {   VBI,    UNDEF,  "teletext descriptor" };
    decode_tag[0x0059] = {   VBI,    UNDEF,  "subtitling descriptor" };
    decode_tag[0x005f] = { UNDEF,    UNDEF,  "private data specifier descriptor" };
    decode_tag[0x0060] = { UNDEF,    UNDEF,  "service move descriptor" };
    decode_tag[0x0065] = { UNDEF,    UNDEF,  "scrambling descriptor" };
    decode_tag[0x0066] = { UNDEF,    UNDEF,  "data broadcast id descriptor" };
    decode_tag[0x006a] = { AUDIO,      AC3,  "AC-3 descriptor (see annexD)" };
    decode_tag[0x006b] = { UNDEF,    UNDEF,  "ancillary data descriptor" };
    decode_tag[0x006f] = {  DATA,    UNDEF,  "application signalling descriptor" };
    decode_tag[0x0070] = { UNDEF,    UNDEF,  "adaptation field data descriptor" };
    decode_tag[0x0074] = { UNDEF,    UNDEF,  "related content descriptor(TS102323[13])" };
    decode_tag[0x007a] = { AUDIO,      AC3,  "enhanced AC-3 descriptor(see annexD)" };
    decode_tag[0x007c] = { AUDIO,    UNDEF,  "AAC descriptor (see annexH)" };
}

*/
