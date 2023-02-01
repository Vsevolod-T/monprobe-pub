#pragma once

#include "theader.h"
#include "tglobal.h"

#include "trawes.h"


class TAnalizerTS;



class TAnalizerES
{
    friend class TAnalizerTS;
    friend class TBuffer;

    friend class ES_MPEG2Audio;
    friend class ES_MPEG2Video;
    friend class ES_AC3;
    friend class ES_AAC;
    friend class ES_Teletext;
    friend class ES_Subtitle;
    friend class ES_h264;
    friend class ES_hevc;

    friend void  ParsePesHeader(TAnalizerES *pes);
    friend const string GetTypeText(TAnalizerES *pes);
    friend const string GetStreamText(TAnalizerES *pes);

    friend void PRINT_ANALIZED_TS(TAnalizerTS *p,const string& hdr);

    // ========= POOL FILLED
    unsigned  pool_state;
    unsigned  pid;

    _statistic     statistic_es;

    // current es values per 1 sec
      struct _es {
          unsigned bytes;      // bytes принятое за секунду
          unsigned scrambled;  // число скремблированных пакетов
          unsigned cc_errors;  // число сбоев cc

           // filled in data es
          unsigned   pes_header_length=0;         // length data in pes_data
          uint8_t    pes_header[MAX_PES_SIZE]{};  // used in parse pes ONLY -> pes header[] VIDEO, AUDIO ...

          unsigned   si_header_length=0;          // length data in pes_data
          uint8_t    si_header[MAX_PES_SIZE]{};   // used in parse pes ONLY -> pes header[] VIDEO, AUDIO ...

          _stream_info stream_info{};

          struct _pcr {
              bool     found       = false;
              double   average_per_sec = 0.0;
              double   bitRate     = 0.0;          //bitrate определенное через pcr (среднее за 1 сек)
              double   maxBitRate  = 0.0;          //pcr
              double   minBitRate  = 100000000.0;  //pcr

              void reset() {
                     maxBitRate     = 0.0;
                     minBitRate     = 100000000.0;  //
                     }
              }pcr;

              double   bitrate_current = 0.0;          // // CopyRawToAnalizer
              double   bitrate_max     = 0.0;          //
              double   bitrate_min     = 100000000.0;  //

              void bitrate_reset() {
                     bitrate_max    = 0.0;
                     bitrate_min     = 100000000.0;  //
                     }


        unsigned min_traffic_limit=0;

        _es_timeouts     timeouts;
        unsigned    es_ui_current_count = 0;
        es_ui_state es_ui_current_state = UI_STREAM_NO;

        // -------------------- ES Analize Rezult
        bool        es_updated          = false; // es
        bool        es_ok               = false; // es
        }es;


    // all es from pmt per 1 sec
    struct  _pmt {

        unsigned sid;                   // * if pmt
        unsigned tsid;                  // * if pmt
        string   provider;              // * if pmt
        string   service_name;          // * if pmt
        string   channel_name_ui;       // channel * if pmt


        es_ui_state  ui_state = UI_STREAM_NO; // sum all es
        bool           pmt_updated          = false;        //

        _statistic     statistic_second;         //

        }pmt;

public:


    //  es set minimal traffic        SetAutoMinimalTrafficEs();
     void ComputeEsMinimalTraffic();
    // compute es state (ui ..  )     pes->ComputeEsState()
     void  ComputeEsState();


    void InnerComputeStateUITraffic(bool traffic_ok);

public:

    unsigned    Pid()               { return pid;           }

    unsigned    RcvBytes()          { return es.bytes;     } //число принятых байтов
    unsigned    RcvScrambled() { return es.scrambled; } //число скремблированных пакетов
    unsigned    RcvErrors()         { return es.cc_errors; } //число сбоев cc

    unsigned RcvErrors_ForTS() {
        if (es.stream_info.type == AUDIO && g.ignore_audio_cc)
            return 0;
        return es.cc_errors;
        }

     unsigned    MinTrafficLimit()         { return es.min_traffic_limit; }


    bool        StateOk()           { return es.es_ok;        }
    bool        StateUpdated()      { return es.es_updated;   }
    es_ui_state    StateUI()        {
         if (es.stream_info.type == PMT) {
             return pmt.ui_state;
         }
        return es.es_ui_current_state;

    }

    void StateReset() {
            es.es_updated      = false;
            es.es_ok               = true;
            es.es_ui_current_state = UI_STREAM_OK;
    }




    es_stream   PesDecodedType()    { return es.stream_info.type_stream;  }
    es_type     PesTypeClass()      { return es.stream_info.type; }

     // ========== PCR  =========
    bool        IsPCR()             { return es.pcr.found;         }
    double      PcrPerSecond()      { return es.pcr.average_per_sec;   }
    double      PcrBitRate()        { return es.pcr.bitRate;       }
    double      PcrBitRateMax()     { return es.pcr.maxBitRate;    }
    double      PcrBitRateMin()     { return es.pcr.minBitRate;    }

    // ========== CHANNEL  ( valid if PMT ) =========
    bool        IsPMT()                 { return  (es.stream_info.type==PMT); }

    unsigned    PmtSid()                { return pmt.sid;          }  // PRINT_ANALIZED_TS + UI Detail
    unsigned    PmtTSid()               { return pmt.tsid;         }  //only PRINT_ANALIZED_TS

    const string& PmtProvider()         { if (pmt.provider.empty())  return STUB_STRING; else return pmt.provider;     }
    const string& PmtServiceName()      { if (pmt.service_name.empty())  return STUB_STRING; else return pmt.service_name; }

    const string& PmtNameUI()           {  if (pmt.channel_name_ui.empty())  return STUB_STRING; else  return pmt.channel_name_ui; }
    void        PmtNameUISet(const string& val)  { pmt.channel_name_ui = val;  }


   // ========== CHANNEL  CHECKED  =========

    void        SetTimeouts(const _es_timeouts& val);

    unsigned GetVideoLimit();
    unsigned GetAudioLimit();


public:

    void        CopyRawToAnalizer(TRawES *p);


    void SetDefault() {

        pool_state  = POOL_ES_UNUSED;

        pid            = 0;
        statistic_es.reset();

        es.bitrate_reset();
        es.pcr.reset();

        StateReset();
        }

    void StartReset() {
        es.bitrate_reset();
        es.pcr.reset();
        StateReset();
        }

    //void StatisticEs_Reset()  { statistic_es.reset(); }
    _statistic *StatisticEs() { return &statistic_es; }


    TAnalizerES() {  SetDefault(); }


    // ----------------------------------------


};



