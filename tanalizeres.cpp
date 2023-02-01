#include "tanalizeres.h"

#include "tdecode.h"





unsigned TAnalizerES::GetVideoLimit()
{
    unsigned kbit=0;
    if (IsPCR())
        kbit = unsigned(PcrBitRate() *1000000/8.0);
    else
        kbit = unsigned((es.bitrate_min+es.bitrate_max)*100000/2);

    return kbit;
}

unsigned TAnalizerES::GetAudioLimit()
{
    unsigned kbit=0;
    if (IsPCR())
        kbit = unsigned(PcrBitRate() *1000000/8.0);
    else
        kbit = unsigned(es.stream_info.BitRate)/8;

    return kbit;
}


void TAnalizerES::ComputeEsMinimalTraffic()
{
    es.min_traffic_limit=0;
    if (es.stream_info.type == VIDEO) { es.min_traffic_limit = GetVideoLimit() / 4; }
   // if (es.stream_info.type == AUDIO) { es.min_traffic_limit = GetAudioLimit() / 4; }
    if (es.stream_info.type_stream ==  STREAM_TYPE_AUDIO_MPEG1) { es.min_traffic_limit = GetAudioLimit() / 4; }
    if (es.stream_info.type_stream ==  STREAM_TYPE_AUDIO_MPEG2) { es.min_traffic_limit = GetAudioLimit() / 4; }
}





void TAnalizerES::SetTimeouts(const _es_timeouts& val)
{
    es.timeouts.time_to_off = (val.time_to_off == 0) ?  1 : val.time_to_off;
    es.timeouts.time_to_on  = (val.time_to_on  == 0) ?  1 : val.time_to_on;
}


void TAnalizerES::InnerComputeStateUITraffic(bool traffic_ok)
{
    switch(es.es_ui_current_state) {

    //case UI_OUT_LOCKED:
    //case UI_QUALITY:

        case UI_STREAM_OK:
            if (traffic_ok) {
                es.es_updated                 = false;
                return;
                }
        else { //STREAM_NO
                es.es_ui_current_state     = UI_STREAM_LOST;
                es.es_ui_current_count     = es.timeouts.time_to_off;
                es.es_updated              = true;
                return;
                }
        case UI_STREAM_LOST:
            if (traffic_ok) {
                es.es_ui_current_state    = UI_STREAM_RESTORE;
                es.es_ui_current_count    = es.timeouts.time_to_on;
                es.es_updated             = true;
                return;
                }
            else { //STREAM_NO
                if (es.es_ui_current_count) {
                    --es.es_ui_current_count;
                     es.es_updated       = false;
                    return;
                   }
                es.es_ui_current_state   = UI_STREAM_NO;
                es.es_updated            = true;
                es.es_ok                 = false;
                return;
                }

        case UI_STREAM_RESTORE:
            if (traffic_ok) {
                if (es.es_ui_current_count) {
                    --es.es_ui_current_count;
                    es.es_updated        = false;
                    return;
                   }
                es. es_ui_current_state  = UI_STREAM_OK;
                es.es_updated            = true;
                es.es_ok                 = true;
                return;
                }
            else { //STREAM_NO
                es.es_ui_current_state   = UI_STREAM_LOST;
                es.es_ui_current_count   = es.timeouts.time_to_off;
                es.es_updated            = true;
                return;
                }

        case UI_STREAM_NO:
            if (traffic_ok) {
                es.es_ui_current_state   = UI_STREAM_RESTORE;
                es.es_ui_current_count   = es.timeouts.time_to_on;
                es.es_updated            = true;
                return;
                }
            else { //STREAM_NO
                es.es_updated            = false;
                return;
                }

    default:

        Log.Scr("BAD ComputeStateUI=%u\n",es.es_ui_current_state);

        break;

        /*
        case UI_UNCHECKED:
                state.ui_current_state      = UI_STREAM_OK;
                state.ui_updated            = true;
                state.ok                    = true;
                return;

        case UI_OUT_LOCKED:
        case UI_QUALITY:
                */
        }; //switch
}





void TAnalizerES::ComputeEsState()
{
    statistic_es.reset();

    es.es_updated=false;

    bool traffic_ok = ((es.bytes < es.min_traffic_limit) ? false : true);

    if (!traffic_ok)
        statistic_es.second_bad_traffic++;

    // set es_updated ... ets ..
    InnerComputeStateUITraffic(traffic_ok);





     if (es.stream_info.type == AUDIO && g.ignore_audio_cc) {
         // skip error
         ;
     }
     else {

         if (es.cc_errors) {
             statistic_es.summary_ccerrors += es.cc_errors;
             statistic_es.second_ccerror++;
             es.es_updated=true;
            }
     }


}



void TAnalizerES::CopyRawToAnalizer(TRawES *p)
{
    if (!p) return;
    pool_state    = p->pool_state;

    pid           = p->pid;

    es.bytes     = p->bytes;      //bytes принятое за секунду
    es.scrambled = p->scrambled;  //число скремблированных пакетов
    es.cc_errors = p->cc_errors;  //число сбоев cc

    es.stream_info = p->stream_info;

   // rcv.pes_header_length = 0; // ???

    if (p->pes_header_len()) {

//Log.Scr("pid=%u pes_header_len=%u\n",pid,p->pes_header_len());

        es.pes_header_length = p->pes_header_len();
        memcpy(&es.pes_header[0],p->pes_header_ptr(),p->pes_header_len());
        }

    if (p->si_header_len()) {
        es.si_header_length = p->si_header_len();
        memcpy(&es.si_header[0],p->si_header_ptr(),p->si_header_len());
        }

    // bitrate
        {
        es.bitrate_current         = ((p->bytes) * 8.0 ) / 1000000.0;

       //const double coeff_flatness = 1.0;
       //bitrate.average = (coeff_flatness * bitrate.average + bitrate.current ) / (coeff_flatness + 1.0);

        if (es.bitrate_current > es.bitrate_max)
            es.bitrate_max = es.bitrate_current;

        if ( (es.bitrate_current > (es.bitrate_max / 2.0)) && (es.bitrate_current < es.bitrate_min) )
            es.bitrate_min = es.bitrate_current;
        }


    if (es.stream_info.type == PMT) {

        pmt.sid          = p->sid; // SID  if pmt
        pmt.tsid         = p->pid; // TSID if pmt
        pmt.provider     = string(p->provider);       //if pmt => provider (inner)
        pmt.service_name = string(p->service_name);   //if pmt => service name  (inner)

        // PMT IS OK
        es.es_updated      = false;
        es.es_ok                  = true;
        es.es_ui_current_state   = UI_STREAM_OK;
        //checked_value.clear();
        }
    else {
        pmt.service_name = string(p->service_name); // if other es => service name  (inner)
        }


    es.pcr.found     = p->Pcr.Found();
    if (es.pcr.found) {

        const double coeff_flatness = 1.0;
        es.pcr.average_per_sec = (coeff_flatness * es.pcr.average_per_sec + double(p->Pcr.count_pcr)) / (coeff_flatness + 1.0);

        es.pcr.bitRate         = p->Pcr.bitRate    / 1000000.0;

        if (es.pcr.bitRate > es.pcr.maxBitRate)
            es.pcr.maxBitRate = es.pcr.bitRate;

        if ( (es.pcr.bitRate > (es.pcr.maxBitRate / 2.0)) && (es.pcr.bitRate < es.pcr.minBitRate) )
            es.pcr.minBitRate = es.pcr.bitRate;
        }


    // DECODE
    if (pid < 0x0020 || pid==0x1fff) {

        // always OK
        es.es_updated           = false;
        es.es_ok                  = true;
        es.es_ui_current_state   = UI_STREAM_OK;

        es.min_traffic_limit =0;
        return;
        }


    ParsePesHeader(this);

}








