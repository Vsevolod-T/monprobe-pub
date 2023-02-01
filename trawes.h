#pragma once


#include "theader.h"





class TPcr
{

  unsigned found_pcr=0;

  uint64_t new_pcr=0;
  uint64_t prev_pcr=0;

  unsigned count_pkt_pcr_pid=0;

public:

unsigned    count_pcr=0;      //число пакетов witch pcr
double        bitRate = 0.0;

public:

   TPcr() {}
    ~TPcr() {}


   void Clear1sec() {
       count_pcr=0;
   }

   void ClearAll() {
       found_pcr=0;
       count_pcr=0;
   }

    void PacketWitchPcrPid() {
        count_pkt_pcr_pid++;
    }

   bool Found() {
       if (found_pcr>1) return true;
       return false;
       }


   void NewPcr(const uint8_t *pkt);
};



class TRawTS;


class TRawES
{
public:
        //  ========= POOL
        unsigned    pool_state=0;
        unsigned    pool_timeout=0;
        unsigned    pool_idx_pmt=0; // used
        unsigned    pool_idx_es=0;  // not used

        unsigned    pid=0;

        //  ========= POOL

        TPcr           Pcr;

        unsigned    sid=0;            // SID or 0
        unsigned    tsid=0;           // TSID or 0


        unsigned   type_from_pmt=0;
        unsigned   type_from_pes=0;

        //es_type     rcv_type=UNDEF;      //received in PMT
        //es_stream   rcv_tag=STREAM_TYPE_UNDEF;        //received in PMT

        unsigned    next_cc=0;       //(inner usage) elapsed cc counter

     // per second statistic
        unsigned    bytes=0;          //bytes принятое за секунду
        unsigned    scrambled=0;   //число скремблированных пакетов
        unsigned    cc_errors=0;     //число сбоев cc

        char        provider[MAX_ES_TEXT_LEN]{};     // if pmt => provider (inner)
        char        service_name[MAX_ES_TEXT_LEN]{}; // if pmt => service name  (inner) , if es => description

public:

        _stream_info stream_info {};

private:

        unsigned    pes_header_length=0;           // length data in pes_data
        uint8_t     pes_header[MAX_PES_SIZE]{};    // used in parse pes ONLY -> pes header[] VIDEO, AUDIO ...
        unsigned    pes_offset=0;   // clear ?

        unsigned    si_header_length=0;                        // filled in PMT .. SDT  - section ES ,
        uint8_t     si_header[MAX_PES_SIZE]{};    // used in parse pmt & sdt

public:

        void pes_header_reset() { pes_header_length=0; pes_offset=0; }
        void pes_header_store(uint8_t *src,unsigned len) {
            if (len > sizeof(pes_header)) len=sizeof(pes_header);
            pes_header_length=len;
            memcpy(pes_header,src,len);
            }
        unsigned pes_header_len() { return pes_header_length; }
        uint8_t *pes_header_ptr() { return &pes_header[0]; }

        unsigned pes_offset_stored() { return pes_offset; }
        void pes_offset_store(unsigned offs) { pes_offset=offs; }

        void si_header_reset() { si_header_length=0; }
        void si_header_store(uint8_t *src,unsigned len) {
            if (len > sizeof(si_header)) len=sizeof(si_header);
            si_header_length=len;
            memcpy(si_header,src,len);
            }
        unsigned si_header_len() { return si_header_length; }
        uint8_t *si_header_ptr() { return &si_header[0]; }

        void Clear() {

            //pes_offset =

              bytes     =
              scrambled =
              cc_errors = 0;
              Pcr.Clear1sec();
              }

}__attribute__ ((aligned (16)));

extern void pmt_set_stream_info(TRawES *p,uint8_t type);
extern void pmt_parse_pes_descriptor(TRawES *pes,uint8_t * p, size_t len);
