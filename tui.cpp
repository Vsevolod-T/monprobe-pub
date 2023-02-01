#include "tui.h"
#include "tlog.h"
#include "tinterface.h"
#include "tdecode.h"   // DEB
#include "tanalizeres.h"
#include "tanalizerts.h"
#include "twsconnection.h"
#include "tchannelmeasure.h"
#include "tchannel.h"

#include "tstatistic.h"

#include "tdecode.h"
#include "ttime.h"

TUI             UI;

//
//  Fill pmt_info
//
void TUI::PmtInfo_Fill()
{
     unsigned ts_index = 0;
     for (auto pAnalizer : pAnalizersArray) {

        size_t ts_count_pmt = pAnalizer->CountPmt();

        if (!ts_count_pmt) {
            _pmt_info tmp{};
            tmp.ptr_ts       = pAnalizer;
            tmp.ptr_pmt      = nullptr; //pAnalizer->GetStub();
            tmp.ts_index     = ts_index;
            tmp.ts_count_pmt = 1;
            tmp.pmt_index    = 0;
            tmp.pmt_state    = UI_STREAM_NO;
            tmp.pmt_updated  = 0;

            table_pmt_info.emplace_back(tmp);
            ++ts_index;
            continue;
            }

        for (size_t pmt_index=0;pmt_index < ts_count_pmt; ++pmt_index) {
            TAnalizerES *pPmt = pAnalizer->GetPmt(pmt_index);
            if (!pPmt) continue;
            _pmt_info tmp{};
            tmp.ptr_ts       = pAnalizer;
            tmp.ptr_pmt      = pPmt;
            tmp.ts_index     = ts_index;
            tmp.ts_count_pmt = ts_count_pmt;
            tmp.pmt_index    = pmt_index;
            tmp.pmt_state    = pPmt->StateUI();
            tmp.pmt_updated  = 0;

            table_pmt_info.emplace_back(tmp);
            }  // next pmt

        ++ts_index;
        } // next TS (AnalizersArray)
}

bool TUI::IsUpdatedPmtInfoStructure(_pmt_info *stored,_pmt_info *readed)
{
    return ( (stored->ptr_ts       != readed->ptr_ts) ||
         (stored->ptr_pmt      != readed->ptr_pmt) ||
        (stored->ts_index     != readed->ts_index) ||
        (stored->ts_count_pmt != readed->ts_count_pmt) ||
        (stored->pmt_index    != readed->pmt_index) );
}

bool TUI::IsUpdatedPmtInfoStatus(_pmt_info *stored,_pmt_info *readed)
{
    return (stored->pmt_state != readed->pmt_state);
}

//
//  Check pmt_info
//  set updated_structure  if needed
//  set updated_state if needed
//
void TUI::PmtInfo_Check()
{
    // info_table  full filled
    // checking update

    unsigned idx = 0;
    unsigned ts_index = 0;

     for (auto pAnalizer : pAnalizersArray) {

        size_t ts_count_pmt = pAnalizer->CountPmt();
        if (!ts_count_pmt) {
            _pmt_info tmp{};
            tmp.ptr_ts       = pAnalizer;
            tmp.ptr_pmt      = nullptr;//pAnalizer->GetStub();
            tmp.ts_index     = ts_index;
            tmp.ts_count_pmt = 1;
            tmp.pmt_index    = 0;
            tmp.pmt_state    = UI_STREAM_NO;
            tmp.pmt_updated  = 0;

            if (IsUpdatedPmtInfoStructure(&table_pmt_info[idx],&tmp)) {
                updated_structure = true;
                table_pmt_info[idx] = tmp;
                }
            else {
                if (IsUpdatedPmtInfoStatus(&table_pmt_info[idx],&tmp)) {
                    updated_state = true;
                    table_pmt_info[idx].pmt_state = tmp.pmt_state;
                    table_pmt_info[idx].pmt_updated = true;
                    }
                }

            //info_table.emplace_back(tmp);
            ++idx;
            ++ts_index;
            continue;
            }

        for (size_t pmt_index=0;pmt_index < ts_count_pmt; ++pmt_index) {
            TAnalizerES *pPmt = pAnalizer->GetPmt(pmt_index);
            if (!pPmt) continue;
            _pmt_info tmp{};
            tmp.ptr_ts       = pAnalizer;
            tmp.ptr_pmt      = pPmt;
            tmp.ts_index     = ts_index;
            tmp.ts_count_pmt = ts_count_pmt;
            tmp.pmt_index    = pmt_index;
            tmp.pmt_state    = pPmt->StateUI();
            tmp.pmt_updated  = 0;

            if (IsUpdatedPmtInfoStructure(&table_pmt_info[idx],&tmp)) {
                updated_structure = true;
                table_pmt_info[idx] = tmp;
                }
            else {
                if (IsUpdatedPmtInfoStatus(&table_pmt_info[idx],&tmp)) {
                    updated_state = true;
                    table_pmt_info[idx].pmt_state = tmp.pmt_state;
                    table_pmt_info[idx].pmt_updated = true;
                    }
                }
            ++idx;
            //info_table.emplace_back(tmp);
            }  // next pmt
        ++ts_index;
        } // for (auto & pAnalizer : pAnalizersArray)
}


void TUI::PmtInfo_Prepare()
{
    updated_structure = false;
    updated_state       = false;

    size_t summary_count_pmt = 0;
    for (auto pAnalizer : pAnalizersArray) {
        size_t ts_count_pmt = pAnalizer->CountPmt();
        if (ts_count_pmt) summary_count_pmt += ts_count_pmt;
        else ++summary_count_pmt;
        }

    // clear & new fill info_table
    if (summary_count_pmt != table_pmt_info.size()) {
//Log.ScrFile("TUI::PmtInfo_Prepare change size from %lu  to %lu\n",table_pmt_info.size(),summary_count_pmt);
        table_pmt_info.clear();
        PmtInfo_Fill();
        updated_structure = true;
//Log.ScrFile("TUI::PmtInfo_Prepare filled %lu\n",table_pmt_info.size());
        }
    else
        PmtInfo_Check();
}


void TUI::Create_UI_ServerInfo()
{
    ui_serverinfo.clear();
      ui_serverinfo += R"({"cmd":"serverinfo","server_version":)" + json_string(SERVER_VERSION);
      ui_serverinfo += R"(,"server_date":)" + json_string(Time.getCurrentDate());
      ui_serverinfo += R"(,"server_time":)" + json_string(Time.getCurrentTime());

      // area
      // place
      ui_serverinfo += R"(,"area":)"  + json_string(g.area.c_str());
      ui_serverinfo += R"(,"place":)" + json_string(g.place.c_str());

      ui_serverinfo += R"(,"server_count":)" + to_string(ui_counter);  //ui_serverinfo += R"(,"server_usec":)" + json_string(g.getCurrentUsec());
      ui_serverinfo += R"(,"streamer":"0.0.0.0:0000")";// + json_string("0.0.0.0:0000");
      ui_serverinfo += R"(,"count_interfaces":)" + to_string(pInterfacesArray.size());
      ui_serverinfo += R"(,"count_channels":)"   + to_string(table_pmt_info.size());
      ui_serverinfo += R"(,"arr_if":[)";

     unsigned next = 0;
     for (auto &  pInterface : pInterfacesArray) {
         if (next) { ui_serverinfo  += ","; } ++next;

         ui_serverinfo += R"({"name":)" + json_string(pInterface->name_sys);
         ui_serverinfo += R"(,"ip":)" + json_string(pInterface->ip_if.c_str());
         ui_serverinfo += R"(,"traffic":)" + json_format02f(pInterface->Traffic());
         //ui_serverinfo += R"(,"start":)" + json_string(pInterface->ip_index);
         //ui_serverinfo += R"(,"count":)" + json_string(pInterface->ip_count_if) ;
          ui_serverinfo +=   "}";
         }

    ui_serverinfo += R"(],"arr_ch":[)";
    next = 0;
     for (auto &  PmtInfo : table_pmt_info) {
        TAnalizerTS *ptr_ts   = PmtInfo.ptr_ts;
        TAnalizerES *ptr_pmt  = PmtInfo.ptr_pmt;   // maybe nullptr
        unsigned ts_count_pmt = PmtInfo.ts_count_pmt;
        unsigned pmt_index    = PmtInfo.pmt_index;
        unsigned pmt_state    = PmtInfo.pmt_state;

        if (next) { ui_serverinfo += ","; } ++next;

        if (!ptr_pmt)
            ui_serverinfo += R"({"name":)" + json_string(ptr_ts->Info.Name());
        else
            ui_serverinfo += R"({"name":)" + json_string(ptr_pmt->PmtNameUI());

        ui_serverinfo += R"(,"ip":)" + json_string(string(ptr_ts->Info.IP().c_str()) + ":" + ptr_ts->Info.Port().c_str());
        ui_serverinfo += R"(,"total":)" + to_string(ts_count_pmt);
        ui_serverinfo += R"(,"num":)" + to_string(pmt_index);
        ui_serverinfo += R"(,"state":)" + to_string(pmt_state) + "}";
        }
    ui_serverinfo +="]}";

//Log.Scr("ui_serverinfo >> %s\n",ui_serverinfo.c_str());
}

void TUI::Create_UI_StatusInfo()
{
    ui_statusinfo.clear();
    ui_statusinfo += R"({"cmd":"state")";
    ui_statusinfo += R"(,"server_date":)" + json_string(Time.getCurrentDate());
    ui_statusinfo += R"(,"server_time":)" + json_string(Time.getCurrentTime());
    ui_statusinfo += R"(,"server_count":)" + to_string(ui_counter); //ui_statusinfo += R"(,"server_usec":)" + json_string(g.getCurrentUsec());
    ui_statusinfo += R"(,"arr_if":[)";

    unsigned next = 0;
    for (auto &  pInterface : pInterfacesArray) {
        if (next) { ui_statusinfo  += ","; } ++next;

        ui_statusinfo += R"({"traffic":)" + json_format02f(pInterface->Traffic()) + "}";
        }

    ui_statusinfo += "]";
    if (!updated_state) { ui_statusinfo += "}"; return; }

    ui_statusinfo += R"(,"arr_stat":[)";
    unsigned line_index=0;
    next = 0;
    for (auto & pmtInfo : table_pmt_info) {
        if (pmtInfo.pmt_updated) {
            if (next) { ui_statusinfo  += ","; } ++next;

             ui_statusinfo += "[" + to_string(line_index);
             ui_statusinfo += "," + to_string(pmtInfo.pmt_state) + "]";

             pmtInfo.pmt_updated = false;
             }
        ++line_index;
       }//for pmt
    ui_statusinfo += "]}";
}


void TUI::Create_UI_Statistic()
{
    ui_statisticinfo.clear();
    ui_statisticinfo = R"({"cmd":"stat")";
    ui_statisticinfo += R"(,"server_date":)" + json_string(Time.getStartMeasureDate());
    ui_statisticinfo += R"(,"server_time":)" + json_string(Time.getStartMeasureTime());
    ui_statisticinfo += R"(,"server_count":)"   + to_string(ui_counter);
    ui_statisticinfo += R"(,"period":)"      + to_string(Time.GetMeasureSecond());

    if (Measure.IsValid()) {
        string ip = string(Measure.Info.IP().c_str()) + ":" + Measure.Info.Port().c_str();
        ui_statisticinfo += R"(,"measure":{)";
        ui_statisticinfo += R"("ip":)" + json_string(ip);
        ui_statisticinfo += R"(,"pkt_summ":)"    + Measure.Packets_Summary();
        ui_statisticinfo += R"(,"pkt_ok":)"      + Measure.Packets_ok();
        ui_statisticinfo += R"(,"pkt_drop":)"    + Measure.Packets_drop();
        ui_statisticinfo += R"(,"pkt_reorder":)" + Measure.Packets_reordered();
        ui_statisticinfo += R"(,"err_percent":)" + Measure.ErrPercent();
        ui_statisticinfo += "}";
        }

    ui_statisticinfo += R"(,"arr_stat":[)";
    unsigned line_index=0;
    unsigned next = 0;

    for (auto & pAnalizer : pAnalizersArray)
    {
        size_t count_pmt = pAnalizer->CountPmt(); //count pmt in ip

         // IF NO PMT
         if (count_pmt==0) {
             if (next) { ui_statisticinfo  += ","; } ++next;
             ui_statisticinfo += "[" + to_string(line_index);          // index
             ui_statisticinfo += "," + to_string(pAnalizer->ts_statistic_period_final.second_bad_traffic);      // ch_no_sec
             ui_statisticinfo += "," + to_string(pAnalizer->ts_statistic_period_final.second_ccerror);          // ch_err_sec
             ui_statisticinfo += "," + to_string(pAnalizer->ts_statistic_period_final.summary_ccerrors) + "]";  // cc errors
            ++line_index;
             continue;
             }

        // if count pmt > 0
        for (size_t j=0; j<count_pmt; ++j) {
            TAnalizerES *ppmt = pAnalizer->GetPmt(j);
            if (!ppmt) continue;

            if (next) { ui_statisticinfo  += ","; } ++next;
            ui_statisticinfo += "[" + to_string(line_index);          // index
            ui_statisticinfo += "," + to_string(pAnalizer->ts_statistic_period_final.second_bad_traffic);      // ch_no_sec
            ui_statisticinfo += "," + to_string(pAnalizer->ts_statistic_period_final.second_ccerror);          // ch_err_sec
            ui_statisticinfo += "," + to_string(pAnalizer->ts_statistic_period_final.summary_ccerrors) + "]";  // cc errors
           ++line_index;
            } // next pmt
        } // next ip

     ui_statisticinfo += "]}";
}



void  TUI::json_es(TAnalizerES *pes,string &dst)
{
 //R"("es":[82,3,0,0])"
    dst += R"({"es":[)";
    dst += to_string(pes->Pid())          + ",";
    dst += to_string(pes->StateUI())      + ",";
    dst += to_string(pes->RcvErrors())    + ",";
    dst += to_string(pes->RcvScrambled()) + "]";
}

void  TUI::json_type(TAnalizerES *pes,string &dst)
{
 //R"("type":["PAT","0.15","","stuff",2.44])"
    string text_traffic = format_traffic(pes->RcvBytes());

    dst += R"("type":[)";
    dst += json_string(GetTypeText(pes)) + ",";  // TYPE
    dst += json_string(text_traffic)  + ",";
    dst += R"("")"s + ",";
    dst += json_string(GetStreamText(pes)) + ",";
    dst += format_02f((pes->RcvBytes() * 8) / 1000.0)  + "]";
}

void  TUI::json_checked(TAnalizerES *pes,string &dst)
{
    if (!pes) {
    dst += R"("checked":{"scramble":0,"traffic_min":0,"traffic_max":0,"ccerror_max":0}})";
    return;
    }

    dst += R"("checked":{)";
    dst += R"("scramble":0,)";//     + json_string(chk.scramble_check) + ",";
    dst += R"("traffic_min":)"  + to_string((pes->MinTrafficLimit()*8)/1000) + ",";
    dst += R"("traffic_max":0,)";//  + to_string(chk.traffic_max) + ",";
    dst += R"("ccerror_max":0)";//  + to_string(0);
    dst += "}";
}


void  TUI::Create_UI_Detail(unsigned line_index,string &dst)
{
    // BAD index
    if ( line_index >= table_pmt_info.size()) {
        dst += R"(,"detail":{"traffic":0.0,"video":0.0,"audio":0.0,"channel":0,"num":0,"ip":"0.0.0.0:0","ini_ts":"?","ini_ch":"?","state":0,"ts_prov":"?","ts_ch":"?"})";
        return;
        }

    TAnalizerTS *pan   = table_pmt_info[line_index].ptr_ts;
    if (!pan) return;

    TAnalizerES *pmt   = table_pmt_info[line_index].ptr_pmt;
    unsigned idx_pmt   = table_pmt_info[line_index].pmt_index;
    unsigned ts_index  = table_pmt_info[line_index].ts_index;

    TChannel *pch = pChannelsArray[ts_index];
    bool measurer = pch->IsMeasurer();



// for 1 second
 unsigned  summary_bytes = pan->GetRawRecievedBytes(); // pan->StatisticTS_1sec_UI_Bytes();

 // current period (pan ? pmt)
 unsigned summary_cc = pan->ts_statistic_period_accumulate.summary_ccerrors;

string text_traffic = format_traffic(summary_bytes);
double traffic = double(summary_bytes * 8) / 1000.0;

if (!pmt) {
    // no pmt - generate fake
    dst += R"(,"detail":{"traffic":)" + json_format02f((summary_bytes  *  8) / 1000.0);
    dst += R"(,"channel":)" + to_string(ts_index);
    dst += R"(,"num":)" + to_string(idx_pmt);
    dst += R"(,"ip":)" + json_string(string(pan->Info.IP().c_str()) + ":" + pan->Info.Port().c_str());
    dst += R"(,"ini_ts":)" + json_string(pan->Info.Name());
    dst += R"(,"ini_ch":"")";
    dst += R"(,"state":)" + to_string(UI_STREAM_NO);
    dst += R"(,"ts_prov":"")";
    dst += R"(,"ts_ch":"")";

    dst += R"(,"root":[{"es":[0,)";
    dst += to_string(UI_STREAM_NO) + ",";
    dst += to_string(summary_cc);
    dst += R"(,0],"type":["IPTS:",)" + json_string(text_traffic);
    dst += R"(,"","Summary: Traffic , CCErrors",)" + to_string(traffic);
    dst += R"(],"checked":{"scramble":0,"traffic_min":0,"traffic_max":0,"ccerror_max":0}}])";

    dst += R"(}})";
    return;
    }

    // Generate detail
    dst += R"(,"detail":{"traffic":)" + json_format02f((summary_bytes  *  8) / 1000.0);
    dst += R"(,"channel":)" + to_string(ts_index);
    dst += R"(,"num":)" + to_string(idx_pmt);
    dst += R"(,"ip":)" + json_string(string(pan->Info.IP().c_str()) + ":" + pan->Info.Port().c_str());
    dst += R"(,"ini_ts":)" + json_string(pan->Info.Name());
    dst += R"(,"ini_ch":)" + json_string(pmt->PmtNameUI());
    dst += R"(,"state":)" + to_string(pmt->StateUI());
    dst += R"(,"ts_prov":)" + json_string(pmt->PmtProvider());
    dst += R"(,"ts_ch":)" + json_string(pmt->PmtServiceName());

    // ************************* ROOT
    int rcount=0;

    // COMMON
    {
    TAnalizerES *pes=nullptr;

    // =============== First - IPTS ==================
    //{ "es":[0,       3,0,0],    "type":["IPTS:", "8.00Mbit",     "","Summary: Traffic , CCErrors",8000.00]  },
    //"es":[PID,STATE,CCERR,SCRAMBLES]

    dst += R"(,"root":[{"es":[0,)";
    dst += to_string(pmt->StateUI()) + ",";
    dst += to_string(summary_cc);
    dst += R"(,0],"type":["IPTS:",)" + json_string(text_traffic);
    dst += R"(,"","Summary: Traffic , CCErrors  , from[)" +  string(pan->IpSrc().c_str())  +R"(]",)"  + json_format02f(traffic);
    dst += R"(],"checked":{"scramble":0,"traffic_min":0,"traffic_max":0,"ccerror_max":0}})";

    rcount=1;

    if (measurer & Measure.IsValid()) {
        if (rcount)   dst +=",";   else rcount=1;

        dst += R"({"es":[0,3,0,0],"type":["0",)";
        dst += Measure.Packets_drop();
        dst += R"(,"","packets drop",1.0],"checked":{"scramble":0,"traffic_min":0,"traffic_max":0,"ccerror_max":0}},)";

        dst += R"({"es":[0,3,0,0],"type":["0",)";
        dst += Measure.Packets_reordered();
        dst += R"(,"","packets reorder",1.0],"checked":{"scramble":0,"traffic_min":0,"traffic_max":0,"ccerror_max":0}},)";

        dst += R"({"es":[0,3,0,0],"type":["0",)";
        dst += Measure.Packets_ok();
        dst += R"(,"","packets ok",1.0],"checked":{"scramble":0,"traffic_min":0,"traffic_max":0,"ccerror_max":0}},)";

        dst += R"({"es":[0,3,0,0],"type":["0",)";
        dst += Measure.Packets_Summary();
        dst += R"(,"","packets summary",1.0],"checked":{"scramble":0,"traffic_min":0,"traffic_max":0,"ccerror_max":0}},)";

        dst += R"({"es":[0,3,0,0],"type":["0",)";
        dst += Measure.ErrPercent();
        dst += R"(,"","percent err",1.0],"checked":{"scramble":0,"traffic_min":0,"traffic_max":0,"ccerror_max":0}})";
        }

    // =============== Common ==================
    for (size_t i=0;i < pan->CountRoot();++i) {
        pes = pan->GetRoot(i);
        if (!pes) continue;

       // text_traffic = format_traffic(pes->RcvBytes());
       // traffic = double(pes->RcvBytes() * 8) / 1000.0;

        if (rcount!=0)   dst +=","; else rcount=1;

        json_es(pes,dst);
        dst += ",";
        json_type(pes,dst);
        dst += ",";
        json_checked(pes,dst);
        dst += "}";
        }

     // =============== Out of band ==================
    for (size_t  i=0;i < pan->CountUnexpected();++i) {
        pes = pan->GetUnexpected(i);
        if (!pes) continue;

        //text_traffic = format_traffic(pes->RcvBytes());
       // traffic = double(pes->RcvBytes() * 8) / 1000.0;
        if (rcount)   dst +=","; else rcount=1;

        json_es(pes,dst);
        dst += ",";
        json_type(pes,dst);
        dst += ",";
        json_checked(pes,dst);
        dst += "}";
        }
    // =============== STUFF ==================
    pes = pan->GetStuff();
    if (pes) {
       // text_traffic = format_traffic(pes->RcvBytes());
        //traffic = double(pes->RcvBytes() * 8) / 1000.0;
        if (rcount)   dst +=","; else rcount=1;

        json_es(pes,dst);
        dst += ",";
        json_type(pes,dst);
        dst += ",";
        json_checked(pes,dst);
        dst += "}";
        }
    dst += R"(])";
    }  // COMMON

    // ************************* SERVICE
    size_t count_es = pan->CountEs(idx_pmt);
    if (count_es) {
        rcount=0;
        dst += R"(,"service":[)";

        for (size_t i=0;i<count_es;++i) {
            TAnalizerES *pes  = pan->GetEs(idx_pmt,i);
            if (!pes) continue;

            //string typ = PesTypeText(pes);   // PesType

             // PMT
            if (pes->IsPMT()) {
                if (rcount)   dst +=",";   else rcount=1;

                json_es(pes,dst);
                dst += ",";
                dst += R"("type":[)";
                dst += json_string(GetTypeText(pes)) + ",";
                dst += json_string(format_traffic(pes->RcvBytes())) + ",";
                dst += R"("")"s + ",";

                    dst += json_string(
                            "sid=" + to_string(pes->PmtSid()) +
                            " [" + pes->PmtProvider() + "]" +
                            " [" + pes->PmtServiceName() + "]" +
                            " [ini=" + pan->Info.Name()  + "]"  //pan->ChannelNameIni() + "]"
                            ) + ",";

                dst += format_02f((pes->RcvBytes() * 8) / 1000.0)  + "]";
                dst += ",";
                json_checked(pes,dst);
                dst += "}";
                }
            else {
                // PES
                if (rcount)   dst +=",";   else rcount=1;

                json_es(pes,dst);
                dst += ",";
                dst += R"("type":[)";
                dst += json_string(GetTypeText(pes)) + ",";
                dst += json_string(format_traffic(pes->RcvBytes())) + ",";
                dst += R"("")"s + ",";
                dst += json_string(GetStreamText(pes)) + ",";
                dst += format_02f((pes->RcvBytes() * 8) / 1000.0)  + "]";
                dst += ",";
                json_checked(pes,dst);
                dst += "}";
                }

            // PCR
            if (pes->IsPCR()) {
                if (rcount!=0)   dst +=",";   else rcount=1;

                json_es(pes,dst);
                dst += ",";
                dst += R"("type":[)";
                dst += json_string("PCR"s) + ",";
                dst += json_string(format_02f(pes->PcrPerSecond())  + " pcr/s") + ",";
                dst += R"("")"s + ",";

                dst += json_string(
                        "Curr=" + json_format02f(pes->PcrBitRate()) +
                        " Min=" + json_format02f(pes->PcrBitRateMin()) +
                        " Max=" + json_format02f(pes->PcrBitRateMax())
                        ) + ",";

                dst += R"(0.00])";
                dst += ",";
                json_checked(nullptr,dst);
                //dst += "}";

                }

            } //For ES
        dst += R"(])";
        }
    dst += R"(}})";
}



const string &TUI::StatusInfoAndDetail(unsigned line_index)
{
    ui_statusinfo_and_detail = "";
    ui_statusinfo_and_detail.reserve(8000);
    ui_statusinfo_and_detail += ui_statusinfo;
    ui_statusinfo_and_detail.pop_back();   // remove last '}'
    Create_UI_Detail(line_index,ui_statusinfo_and_detail);
    return ui_statusinfo_and_detail;
}



unsigned TUI::GetIndexTS(unsigned line_index) {  // used in wsconnection only
    if ( line_index >= table_pmt_info.size()) return 0;
    return table_pmt_info[line_index].ts_index;
}

unsigned TUI::GetIndexPMT(unsigned line_index) { // used in wsconnection only
    if ( line_index >= table_pmt_info.size()) return 0;
    return table_pmt_info[line_index].pmt_index;
}

string TUI::GetMcastIP(unsigned line_index)    // used in wsconnection only
{
    if ( line_index >= table_pmt_info.size()) return "";
    TAnalizerTS *pan   = table_pmt_info[line_index].ptr_ts;
    return string(pan->Info.IP().c_str()) + ":" + pan->Info.Port().c_str();
}

string TUI::GetMcastPMT(unsigned line_index)   // used in wsconnection only
{
    if (line_index >= table_pmt_info.size()) return "";
    TAnalizerES *pmt   = table_pmt_info[line_index].ptr_pmt;
    return to_string(pmt->Pid());
}



void TUI::Run()
{
    ui_counter++;

        PmtInfo_Prepare();
        Create_UI_ServerInfo(); //Log.Scr("UI_ServerInfo ");
        Create_UI_StatusInfo(); //Log.Scr("UI_StatusInfo ");

        if (Time.IsMeasureEnd()) {
            Create_UI_Statistic();
           // Log.Scr("StatisticInfo >> %s\n",StatisticInfo().c_str());
        }

    if (updated_structure)    {
        //Create_UI_ServerInfo();
        //Journal.Add(m_server_info);
        //Journal.Add(ui_serverinfo.c_str());
       // Log.ScrFile("Update Structure ui_serverinfo >> sz=%lu cap=%lu\n",ui_serverinfo.size(),ui_serverinfo.capacity());

      //  Log.ScrFile("Update Structure >> %s\n",ui_serverinfo.c_str());
        }
    if (updated_state) {
        //Create_UI_StatusInfo();
        //Journal.Add(m_status_info);
        //Journal.Add(ui_statusinfo.c_str());
        //Log.ScrFile("Update Status >>%s\n",ui_statusinfo.c_str());
        }

//Log.Scr("ServerInfo>>%s\n",ui_serverinfo.c_str());
//Log.Scr("StatusInfo>>%s\n",ui_statusinfo.c_str());
//Log.Scr("StatusInfoAndDetail>>%s\n",ui_statusinfo_and_detail.c_str());
//Log.Scr("StatisticInfo>>%s\n",ui_statisticinfo.c_str());
}
