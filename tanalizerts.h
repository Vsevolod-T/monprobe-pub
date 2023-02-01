#pragma once

#include "theader.h"

#include "tini.h"
#include "tlog.h"

#include "trawes.h"
#include "trawts.h"

#include "tanalizeres.h"



struct _info_ts {

    vector < vector <unsigned> > data_es;
    vector <unsigned> common_es;
    vector <unsigned> unexpected_es;
    vector <unsigned> stuff_es;
     uint32_t  ip_src;

    void add_data_vec(unsigned idx_pmt,unsigned idx_es,unsigned store_idx)
    {
    if (idx_pmt >= data_es.size()) {
        // add N pmt
        size_t diff = idx_pmt - data_es.size() ;
        size_t sz = data_es.size();
        data_es.resize(sz+diff+1);
        }

    if (idx_es >= data_es[idx_pmt].size()) {
        // add N es
        size_t diff = idx_es - data_es[idx_pmt].size() ;
        size_t sz = data_es[idx_pmt].size();
        data_es[idx_pmt].resize(sz+diff+1);
        }

    data_es[idx_pmt][idx_es] = store_idx;
    }


    void add_common_vec(unsigned store_idx)
    {
        size_t sz = common_es.size();
        common_es.resize(sz+1);
        common_es[sz]=store_idx;
    }


    void add_unexpected_vec(unsigned store_idx)
    {
        size_t sz = unexpected_es.size();
        unexpected_es.resize(sz+1);
        unexpected_es[sz]=store_idx;
    }


    void add_stuff_vec(unsigned store_idx)
    {
        size_t sz = stuff_es.size();
        stuff_es.resize(sz+1);
        stuff_es[sz]=store_idx;
    }

    void initialize_vec() {
        data_es.clear();
        common_es.clear();
        unexpected_es.clear();
        stuff_es.clear();
        }

    }__attribute__ ((aligned (16)));







class TAnalizerTS
{
    friend class TChannel;

    TRawTS      RawTS;                   //Copy RawTS from Channel (IP)
    void FillInfoFromRaw();

    vector <TAnalizerES> m_pool_es;
    _info_ts    m_info;
    TIPAddress m_ipsrc;

    _es_timeouts timeouts;  //LOST , RESTORE ..


private:

    struct _out {
        //bool   log_update_ccerror_above_limit; //# out to log if cc error
       // bool   log_update_status_restore_lost; //# out to log if restore/lost
        bool   print_analizer_out;
        }out;

    struct _updated {
        unsigned prev_count_pmt;
        bool     raw_pmt;
       } updated;

private:

    // call this
    TAnalizerES * SearshPmtPid(unsigned pmt_pid) {
        size_t sz = CountPmt();
        for (unsigned i=0;i<sz;++i) {
            TAnalizerES *pes = GetEs(i,0);
            if (pes && pes->Pid()==pmt_pid) return pes;
            }
        return nullptr;
        }


    // called from eq sw
   unsigned SearshPmtIdx(unsigned pmt_pid) {
       size_t sz = CountPmt();
       for (unsigned i=0;i<sz;++i) {
           TAnalizerES *pes = GetEs(i,0);
           if (pes && pes->Pid()==pmt_pid) return i;
           }
       return 0;
       }


private:

    // !!!  CALLED from Channel  (HOT) !!!
    void CopyFromChannelToRawAnalize(TRawTS &inp_ts,unsigned total_bytes) {
        //Log.Scr("An [%s:%u]\n",Info.IP().c_str(),Info.Port().bin());
        RawTS = inp_ts;
        ts_summary_bytes = total_bytes;
        m_ipsrc = inp_ts.GetSrc();
        //Log.Scr("pmt [%u]\n",inp_ts.);
        }

    void CheckRawUpdateStructure();
    void CopyAll_RawEsToAnalizerEs();
    void Sorting();

public:

    unsigned ts_summary_bytes=0;

    TIPAddress& IpSrc() { return m_ipsrc; }


    // ************************************************* 0x0000-0x001f COMMON (ROOT)
    size_t    CountRoot()  const               { return m_info.common_es.size(); }

    TAnalizerES *GetRoot(size_t IdxCommon)  {
        unsigned idx_pool  = m_info.common_es[IdxCommon];
        return &m_pool_es[idx_pool];
        //if (m_pool_es[idx_pool].pool_state != POOL_ES_COMMON) return nullptr;
        }

    // ************************************************* 0x0020-0x1ffe  DATA (PMT+ES)
    size_t    CountPmt()  const              { return m_info.data_es.size(); }
    size_t    CountEs(size_t IdxPmt) const { return m_info.data_es[IdxPmt].size(); }

    TAnalizerES *GetPmt(size_t IdxPmt) {
        unsigned idx_pool = m_info.data_es[IdxPmt][0];
        return &m_pool_es[idx_pool];
        //if (m_pool_es[idx_pool].pool_state == POOL_ES_UNUSED) return nullptr;
        }

    TAnalizerES *GetEs(size_t IdxPmt,size_t IdxEs) {
        unsigned idx_pool = m_info.data_es[IdxPmt][IdxEs];
        return &m_pool_es[idx_pool];
        //if (m_pool_es[idx_pool].pool_state == POOL_ES_UNUSED) return nullptr;
        }

    //TAnalizerES *GetStub() { return &m_stub; }
    // ************************************************* 0x1fff  STUFF
    TAnalizerES *GetStuff() {
        if (m_info.stuff_es.size()==0)
            return nullptr;
        unsigned idx_pool = m_info.stuff_es[0];
        return &m_pool_es[idx_pool];
        //if (m_pool_es[idx_pool].pool_state != POOL_ES_STUFF) return nullptr;
        }

    // *************************************************  0x0020-0x1ffe   UNEXPECTED
    size_t     CountUnexpected()  const { return m_info.unexpected_es.size();   }

    TAnalizerES *GetUnexpected(size_t IdxUnexpected) {
        unsigned idx_pool  = m_info.unexpected_es[IdxUnexpected];
        return &m_pool_es[idx_pool];
        //if (m_pool_es[idx_pool].pool_state != POOL_ES_UNEXPECTED) return nullptr;
        }
    // ***************************************************************************************** POOL END

public:
    TAnalizerTS() {}
    ~TAnalizerTS() {}

    bool Initialize(const TInfo& v);

    void Run(bool start_mode);

    TInfo Info;

    unsigned GetRawRecievedBytes() { return ts_summary_bytes; }

    // ******************** (ini)
string ini_name;
string ChannelNameIni() {
        return ini_name;
        }

    void SetTimeouts(const _es_timeouts &taconfig)  { timeouts = taconfig; }

    bool IsUpdatedRawPmt() { return updated.raw_pmt; }

public:

    // +++++++++++++++++++++++++++++++++++++++++ statistic
    _statistic     ts_statistic_second;            // 1 sec
    _statistic     ts_statistic_period_accumulate; // for log & stat_current
    _statistic     ts_statistic_period_final;      // for stat
    // +++++++++++++++++++++++++++++++++++++++++


    TRawTS *GetRawTS() { return &RawTS; }
};


extern vector <TAnalizerTS *> pAnalizersArray;
extern TAnalizerTS *FindAnalizerByIp(TIPAddress ip_if,TIPAddress ip,TIPPort port);
extern TAnalizerTS *FindAnalizer(const string &sect,TIPAddress ip,TIPPort port);

