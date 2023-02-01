#include "tchannel.h"

vector <TChannel *> pChannelsArray;



TChannel  *FindChannel(TIPAddress ip_if,TIPAddress ip,TIPPort port)
    {
    for (auto & p : pChannelsArray) {

        if (  (p->Info.IF() == ip_if) &&
              (p->Info.IP() ==  ip) &&
              (p->Info.Port() ==  port) )
                          return p;
    }  // for

    return nullptr;
    }


TChannel  *FindChannel(const string &sect,TIPAddress ip,TIPPort port)
    {
    for (auto & p : pChannelsArray) {

        if (  (p->Info.Section() == sect) &&
              (p->Info.IP() ==  ip) &&
              (p->Info.Port() ==  port) )
                          return p;
    }  // for

    return nullptr;
    }


// New Init
void TChannel::Initialize(TInfo *p,unsigned second_alive)
{
    if (!p) {
        Log.ScrFile("Channel Initialize error - NO Info!\n");
        exit(1);
        }

    ReceiveStop();

    m_pAnalizer = nullptr;
    m_RawTS.Initialize(second_alive);
    ts_summary_bytes=0;

    Info = *p;

// packet fill  from TAnaliserTS
    packet.ptr = nullptr;
    packet.len = 0;

    packet.ip_if = Info.IF();
    packet.ip = Info.IP();
    packet.port = Info.Port();
    packet.ip_src = uint32_t(0);

}





int TChannel::ReceiveStart()
{
    ReceiveStop();

    if( (m_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0 ) {
        Log.ScrFile("error socket open\n");
        return -1;
        }

    SetNonBlocking(m_sock);  // *************** fcntl

    // *************** SO_REUSEADDR
    const int on = 1;
    if (setsockopt(m_sock,SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
        Log.ScrFile("error inp socket reuse addr\n");
        return -1;
        }

    // *************** SO_REUSEPORT
    const int on_p = 1;
    if (setsockopt(m_sock,SOL_SOCKET, SO_REUSEPORT, &on_p, sizeof(on_p)) < 0) {
        Log.ScrFile("error inp socket reuse port\n");
        return -1;
        }

    // Bind to the multicast port
    struct sockaddr_in  multicastAddr {};
    uint16_t            multicastPort = Info.Port().bin();
    memset(&multicastAddr,0,sizeof(multicastAddr));
    multicastAddr.sin_family        = AF_INET;
    multicastAddr.sin_addr.s_addr   = inet_addr(Info.IP().c_str());
    multicastAddr.sin_port          =  htons(multicastPort);
    if (bind(m_sock,(struct sockaddr *)&multicastAddr,sizeof(multicastAddr)) < 0) {
        Log.ScrFile("error socket bind\n");
        return -1;
        }

    // Join to multicast group
    struct ip_mreq multicastRequest {};
    memset(&multicastRequest, 0, sizeof(multicastRequest));
    multicastRequest.imr_multiaddr.s_addr = inet_addr(Info.IP().c_str());  // Specify the multicast group
    multicastRequest.imr_interface.s_addr = htonl(Info.IF().bin());        // Accept multicast from interface
    if (setsockopt(m_sock,IPPROTO_IP,IP_ADD_MEMBERSHIP,(void *)&multicastRequest,sizeof(multicastRequest)) < 0)  {
        Log.ScrFile("error in interface configuration => %s\n",Info.IF().c_str());
        return -1;
        }

   //---------------------------------- DIAG
    // Log.ScrFile("TChannel::ReceiveStart =>  [%s]%s:%s:%s\n",Info.Section().c_str(),Info.IF().c_str(),Info.IP().c_str(),Info.Port().c_str());

    return 0;
}

void TChannel::ReceiveStop()
{
    if (m_sock) {
        close(m_sock);
        m_sock=0;
        //Log.ScrFile("input channel %s stopped\n",Info.Name().c_str());
        }
}


// ****************************************** SECTION MPEG
uint8_t *TChannel::ReceiveSection(unsigned table_id,uint8_t *pkt_data,unsigned pkt_data_length,_mpeg_ts_section &section)
{
    if (!ts_header.payload) return nullptr;


    // ts_header.start_section   - recved

    // section.start                    - inner
    // ret nullptr = skip_packet
    // ret ptr       = if (section.start==0) parse_packet; else skip_packet;
    // skip_packet = return nullptr;




    // ---------------------------------------------------------  RCV START section
    if (ts_header.start_section) {

        // extened table_id ?  -> skip byte
        pkt_data++;
        pkt_data_length--;

        // pkt_data = si ptr
        if (pkt_data[0] !=  table_id) return nullptr;

        if (!section.buffer) {
            section.buffer = new (nothrow) uint8_t[1024];  //0x400
            if (!section.buffer) return nullptr;
        }

        /* common_si
        table_id 8                                // +[0]

        section_syntax_indicator 1     // alwais 1
        '0'  1                                       // alwais 0
        reserved 2                              //
        section_length 12                   // +[1,2] & 0x03ff (max=1021=0x3fd);     full length section from -> include crc32
        transport_stream_id 16          // +[3,4] sid,tsid... (user)
        reserved 2                              //
        version_number 5                  //
        current_next_indicator 1        //  +[5]
        section_number 8                  //  +[6]
        last_section_number 8           //  +[7]
        ...
        */

        section.table_id            = table_id;
        section.full_length         = (1 + 2 + ((pkt_data[1] << 8) | pkt_data[2])) & 0x03ff; // table_id + section_length + length section include crc32
        section.current_length  = 0;


        // need next pkt ?
        if (section.full_length > pkt_data_length) {
            memcpy(section.buffer,pkt_data,pkt_data_length);   // copy si + data
            section.start                 = 1;
            section.current_length = pkt_data_length;
            return nullptr;
        }

        // all in 1 pkt
        memcpy(section.buffer,pkt_data,section.full_length);   // copy si + data
        section.start                 = 0;
        section.current_length = section.full_length;
        return pkt_data;
    } // start_section


    // --------------------------------------------------------- RCV NEXT section part
    if (!section.start)  return nullptr;
    if (!section.buffer) return nullptr;


    unsigned diff_len = section.full_length - section.current_length;
    unsigned store_len = min(diff_len,pkt_data_length);

    if (store_len) {
        uint8_t *dst            = section.buffer + section.current_length;
        memcpy(dst,pkt_data,store_len);
        section.current_length += store_len;
        diff_len = section.full_length - section.current_length;
    }

    if (diff_len)
        return nullptr;

    section.start = 0;  // Final !
    return section.buffer;
}




void TChannel::pes_h264_sps(TRawES *pes,uint8_t *p, unsigned length)
{
    unsigned offset = pes->pes_offset_stored();
    uint8_t *t = p + offset;

    // ранее найденное
    if ( (t[0] == 0x00) &&
         (t[1] == 0x00) &&
         (t[2] == 0x00) &&
         (t[3] == 0x01) &&
         ((t[4] & 0x1f) == 0x07) ) {

        pes->pes_header_store(&t[5],length-5);
//Log.Scr("offset stored =%u\n",offset);
        return;
    }

    // ранее было найдено
    if (offset > 0) return;

    // не нашли - ищем и сохраним в offset
    t = p;
    offset = 0;

    while(1) {

        if ( (t[0] == 0x00) &&
             (t[1] == 0x00) &&
             (t[2] == 0x00) &&
             (t[3] == 0x01) &&
             ((t[4] & 0x1f) == 0x07) )
            {
            pes->pes_header_store(&t[5],length-5);
            pes->pes_offset_store(offset);
//Log.Scr("offset found=%u\n",offset);
            return;
            }

        t++;
        offset++;
        length--;
        if ((length-5)==0) {

//Log.Scr("offset NOT found\n");
            return;
        }
    }
}

void TChannel::pes_h265_sps(TRawES *pes, uint8_t *p, unsigned length)
{
    unsigned offset = pes->pes_offset_stored();
    uint8_t *t = p + offset;

    // ранее найденное
    if ( (t[0] == 0x00) &&
         (t[1] == 0x00) &&
         (t[2] == 0x00) &&
         (t[3] == 0x01) ) {
        uint16_t header = (uint16_t(t[4]) << 8) | t[5];
        if (header & 0x8000) return; // ignore forbidden_bit == 1
        unsigned nal_unit_type   = (header & 0x7e00) >> 9;
        if (nal_unit_type == 0x21) { // 0x21 = SPS
            pes->pes_header_store(&t[4],length-4);
            pes->pes_offset_store(offset);
//Log.Scr("SPS offset stored =%u\n",offset);
            return;
            }
        }

    // ранее было найдено
    if (offset > 0) return;

    // не нашли - ищем и сохраним в offset
    t = p;
    offset = 0;

    while(1) {

        if ( (t[0] == 0x00) &&
             (t[1] == 0x00) &&
             (t[2] == 0x00) &&
             (t[3] == 0x01) ) {
            uint16_t header = (uint16_t(t[4]) << 8) | t[5];
            if (header & 0x8000) return; // ignore forbidden_bit == 1
            unsigned nal_unit_type   = (header & 0x7e00) >> 9;
            if (nal_unit_type == 0x21) { // 0x21 = SPS
                pes->pes_header_store(&t[4],length-4);
                pes->pes_offset_store(offset);
    //Log.Scr("SPS offset found=%u\n",offset);
                return;
                }
            }


        t++;
        offset++;
        length--;
        if ((length-6)==0) {

//Log.Scr("offset NOT found\n");
            return;
        }
    }
}







// start_section and !PMT
TRawES *TChannel::ParsePES(unsigned pid, uint8_t *pkt,unsigned pkt_length)
{
    TRawES *pes = m_RawTS.CGetData(pid);
    if (!pes) return nullptr;

    if (pkt_length <  3)  return pes;

    //
    //  PES Header copy
    //

    //
    // start code  =  0x000001
    //                   =  0x000001[e0-ef] - video
    //                   =  0x000001[c0-df] - audio

    //   PES Header = start code + 8 + 1  (this = p)
    //                            =  0x000001b3  - Video MPEG2_SEQUENCE_START  - ES_MPEG2Video::Parse_MPEG2Video_SeqStart
    //                            =  0x000001f0   - ECM
    //                            =  0x000001f1   - EMM

     //                           = 0x000000014e  - HEVC IDR
     //                           = 0x0000000107  - H264 SPS

    //                            = if (es_buf[0] >= 0x10 || es_buf[0] <= 0x1F)    = Teletext
    //                            =  if (l >= 2 || es_buf[0]== 0x20 || es_buf[1] == 0x00)  and if(es_buf[l-1] == 0xff) = subtitle

    //                            = 0xff[]   //if ((buf_ptr[0] == 0xFF && (buf_ptr[1] & 0xE0) == 0xE0))   AUDIO  ()
    //                            = if ((buf_ptr[0] == 0x0b && buf_ptr[1] == 0x77)) AC3

    if (pkt[0]!=0x00 || pkt[1]!=0x00 || pkt[2]!=0x01 )  return pes;

// ===================MAX_PES_SIZE = 128    and + pes header


    if (pkt_length <  8)  return pes;

// ===================

    // Stream ID
    unsigned pes_type = pkt[3];  //Audio(0xC0-0xDF), Video(0xE0-0xEF) ECM(0xf0)  EMM(0xf1)

    unsigned pes_flags  = pkt[7];
    //[[maybe_unused]] unsigned pes_length = pkt[8];

    unsigned pts_flag =  pes_flags & 0x80;
    unsigned dts_flag =  pes_flags & 0x40;
    unsigned escr_flag =  pes_flags & 0x20;
    unsigned esrate_flag =  pes_flags & 0x10;

    //[[maybe_unused]] unsigned trick_mode_flag =  pes_flags & 0x08;
    unsigned copy_info_flag =  pes_flags & 0x04;
    unsigned pes_crc_flag =  pes_flags & 0x02;
    unsigned pes_ext_flag =  pes_flags & 0x01;

    unsigned sz = 0;
    if (pts_flag) { sz+= 5; }
    if (dts_flag) { sz+= 5; }
    if (escr_flag) { sz+= 6; }
    if (esrate_flag) { sz+= 3; }  // uint16 in 50 bytes/sec

    // if (trick_mode_flag) { sz+= ?; }  not used
      if (copy_info_flag) { sz+= 1; }
      if (pes_crc_flag) { sz+= 2; }
      if (pes_ext_flag) { sz+= 1; }

      // PES header data length  = pkt[8]
      unsigned pes_header_lenght =  pkt[8];

      if ((pkt_length-8) <  pes_header_lenght) {
            //Log.Scr("ParsePES Err: ip=%s pid=%u len=%u\n",Info.IP().c_str(),pid,pkt_length);
      return pes;
      }
      unsigned work_header_len = pkt_length - pes_header_lenght;


      uint8_t *p   = &pkt[8];
      p           += pkt[8]+1;  // code work




      // ===================MAX_PES_SIZE = 128    and + pes header
       //   if (pkt_length <  160) {
      //Log.Scr("ParsePES xxx: ip=%s pid=%u len=%u    pes_header_lenght=%u\n",Info.IP().c_str(),pid,pkt_length,pes_header_lenght);
       //   return pes;
      //    }
      // ===================






      switch (pes->stream_info.type_stream)
      {
      case STREAM_TYPE_VIDEO_MPEG1:
      case STREAM_TYPE_VIDEO_MPEG2:
//Log.Scr("ParsePES VIDEO_MPEG: ip=%s pid=%u len=%u    work_header_len=%u\n",Info.IP().c_str(),pid,pkt_length,work_header_len);

          if ( p[0] == 0x00 && p[1] == 0x00 && p[2] == 0x01 && p[3] == 0xb3 )  {
              //MPEG2 Video
              pes->pes_header_store(&p[4],work_header_len-4);
              pes->pes_offset_store(0);
              return pes;
          }

        break;
      case STREAM_TYPE_AUDIO_MPEG1:
      case STREAM_TYPE_AUDIO_MPEG2:
//Log.Scr("ParsePES AUDIO_MPEG: ip=%s pid=%u len=%u    work_header_len=%u\n",Info.IP().c_str(),pid,pkt_length,work_header_len);
         // pes->pes_header_store(p,work_header_len);
         // return pes;
          if (*p++ != 0xff)   return nullptr;
          pes->pes_header_store(p,work_header_len-1);
          pes->pes_offset_store(0);
          return pes;
        break;

      case STREAM_TYPE_AUDIO_AAC:
      case STREAM_TYPE_AUDIO_AAC_ADTS:
      case STREAM_TYPE_AUDIO_AAC_LATM:
//Log.Scr("ParsePES AUDIO_AAC: ip=%s pid=%u len=%u    work_header_len=%u\n",Info.IP().c_str(),pid,pkt_length,work_header_len);

          //if (*p++ != 0xff)   return nullptr;
          pes->pes_header_store(p,work_header_len);
          pes->pes_offset_store(0);
          return pes;
          break;

      case STREAM_TYPE_VIDEO_H264:
//Log.Scr("ParsePES VIDEO_H264: ip=%s pid=%u len=%u    work_header_len=%u\n",Info.IP().c_str(),pid,pkt_length,work_header_len);
        {
          pes_h264_sps(pes,p,work_header_len);
          return pes;
        }
        break;

      case STREAM_TYPE_VIDEO_HEVC:
//Log.Scr("ParsePES VIDEO_HEVC: ip=%s pid=%u len=%u    work_header_len=%u\n",Info.IP().c_str(),pid,pkt_length,work_header_len);
      {
         //PrintX(p,64);
          pes_h265_sps(pes,p,work_header_len);
          return pes;
      }
        break;

      case STREAM_TYPE_AUDIO_AC3:
      case STREAM_TYPE_AUDIO_EAC3:
//Log.Scr("ParsePES AUDIO_AC3: ip=%s pid=%u len=%u    work_header_len=%u\n",Info.IP().c_str(),pid,pkt_length,work_header_len);
//PrintX(p,16);
          //if (*p++ != 0xff)   return nullptr;
          pes->pes_header_store(p,work_header_len);
          pes->pes_offset_store(0);
          return pes;

        break;
      case STREAM_TYPE_DVB_SUBTITLE:
//Log.Scr("ParsePES DVB_SUBTITLE: ip=%s pid=%u len=%u    work_header_len=%u\n",Info.IP().c_str(),pid,pkt_length,work_header_len);
          pes->pes_header_store(p,work_header_len);
          pes->pes_offset_store(0);
          return pes;
        break;
      case STREAM_TYPE_DVB_TELETEXT:
//Log.Scr("ParsePES DVB_TELETEXT: ip=%s pid=%u len=%u    work_header_len=%u\n",Info.IP().c_str(),pid,pkt_length,work_header_len);
          pes->pes_header_store(p,work_header_len);
          pes->pes_offset_store(0);
          return pes;
        break;

      default:
//Log.Scr("ParsePES DVB_UNDEFUNED: ip=%s pid=%u len=%u    work_header_len=%u\n",Info.IP().c_str(),pid,pkt_length,work_header_len);
          pes->pes_header_reset();
          //pes->pes_header_store(p,work_header_len);
          pes->pes_offset_store(0);
          return pes;
        break;
      }

return pes;
 }



  void TChannel::Callback_Parser(uint8_t *rcvData, unsigned recvLen, uint32_t ip_src)
  {
      packet.ptr = rcvData;
      packet.len = recvLen;
      packet.ip_src = ip_src;
      m_RawTS.SetSrc(ip_src);

      ts_summary_bytes +=recvLen;

      bool first=true;
//Log.Scr("Len=%u",recvLen);

      _mpeg_ts *rcvts = reinterpret_cast<_mpeg_ts *>(rcvData);
      unsigned mpeg_ts_count=recvLen/188;

      for (unsigned i=0;i<mpeg_ts_count;++i) {

          //decode ts header
          uint_fast32_t header = bswap_32(rcvts->header);
          ts_header.sync          = (header & 0xff000000) >> 24;
          ts_header.error         = (header & 0x00800000) ? 1 : 0;
          ts_header.start_section = (header & 0x00400000) ? 1 : 0;
          ts_header.priority      = (header & 0x00200000) ? 1 : 0;
          ts_header.pid           = (header & 0x001fff00) >> 8;
          ts_header.scrambled     = (header & 0x000000c0) >> 6;
          ts_header.adaptation    = (header & 0x00000020) ? 1 : 0;
          ts_header.payload       = (header & 0x00000010) ? 1 : 0;
          ts_header.counter       = (header & 0x0000000f);

          if (ts_header.sync!=0x47)   { ++rcvts; continue; }
          if (ts_header.error)        { ++rcvts; continue; }

         if (measurer  && ts_header.pid==measure_pid) {
             if (first) {
                Measure.Packet((uint8_t *)rcvts->data);
                first=false;
                }
            }


          TRawES *pes = nullptr;

          if (ts_header.adaptation) {
              ts.adapt_length = rcvts->data[0];

              if (ts.adapt_length > (188 - 4 - 1 ) )  {  // length = 188 - sizeof header - sizeof adapt_length
                  // illegal  length
                  ts.adapt_length = 0;
                  //ts_header.adaptation = 0; // skip failed adaptation field
                  ++rcvts; continue;          // skip failed pkt
              }

              //ts.adapt_field  = &rcvts->data[1];
              ts.pdata_pkt      = &rcvts->data[1+ts.adapt_length]; // skip byte adapt_length
              ts.data_length   = 184 - 1 - ts.adapt_length;
          }
          else {
              ts.adapt_length = 0;
              //ts.adapt_field  = nullptr;
              ts.pdata_pkt      = &rcvts->data[0];
              ts.data_length   = 184; //
          }



          // ROOT pids (all)
          if (ts_header.pid < 0x0020) {

              if (ts_header.pid == 0x0000)         pes = ParsePAT(ts.pdata_pkt,ts.data_length);
              //else if (ts_header.pid == 0x0001)  pes = ParseCAT(ts.pdata_pkt,ts.data_length);
              //else if (ts_header.pid == 0x0010)  pes = ParseNIT(ts.pdata_pkt,ts.data_length);
              else if (ts_header.pid == 0x0011)  pes = ParseSDT(ts.pdata_pkt,ts.data_length);
              else if (ts_header.pid == 0x0014)  pes = ParseTDT(ts.pdata_pkt,ts.data_length);
              else  pes = ParseCOMMON_Other(ts_header.pid);

              if (pes) {
                  if (pes->next_cc != ts_header.counter) pes->cc_errors++;
                  pes->next_cc      = ++ts_header.counter % 16;
                  if (ts_header.scrambled) pes->scrambled++;
                  pes->bytes       += 188; // - adapt_length;
              }
              ++rcvts; continue;
          }

          // STUFF pid (no calc CCerror)
          if (ts_header.pid == 0x1fff) {
              pes = ParseSTUFF();
              if (pes) {
                  pes->cc_errors = 0;
                  pes->scrambled = 0;
                  pes->bytes    += 188; // - adapt_length;
              }
              ++rcvts; continue;
          }


          if (ts_header.start_section) {

              // SECTION (PMT)
              pes = ParsePMT(ts_header.pid,ts.pdata_pkt,ts.data_length);
              if (!pes) {

                  // Not PMT -> SECTION PES HEADER Parser
                  pes=ParsePES(ts_header.pid,ts.pdata_pkt,ts.data_length);
                  if (!pes) {  ++rcvts; continue; }

                  }

              if (pes->next_cc != ts_header.counter) pes->cc_errors++;
              pes->next_cc = ++ts_header.counter % 16;
              if (ts_header.scrambled) pes->scrambled++;
              pes->bytes += 188; // - adapt_length;

              ++rcvts; continue;
          }

          // OTHER (pid > 0x1f ,pid != 1fff ,pid != PMT)
          // no start_section
          pes = m_RawTS.CGetData(ts_header.pid);
          if (pes) {

              // *********************** PCR
              if (pes->Pcr.Found()) {
                  pes->Pcr.PacketWitchPcrPid();
              }
              if (ts_header.adaptation) {
                  ParsePCR(&rcvts->data[0],pes);
              }
              // ***********************

              if (ts_header.payload)
              {

// **************************
//if (ts_header.pid==1022)  {
//    pes->cc_errors++;
//}
// ***************************



                  if (pes->next_cc != ts_header.counter) pes->cc_errors++;
                  pes->next_cc = ++ts_header.counter % 16;
                  if (ts_header.scrambled) pes->scrambled++;
                  pes->bytes += 188;   // - adapt_length;
              }
              else {
                  // if pcr ?
                  //printf("no payload => pid=%u\n",pes->pid);
              }

              ++rcvts; continue;
          }



          // NOT DATA (pid > 0x1f ,pid != 1fff ,pid != PMT)  (Not In PMT)(emm,ecm,unexpected)
          {
          pes = m_RawTS.CInsertUnexpected(ts_header.pid);
          if (pes) {
              if (pes->next_cc != ts_header.counter) pes->cc_errors++;
               pes->next_cc = ++ts_header.counter % 16;
               if (ts_header.scrambled) pes->scrambled++;
               pes->bytes += 188;   // - adapt_length;
              }
          }

          ++rcvts; continue;
      }  // Next PKT

  }

