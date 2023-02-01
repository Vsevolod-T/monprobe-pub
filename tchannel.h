#pragma once

#include "theader.h"

#include "tdecode.h"
#include "trawes.h"
#include "trawts.h"

#include "tchannelmeasure.h"

class TBuffer;
class TChannelOut;


class TChannel
{
private:


_mpeg_ts_header ts_header;

    struct _mpeg_ts_pkt {
       // uint8_t   *adapt_field;    // ptr to adaptation field or nullptr
        unsigned   adapt_length;   // length adaptation field
        uint8_t   *pdata_pkt;           // ptr to data packet
        unsigned   data_length;    // length data packet
        }ts;

    struct _mpeg_ts {
        uint32_t   header;
        uint8_t    data[184];
    }__attribute__((packed));

    struct _mpeg_ts_section {
        uint8_t   *buffer;   // 1024 bytes  (SI+SECTION DATA)
        unsigned   start;         //
        unsigned   table_id;

        unsigned   full_length;      //
        unsigned   current_length;

        _mpeg_ts_section() {
            buffer = nullptr;
            full_length =
            start =
            current_length =
            table_id = 0;
            }

        ~_mpeg_ts_section() {
            delete [] buffer;
            }

        unsigned data_size() const {
                return current_length;
            }
        };

     TRawTS  m_RawTS;

     // ****************************************** mpeg sections

     _mpeg_ts_section m_common_sections[0x0020] {};
     _mpeg_ts_section  m_section_pmt {};

     uint8_t *ReceiveSection(unsigned table_id,uint8_t *pkt_data,unsigned pkt_data_length,_mpeg_ts_section &section);



    // ******************************************
    int           m_sock;






    // ******************************************
    TRawES *GetSID(const unsigned sid)
        {
        unsigned count_pmt = m_RawTS.GetCountPmt();
        for (unsigned i=0;i<count_pmt;++i) {
            TRawES *p = m_RawTS.GetIdxData(i,0);
            if (p && p->sid==sid) return p;
            }
        return nullptr;
        }


    // ****************************************** PAT
    TRawES *ParsePAT(uint8_t *data,unsigned data_length)
        {
        // Insert PAT
        TRawES *p_pat = m_RawTS.CInsertCommon(0);
        if (!p_pat) return nullptr;

        //fill this PAT
        p_pat->sid      = 0;
        //p_pat->pmt      = 0;
        p_pat->pid      = 0;
        p_pat->stream_info.type = PAT;
        p_pat->stream_info.type_stream = STREAM_TYPE_UNKNOWN;
        //p_pat->raw_es_data_length=0;

        p_pat->si_header_reset();

        uint8_t *p = ReceiveSection(0x00,data,data_length,m_common_sections[0x0000]);
        // p == nullptr => skip_packet
        // if (section.start==0) parse_packet; else skip_packet;

if(!p) return p_pat;
//if (m_common_sections[0x0000].start) return p_pat;

        // p = ptr (SI + SECTION + CRC32)

        uint8_t *p_end = p + m_common_sections[0x0000].data_size() - 4; // no length crc32

        // uint8_t table_id = get8(p);                //
        // uint16_t section_length = get16(p); //
        p+=3;

        p_pat->tsid  = get16(p); //transport_stream_id

        // uint8_t version = get8(p); // version_number + current_next_indicator
        // uint8_t section = get8(p); // section_number
        // uint8_t last_section = get8(p); // last_section_number


        //uint8_t section = p[1];
        //uint8_t last_section = p[4];
//Log.Scr("section=%u last section=%u\n",section,last_section);

        p+=3;

       // p = ptr to patN

        unsigned pmt_idx=0;   // first index PMT

        while(p < p_end) {

            unsigned sid = get16(p);
            unsigned pid = get16(p) & 0x1fff;

            // Network ID ?
            if (sid==0)  continue;

            // add pmt stream
            TRawES *p_pmt = m_RawTS.CInsertData(pid,pmt_idx,0);
            if (!p_pmt) continue;

            p_pmt->sid     = sid;
            p_pmt->pid     = pid;
            p_pmt->stream_info.type = PMT;
            p_pmt->stream_info.type_stream = STREAM_TYPE_UNKNOWN;

            // +++
            //p_pmt->raw_si_length = 0;
            p_pmt->si_header_reset();
            // +++

            ++pmt_idx;
            } //parse next pmt

         m_RawTS.CSetCountPmt(pmt_idx);

        return  p_pat;
        }

    // ****************************************** PMT
    TRawES *ParsePMT(unsigned rcvpid,uint8_t *data,unsigned data_length)
        {

        TRawES *p_pmt =  m_RawTS.CGetData(rcvpid);
        if (!p_pmt) return nullptr;

        uint8_t *p = ReceiveSection(0x02,data,data_length,m_section_pmt);

        // ret ptr       = if (section.start==0) parse_packet; else return nullptr;
        // skip_packet = return nullptr;

if(!p) return nullptr;               // not PMT
if (m_section_pmt.start) return p_pmt;// continue rcv PMT section

// this = (p && m_section_pmt.start==0)

         // p = ptr (SI + SECTION + CRC32)

        uint8_t *p_end = p + m_section_pmt.data_size() - 4; // no length crc32

        // uint8_t table_id = get8(p);         //
        // uint16_t section_length = get16(p); //
        p+=3;

        p_pmt->tsid = get16(p); // program_number

       // version = get8(p); // version_number + current_next_indicator
       // section = get8(p); // section_number
       // last_section = get8(p); // last_section_number
       p+=3;

       //uint16_t PCR_PID = get16(p); //PCR_PID
       p+=2;

       unsigned program_info_length = get16(p) & 0x03ff;  // !!!! UPDATE !!! 0x03ff

       // +++
       //p_pmt->raw_si_length = min(program_info_length, MAX_PMT_ES_INFO);  //
       //memcpy(p_pmt->raw_si_data,p,p_pmt->raw_si_length);                   // store in pmt es
       // +++

       //p_pmt->raw_si_length = program_info_length;

       p_pmt->si_header_store(p,program_info_length);

        p             += program_info_length; // skip descriptor

        // p = ptr to pmtN

        unsigned idx_pmt = p_pmt->pool_idx_pmt;
        unsigned idx_es  = 1;   // first index RawES   , 0 - PMT

        while(p  <  p_end) {

            // +++
            uint8_t *pmt_info_ptr = p;
            // +++

            uint8_t  es_type_rcv    = get8(p);
            unsigned es_pid         = get16(p) & 0x1fff;
            unsigned es_info_length = get16(p) & 0x03ff; // descriptors size



            // +++
            //unsigned sect_len = min( (5 +  es_info_length) , MAX_PMT_ES_INFO);
            // +++

            // add es stream
            TRawES *p_es = m_RawTS.CInsertData(es_pid,idx_pmt,idx_es);
            if (!p_es) break;

            // +++
            // 5 = stream_type(8) + es_pid(16) + es_info_length(16) + data
             p_es->si_header_store(pmt_info_ptr,(5 +  es_info_length));
            // +++

             p_es->sid     = 0;
             p_es->pid     = es_pid;

              // set es type & type_stream
             pmt_set_stream_info(p_es,es_type_rcv);
             pmt_parse_pes_descriptor(p_es,p,es_info_length);

            ++idx_es;
            p += es_info_length;
            } //next record
    return p_pmt;
    }


    // ****************************************** STUFF
    TRawES *ParseSTUFF()
        {
        TRawES *pes  = m_RawTS.CInsertStuff();
        if (!pes) return nullptr;
        pes->sid      = 0;
        pes->pid      = 0x1fff;
        pes->stream_info.type = STUFF;
        pes->stream_info.type_stream  = STREAM_TYPE_UNKNOWN;
        pes->si_header_reset();
        return pes;
        }

/*
    TRawES *ParseCAT(uint8_t *data,unsigned data_length)
     {
        TRawES *pes  = m_RawTS.CInsertCommon(0x0001);
        if (!pes) return nullptr;
        pes->sid      = 0;
        pes->pid      = 0x01;
        pes->stream_info.type = CAT;
        pes->stream_info.type_stream  = STREAM_TYPE_UNKNOWN;
        pes->si_header_reset();
        return pes;
     }

    TRawES *ParseNIT(uint8_t *data,unsigned data_length)
     {
        TRawES *pes  = m_RawTS.CInsertCommon(0x0010);
        if (!pes) return nullptr;
        pes->sid      = 0;
        pes->pid      = 0x10;
        pes->stream_info.type = NIT;
        pes->stream_info.type_stream  = STREAM_TYPE_UNKNOWN;
        pes->si_header_reset();
        return pes;
     }
*/

    // ****************************************** OTHER
    TRawES *ParseCOMMON_Other(unsigned pid)
        {
        TRawES *pes = m_RawTS.CInsertCommon(pid);
        if (!pes) return nullptr;
        pes->sid      = 0;
        pes->pid      = pid;
        pes->stream_info.type = es_type(pid);
        pes->stream_info.type_stream  = STREAM_TYPE_UNKNOWN;
        pes->si_header_reset();
        return pes;
        }


    // ****************************************** PCR (gotPCR = es->count_pcr)
    void ParsePCR(const uint8_t *pkt,TRawES *pes)
        {
        // pkt = ptr to adaptation fieeld
        // present pcr ?
        if (!(pkt[1] & 0x10)) return;
        pes->Pcr.NewPcr(pkt);
        }

    // ****************************************** SDT
    TRawES *ParseSDT(uint8_t *data,unsigned data_length)
        {
        TRawES *p_sdt = m_RawTS.CInsertCommon(0x0011);
        if (!p_sdt) return nullptr;

        p_sdt->sid      = 0;
        p_sdt->pid      = 0x0011;
        p_sdt->stream_info.type = SDT;
        p_sdt->stream_info.type_stream  = STREAM_TYPE_UNKNOWN;

        uint8_t *p = ReceiveSection(0x42,data,data_length,m_common_sections[0x0011]);
       // if (m_common_sections[0x0011].start) return p_sdt;
        if(!p) return p_sdt;
        // p = ptr (SI + SECTION + CRC32)

        p_sdt->si_header_reset();

        uint8_t *ptr_end = p + m_common_sections[0x0011].data_size()-4; // no length crc32

        // uint8_t table_id = get8(p);                //
        // uint16_t section_length = get16(p); //
        p+=3;

        // tsID = get16(p); //transport_stream_id
        // version = get8(p); // version_number + current_next_indicator
        // section = get8(p); // section_number
        // last_section = get8(p); // last_section_number
        // networkId = get16(p); //original_network_id
        // reserved = get8(p); // reserved_future_use

        p +=8;

        // p = ptr to sdtN

        while(p  <  ptr_end) {

            uint8_t *psdt = p;

//Log.Scr("%s SDT: ",Info.IP().c_str()); PrintX(p,32);

            unsigned  service_id = (get16(p));

            p += 1;  // skip  eit flags

            unsigned desc_length = (get16(p) & 0x03ff);  //full size desc

            // +++
            // 5 = stream_type(8) + es_pid(16) + es_info_length(16) + data
            p_sdt->si_header_store(psdt, (5 +  desc_length) );
            // +++

//Log.Scr("%s SDT: service_id=%u desc_length=%u\n",Info.IP().c_str(),service_id,desc_length);

            if (desc_length) {

                TRawES *pes = GetSID(service_id);
                if (pes) {

                uint8_t *desc = p;
                //uint8_t *pmax = p + desc_length;

                desc += 1;  // skip descr tag
                desc += 1;  // skip descr length
                desc += 1;  // skip data identifier

                unsigned N;

                if (pes) {
                    N=*desc; Decode588595v2(sizeof(pes->provider),pes->provider,(desc+1),N); desc +=N; desc++;
                    N=*desc; Decode588595v2(sizeof(pes->service_name),pes->service_name,(desc+1),N); desc +=N; desc++;
                    }
                } // if pes

            p += desc_length;
            }

            if ((p+15) >= ptr_end) break;
            } //while
        return p_sdt;
        }


    TRawES *ParseTDT(uint8_t *data,unsigned data_length)
        {
        TRawES *p_tdt = m_RawTS.CInsertCommon(0x0014);
        if (p_tdt==nullptr) return nullptr;

        p_tdt->sid      = 0;
        p_tdt->pid      = 0x0014;
        p_tdt->stream_info.type = TDT;
        p_tdt->stream_info.type_stream  = STREAM_TYPE_UNKNOWN;
        p_tdt->si_header_reset();

        // table_id = 0x70 - TDT   0x73 - TOT
        uint8_t table_id = data[1];
        uint8_t *p = ReceiveSection(table_id,data,data_length,m_common_sections[0x0014]);
        //if (m_common_sections[0x0014].start) return p_tdt;
        if(!p) return p_tdt;
         // p = ptr (SI + SECTION + CRC32)

        // uint8_t table_id = get8(p);                //
        // uint16_t section_length = get16(p); //
        p+=3;

        char buf[MAX_ES_TEXT_LEN-2]; memset(buf,0,sizeof(buf));

        if (table_id == 0x70) {

            unsigned date = unsigned(p[0] << 8) | unsigned(p[1]);
            unsigned yp, mp, k;
            yp = unsigned((date - 15078.2)/365.25);
            mp = unsigned((date - 14956.1 - unsigned(yp * 365.25))/30.6001);
            unsigned day = unsigned(date - 14956 - unsigned(yp * 365.25) - unsigned(mp * 30.6001));
            if (mp == 14 || mp == 15) k = 1;
            else k = 0;
            unsigned year = yp + k + 1900;
            unsigned month = mp - 1 - k*12;

            unsigned hour   = 10*((p[2]&0xf0)>>4) + (p[2]&0x0f);
            unsigned minute = 10*((p[3]&0xf0)>>4) + (p[3]&0x0f);
            unsigned second = 10*((p[4]&0xf0)>>4) + (p[4]&0x0f);

            sprintf(buf,"TDT  %u-%u-%u %u:%u:%u",year,month,day,hour,minute,second);

            } else if (table_id == 0x73) {

            unsigned date = unsigned(p[0] << 8) | unsigned(p[1]);
            unsigned yp, mp, k;
            yp = unsigned((date - 15078.2)/365.25);
            mp = unsigned((date - 14956.1 - unsigned(yp * 365.25))/30.6001);
            unsigned day = unsigned(date - 14956 - unsigned(yp * 365.25) - unsigned(mp * 30.6001));
            if (mp == 14 || mp == 15) k = 1;
            else k = 0;
            unsigned year = yp + k + 1900;
            unsigned month = mp - 1 - k*12;

            unsigned hour   = 10*((p[2]&0xf0)>>4) + (p[2]&0x0f);
            unsigned minute = 10*((p[3]&0xf0)>>4) + (p[3]&0x0f);
            unsigned second = 10*((p[4]&0xf0)>>4) + (p[4]&0x0f);

            //uint32_t loop_len = ((pkt[5] & 0x0f) << 8) | (pkt[6] & 0xff);
            p +=2;

            p +=7;

            unsigned offset_hours = p[4]; // decimal ? ascii ? hex ?
            unsigned offset_minute = p[5]; // decimal ? ascii ? hex ?
            unsigned offset_seconds = 0;//(offset_hours *60 + offset_minute) * 60;
            //if (p[3] & 1) offset_seconds = -1;
            sprintf(buf,"TOT  %u-%u-%u %u:%u:%u  offset  %u:%u:%u",year,month,day,hour,minute,second,offset_hours,offset_minute,offset_seconds);
            }
        strcpy(p_tdt->service_name,buf);
        return p_tdt;
        }


void pes_h264_sps(TRawES *pes,uint8_t *p, unsigned length);
void pes_h265_sps(TRawES *pes,uint8_t *p, unsigned length);


TRawES *ParsePES(unsigned pid,uint8_t *pkt,unsigned pkt_length);

public:

    void  Initialize(TInfo  *p,unsigned second_alive);


    int  ReceiveStart();
    void ReceiveStop();


    TPacket packet;
    TInfo   Info;

    TAnalizerTS  *m_pAnalizer=nullptr;     // for callback
    unsigned ts_summary_bytes=0;

    // ============================================================

    TIPAddress     ipSender;

    bool     measurer=false;
    uint16_t measure_pid;

    bool IsMeasurer() { return measurer; }

    // ****************************************** CALLBACK (1 second timer)

    //unsigned tic_counter=0;

    void Callback_Timer()
        {
        if (measurer)
            Measure.Timer();

        m_RawTS.TimeoutedDelete();
        m_pAnalizer->CopyFromChannelToRawAnalize(m_RawTS,ts_summary_bytes);

        m_RawTS.Clear();
        ts_summary_bytes=0;
        }

    // ****************************************** CALLBACK (UDP pkt received)

    void Callback_Parser(uint8_t *rcvData,unsigned recvLen,uint32_t ip_src);

};

extern vector <TChannel *> pChannelsArray;

extern TChannel  *FindChannel(TIPAddress ip_if,TIPAddress ip,TIPPort port);
extern TChannel  *FindChannel(const string &sect,TIPAddress ip,TIPPort port);

