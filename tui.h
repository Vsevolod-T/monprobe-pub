#pragma once

#include "theader.h"


class TAnalizerTS;
class TAnalizerES;


class TUI
{
    struct  _pmt_info {
        TAnalizerTS *ptr_ts;         //
        TAnalizerES *ptr_pmt;
        unsigned ts_index;           // index ts in AnalizersArray
        unsigned ts_count_pmt;   // count pmt in ts
        unsigned pmt_index;        // index pmt in ts
        unsigned pmt_state;        // stored ui status
        unsigned pmt_updated;   //
        };
    vector <_pmt_info>   table_pmt_info;

    bool updated_structure = false;
    bool updated_state = false;

    bool IsUpdatedPmtInfoStructure(_pmt_info *stored,_pmt_info *readed);
    bool IsUpdatedPmtInfoStatus(_pmt_info *stored,_pmt_info *readed);

    void PmtInfo_Fill();
    void PmtInfo_Check();
    void PmtInfo_Prepare();

    uint64_t ui_counter=0;

    string ui_serverinfo;
    void  Create_UI_ServerInfo();

    string ui_statusinfo;
    void  Create_UI_StatusInfo();

    string ui_statisticinfo;
     void Create_UI_Statistic();

    string json_string(string val)   { return R"(")"s + val + R"(")"; }
    string json_string(const char *val)    { return R"(")"s + val + R"(")"; }

    string json_format02f(double val) { return format_02f(val); }


    void  json_es(TAnalizerES *pes,string &dst);
    void  json_type(TAnalizerES *pes,string &dst);
    void  json_checked(TAnalizerES *pes,string &dst);

    void  Create_UI_Detail(unsigned line_index,string &dst);

    string ui_statusinfo_and_detail;


public:

    TUI()  {
    ui_serverinfo.reserve(64000);
    ui_statusinfo.reserve(8000);
    ui_statisticinfo.reserve(8000);
    }
    ~TUI() {}

    void  Read() {}
    void  Run();

    unsigned  GetIndexTS(unsigned line_index);          // used in wsconnection only
    unsigned  GetIndexPMT(unsigned line_index);       // used in wsconnection only

    string    GetMcastIP(unsigned line_index);              // used in wsconnection only
    string    GetMcastPMT(unsigned line_index);          // used in wsconnection only

    // called from wsconnection.run
    bool  IsUpdatedStructure()  { return updated_structure; }
   // bool  IsUpdatedStatus()     { return updated_state; }
    const string &ServerInfo()  { return ui_serverinfo; }
    const string &StatusInfo()  { return ui_statusinfo; }
    const string &StatusInfoAndDetail(unsigned line_index);

    const string &StatisticInfo()  { return ui_statisticinfo; }
};

extern TUI UI;

