


#include "tdecode.h"





ES_MPEG2Audio  m2audio;
ES_MPEG2Video  m2video;
ES_AC3         ac3audio;
ES_AAC         aacaudio;
ES_Teletext    mteletext;
ES_Subtitle    msubtitle;
ES_h264        mh264;
ES_hevc        mhevc;

TDecode Decode;


// length visual
size_t VisualLength(const string &str)
{
    wchar_t buff[512] {};
    std::mbstowcs(&buff[0],str.c_str(),str.size());
    wstring s=buff;
    return s.size();
}

static const uint16_t iso588595[128] = {
    //first = 0xa0 offset
    0x2020, 0x2020,0x2020,0x2020,0x2020,0x2020,0x2020,0x2020,0x2020,0x2020,0x2020,0x2020,0x2020,0x2020, 0x2020,0x2020,  //80
    0x2020, 0x2020,0x2020,0x2020,0x2020,0x2020,0x2020,0x2020,0x2020,0x2020,0x2020,0x2020,0x2020,0x2020, 0x2020,0x2020,  //90
    0x2020, 0xD001,0x2020,0x2020,0x2020,0x2020,0x2020,0x2020,0x2020,0x2020,0x2020,0x2020,0x2020,0x2020, 0x2020,0x2020,  //A0
    0xD090,0xD091,0xD092,0xD093,0xD094,0xD095,0xD096,0xD097,0xD098,0xD099,0xD09a,0xD09b,0xD09c,0xD09d,0xD09e,0xD09f,   //B0
    0xD0A0,0xD0A1,0xD0A2,0xD0A3,0xD0A4,0xD0A5,0xD0A6,0xD0A7,0xD0A8,0xD0A9,0xD0AA,0xD0Ab,0xD0Ac,0xD0Ad,0xD0Ae,0xD0Af, //C0
    0xD0B0,0xD0B1,0xD0B2,0xD0B3,0xD0B4,0xD0B5,0xD0B6,0xD0B7,0xD0B8,0xD0B9,0xD0BA,0xD0Bb,0xD0Bc,0xD0Bd,0xD0Be,0xD0Bf, //D0
    0xD180,0xD181,0xD182,0xD183,0xD184,0xD185,0xD186,0xD187,0xD188,0xD189,0xD18A,0xD18B,0xD18C,0xD18D,0xD18E,0xD18F, //E0
    0x2020,0xD191,0x2020,0x2020,0x2020,0x2020,0x2020,0x2020,0x2020,0x2020,0x2020,0x2020,0x2020,0x2020,0x2020,0x2020 //F0
};


int Decode588595v2(unsigned max_size_out,char *dst,uint8_t *src,unsigned insize)
{
    if (insize==0) return 0;
    if (max_size_out < 3) return 0;

    int size = 0;



    char *dst_max = dst + max_size_out;

    while(insize) {
        insize--;
        uint8_t ch = *src++;

        if ( ch  < 32  ) {   continue; }
        if ( ch  < 128  ) {
                   if (dst > (dst_max-2)) break;
                 *dst++ = char(ch);
                 size+=1;
                 continue;
                 }

        if (ch < 0xb0)  continue;
        if (ch > 0xef)  continue;

        ch -= 128;


        if (dst > (dst_max-2)) break;

        uint16_t outch = iso588595[ch];
        *dst++ = char(outch >> 8);
        *dst++ = char(outch & 0xff);
        size +=2;
        }
    *dst++='\0';
    *dst++='\0';
    size +=2;

    return size;
}


void ParsePesHeader(TAnalizerES *pes)
{
    if (!pes) return;

    switch (pes->es.stream_info.type_stream)
    {
    case STREAM_TYPE_VIDEO_MPEG1:
    case STREAM_TYPE_VIDEO_MPEG2:
        m2video.Parse(pes);
      break;
    case STREAM_TYPE_AUDIO_MPEG1:
    case STREAM_TYPE_AUDIO_MPEG2:
      m2audio.Parse(pes);
      break;
    case STREAM_TYPE_AUDIO_AAC:
    case STREAM_TYPE_AUDIO_AAC_ADTS:
    case STREAM_TYPE_AUDIO_AAC_LATM:
        aacaudio.Parse(pes);
      break;
    case STREAM_TYPE_VIDEO_H264:
        mh264.Parse(pes);
      break;
    case STREAM_TYPE_VIDEO_HEVC:
        mhevc.Parse(pes);
      break;
    case STREAM_TYPE_AUDIO_AC3:
    case STREAM_TYPE_AUDIO_EAC3:
        ac3audio.Parse(pes);
      break;
    case STREAM_TYPE_DVB_SUBTITLE:
        msubtitle.Parse(pes);
      break;
    case STREAM_TYPE_DVB_TELETEXT:
        mteletext.Parse(pes);
      break;
    default:
      break;
    }
}


const string GetTypeText(TAnalizerES *pes)
{
    if (!pes) return "";

    switch (pes->es.stream_info.type)
    {
    case PAT:   return "PAT";
    case CAT:   return "CAT";
    case TSDT:  return "TSDT";
    case IPMP:  return "IPMP";
    case NIT:   return "NIT";
    case SDT:   return "SDT";
    case EIT:   return "EIT";
    case RST:   return "RST";
    case TDT:   return "TDT";

    case STUFF: return "STUFF";

    case EMM:   return "EMM";
    case ECM:   return "ECM";

    case PMT:   return "PMT";

    case UNDEF: return "UNDEF";
    case VIDEO:   return "VIDEO";
    case AUDIO:   return "AUDIO";
    case VBI:  return "VBI";
    case DATA:  return "DATA";
    };
    return "";
}


const string GetStreamText(TAnalizerES *pes)
{
    if (!pes) return "";

    string ss;

switch (pes->es.stream_info.type_stream) {

        case STREAM_TYPE_VIDEO_MPEG1:
        case STREAM_TYPE_VIDEO_MPEG2:
            m2video.GetText(pes,ss);
          break;
        case STREAM_TYPE_AUDIO_MPEG1:
        case STREAM_TYPE_AUDIO_MPEG2:
          m2audio.GetText(pes,ss);
          break;
        case STREAM_TYPE_AUDIO_AAC:
        case STREAM_TYPE_AUDIO_AAC_ADTS:
        case STREAM_TYPE_AUDIO_AAC_LATM:
            aacaudio.GetText(pes,ss);
          break;
        case STREAM_TYPE_VIDEO_H264:
            mh264.GetText(pes,ss);
          break;
        case STREAM_TYPE_VIDEO_HEVC:
            mhevc.GetText(pes,ss);
          break;
        case STREAM_TYPE_AUDIO_AC3:
        case STREAM_TYPE_AUDIO_EAC3:
            ac3audio.GetText(pes,ss);
          break;
        case STREAM_TYPE_DVB_SUBTITLE:
            msubtitle.GetText(pes,ss);
          break;
        case STREAM_TYPE_DVB_TELETEXT:
            mteletext.GetText(pes,ss);
          break;

        default:
            //char buf[64];
            //sprintf(buf,"[%x]",pes->stream_info.type);
            //ss = buf;

            break;
        }


return ss;
}


const char* GetStreamCodecName(es_stream stream_type)
{
  switch (stream_type)
  {
    case STREAM_TYPE_VIDEO_MPEG1:
      return "mpeg1video";
    case STREAM_TYPE_VIDEO_MPEG2:
      return "mpeg2video";
    case STREAM_TYPE_AUDIO_MPEG1:
      return "mp1";
    case STREAM_TYPE_AUDIO_MPEG2:
      return "mp2";
    case STREAM_TYPE_AUDIO_AAC:
      return "aac";
    case STREAM_TYPE_AUDIO_AAC_ADTS:
      return "aac";
    case STREAM_TYPE_AUDIO_AAC_LATM:
      return "aac_latm";
    case STREAM_TYPE_VIDEO_H264:
      return "h264";
    case STREAM_TYPE_VIDEO_HEVC:
      return "hevc";
    case STREAM_TYPE_AUDIO_AC3:
      return "ac3";
    case STREAM_TYPE_AUDIO_EAC3:
      return "eac3";
    case STREAM_TYPE_DVB_TELETEXT:
      return "teletext";
    case STREAM_TYPE_DVB_SUBTITLE:
      return "dvbsub";
    case STREAM_TYPE_VIDEO_MPEG4:
      return "mpeg4video";
    case STREAM_TYPE_VIDEO_VC1:
      return "vc1";
    case STREAM_TYPE_AUDIO_LPCM:
      return "lpcm";
    case STREAM_TYPE_AUDIO_DTS:
      return "dts";
    case STREAM_TYPE_PRIVATE_DATA:
    default:
      return "data";
  }
}


void PRINT_ANALIZED_TS(TAnalizerTS *p,const string& hdr)
{
    if (!p) return;

    string out_str;// = "Analized -----------------------------------------------------------\n";

    string  traffic;
    TAnalizerES *pes;


    // COMMON
    size_t sz = p->CountRoot();
    for (size_t i=0;i<sz;++i) {
        pes= p->GetRoot(i);
        if (!pes) continue;

        out_str +=
                "CES: pid="  + to_string(pes->Pid()) +
                 " ok="      + to_string(pes->StateOk()) +
                 " ui="      + to_string(pes->StateUI()) +
                 " "         + GetTypeText(pes) +
                 " desc="    + GetStreamText(pes) +
                 " lang="    + string(&pes->es.stream_info.language[0]) +
                 " traf="     + format_traffic(pes->RcvBytes()) +
                " si_len="  + to_string(pes->es.si_header_length) +
                 " pes_len="  + to_string(pes->es.pes_header_length) +
                 "\n";
        }



    // STUFF
   pes = p->GetStuff();
   if (pes)  {

       out_str +=
               "SES: pid="  + to_string(pes->Pid()) +
                " ok="      + to_string(pes->StateOk()) +
                " ui="      + to_string(pes->StateUI()) +
                " "         + GetTypeText(pes) +
                " desc="    + GetStreamText(pes) +
                " lang="    + string(&pes->es.stream_info.language[0]) +
                " traf="    + format_traffic(pes->RcvBytes()) +
                " si_len="  + to_string(pes->es.si_header_length) +
                " pes_len="  + to_string(pes->es.pes_header_length) +
                "\n";
   }



   // OUT OF BAND
   size_t cnt = p->CountUnexpected();
   for (size_t i=0;i<cnt;++i) {
       pes = p->GetUnexpected(i);
       if (!pes) continue;

       out_str +=
               "UES: pid="  + to_string(pes->Pid()) +
                " ok="      + to_string(pes->StateOk()) +
                " ui="      + to_string(pes->StateUI()) +
                " "         + GetTypeText(pes) +
                " desc="    + GetStreamText(pes) +
                " lang="    + string(&pes->es.stream_info.language[0]) +
                " traf="    + format_traffic(pes->RcvBytes()) +
                " si_len="  + to_string(pes->es.si_header_length) +
                " pes_len="  + to_string(pes->es.pes_header_length) +
                "\n";
      }



    // PMT
    size_t count_pmt = p->CountPmt();
    for (size_t i=0;i<count_pmt;++i) {

        pes = p->GetPmt(i);
        if (!pes) continue;

        out_str += "PES: pid="+ to_string(pes->Pid()) +
                   " ok="     + to_string(pes->StateOk()) + // StateES  StatePMT
                   " ui="     + to_string(pes->StateUI()) +
                   " PMT"     +
                   " tsid="   + to_string(pes->PmtTSid()) +
                   " sid="    + to_string(pes->PmtSid()) +
                   " traf="   + format_traffic(pes->RcvBytes()) +
                   " ["       + pes->PmtProvider() +
                   "]["       + pes->PmtServiceName() +
                   "][ini="   + p->Info.Name() + //+ pes->PmtNameUI() +
                   "]" +
                    //"\t pmt_info_len=" +  to_string(pes->PesPmtInfoLength()) +

                   // " \t "    + pes->TextScramble() +
                   // " "       + pes->TextCCError() +
                   " si_len="  + to_string(pes->es.si_header_length) +
                   " pes_len="  + to_string(pes->es.pes_header_length) +
                   "\n";


        // ES in PMT
        size_t count_es = p->CountEs(i);
        for (size_t j=1;j<count_es;++j) {

            pes = p->GetEs(i,j);
            if (!pes) continue;


            out_str += "ES" + to_string(j) + ": " +
                       " pid="   + to_string(pes->Pid()) +
                       " ok=" + to_string(pes->StateOk()) +
                       " ui="    + to_string(pes->StateUI()) +
                       " "       + GetTypeText(pes) +
                       " "       + GetStreamText(pes) + //+ pes->PmtServiceName() +
                       " lang="    + string(&pes->es.stream_info.language[0]) +
                       " traf="  + format_traffic(pes->RcvBytes()) +
                       " ccerr=" + to_string(pes->RcvErrors()) +
                       //"\t pmt_es_info_len=" +  to_string(pes->PesPmtInfoLength()) +
                      // " "       + pes->TextScramble() +
                      // " "       + pes->TextCCError() +
                     " "   + // pes->GetPesDesc() +
                    " si_len="  + to_string(pes->es.si_header_length) +
                    " pes_len="  + to_string(pes->es.pes_header_length) +

                    " traffic_min="  + to_string(pes->es.min_traffic_limit) +

                    " es_ok="  + to_string(pes->es.es_ok) +
                    " es_ccerr="  + to_string(pes->RcvErrors()) +



                    " Codec=" + string(GetStreamCodecName(pes->es.stream_info.type_stream)) +
                       "\n";

            if (pes->IsPCR()) {
                out_str += "ES" + to_string(j) + ": " +
                           " pid="       + to_string(pes->Pid()) +
                           " ok="     + to_string(pes->StateOk()) +
                           " ui="        + to_string(pes->StateUI()) +
                           " PCR"        +
                           " pcr/s="     + to_string(pes->PcrPerSecond()) +
                           " Estimated=" + to_string(pes->PcrBitRate()) +
                           " MIN="       + to_string(pes->PcrBitRateMin()) +
                           " MAX="       + to_string(pes->PcrBitRateMax()) +
                           "\n";
                }

            } //For ES in PMT

        } //For PMT



    count_pmt = p->CountPmt();
    for (unsigned i=0;i<count_pmt;i++) {

        pes = p->GetPmt(i);
        if (!pes) continue;

        string type_stream = (count_pmt > 1) ?   "M" : "S";

        out_str +=  string("FIN ") +
                    " Type="       + type_stream +
                    " pmt="        + to_string(pes->Pid()) +
                    " count_es="   + to_string(p->CountEs(i)) +
                    " State="      + to_string(pes->StateOk()) +    // StateES StatePMT
                    " StateUI="    + to_string(pes->StateUI()) +
                    " UpdateUI="   + to_string(pes->StateUpdated()) +
                    " Name="       + pes->PmtNameUI() +
                  //  " CountRawES=" + to_string(Analized.CountData(i)) +
                    "\n";
       }

    string inf = hdr + " " + p->Info.Section() + " " + p->Info.IP().c_str() + ":" + p->Info.Port().c_str();
    Log.Scr("%s\n%s\n",inf.c_str(),out_str.c_str());
}






