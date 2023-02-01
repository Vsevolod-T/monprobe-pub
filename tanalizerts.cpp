#include "tanalizerts.h"
#include "tdecode.h"
#include "ttime.h"

vector <TAnalizerTS *> pAnalizersArray;


TAnalizerTS *FindAnalizerByIp(TIPAddress ip_if,TIPAddress ip,TIPPort port)
{
    for (auto & p : pAnalizersArray) {
        if (  (p->Info.IF() == ip_if) &&
              (p->Info.IP() ==  ip) &&
              (p->Info.Port() ==  port) )
                          return p;
        }  // for
    return nullptr;
}
TAnalizerTS *FindAnalizer(const string &sect,TIPAddress ip,TIPPort port)
{
    for (auto & p : pAnalizersArray) {
        if (  (p->Info.Section() ==sect ) &&
              (p->Info.IP() ==  ip) &&
              (p->Info.Port() ==  port) )
                          return p;
        }  // for
    return nullptr;
}


//
void TAnalizerTS::FillInfoFromRaw()
{
    m_info.initialize_vec();


    for (unsigned i=0;i<RawTS.pool.size();++i) {
        unsigned state   = RawTS.pool[i].pool_state;
        unsigned idx_pmt = RawTS.pool[i].pool_idx_pmt;
        unsigned idx_es  = RawTS.pool[i].pool_idx_es;

        switch (state) {

            case POOL_ES_UNUSED:
                break;
            case POOL_ES_COMMON:
                //m_info.add_common(i);
                m_info.add_common_vec(i);
                break;
            case POOL_ES_UNEXPECTED:
                m_info.add_unexpected_vec(i);
                break;
            case POOL_ES_STUFF:
                m_info.add_stuff_vec(i);
                break;

            //case POOL_ES_PMT: break;
            case POOL_ES_DATA:
                m_info.add_data_vec(idx_pmt,idx_es,i);
                break;
            default:
                break;
        };
    }


    if (m_pool_es.size() < RawTS.pool.size()) {
        size_t idx_start = m_pool_es.size();
        size_t idx_end   = RawTS.pool.size();

        m_pool_es.resize(RawTS.pool.size());

        for (size_t i=idx_start;i<idx_end;++i)
            m_pool_es[i].SetDefault();
        }



//m_info.print_data_vec();

}

void TAnalizerTS::CheckRawUpdateStructure()
{
    unsigned count_pmt=RawTS.GetCountPmt();
    if (updated.prev_count_pmt != count_pmt) {
        updated.raw_pmt = true;
        updated.prev_count_pmt = count_pmt;
        return;
        }

   updated.raw_pmt = false;

    for (unsigned i=0;i<count_pmt;++i) {
        unsigned pid=0;
        TRawES *p = RawTS.GetIdxData(i,0);
        if (p)
            pid = p->pid;

        if  (SearshPmtPid(pid)==nullptr) {
            updated.raw_pmt = true;
            return;
            }
        }
}

void TAnalizerTS::CopyAll_RawEsToAnalizerEs()
{
    size_t sz=RawTS.pool.size();
    for (unsigned i=0;i<sz;++i) {
        TRawES *pes       = &RawTS.pool[i];
        TAnalizerES *paes = &m_pool_es[i];
        paes->pool_state  = pes->pool_state;
        if (pes->pool_state != POOL_ES_UNUSED)
             paes->CopyRawToAnalizer(pes);
        }
}

void TAnalizerTS::Sorting()
{
    unsigned *start;
    unsigned *end;

    // sort common
    size_t count  = CountRoot();
    if (count) {
        start = &m_info.common_es[0];
        end   = &m_info.common_es[count];
        sort(start,end,[this](unsigned a,unsigned b) { return m_pool_es[a].pid < m_pool_es[b].pid; });
        }

    // sort data
    size_t szY = CountPmt();
    for (unsigned i=0;i<szY;++i) {

        count = CountEs(i);
        if (count) {
            start = &m_info.data_es[i][1];
            end   = &m_info.data_es[i][count];
            sort(start,end,[this](unsigned a,unsigned b) { return m_pool_es[a].pid < m_pool_es[b].pid; });
            }
        }

    // sort unexpected
    count = CountUnexpected();
    if (count) {
        start = &m_info.unexpected_es[0];
        end   = &m_info.unexpected_es[count];
        sort(start,end,[this](unsigned a,unsigned b) { return m_pool_es[a].pid < m_pool_es[b].pid; });
        }
}







bool TAnalizerTS::Initialize(const TInfo& v)
{
    Info = v;
    out.print_analizer_out = Ini.GetBoolean("DEBUG","print_analizer_out",false);
    //out.log_update_ccerror_above_limit = Ini.GetBoolean("Log","update_ccerror_above_limit",false);	//# out to log if cc error > limit
    //out.log_update_status_restore_lost = Ini.GetBoolean("Log","update_status_restore_lost",false);  //# out to log if restore/lost

    RawTS.Initialize(0);
    m_info.initialize_vec();
    return true;
}


//============================================================
void TAnalizerTS::Run(bool start_mode)
{
    FillInfoFromRaw();
    CheckRawUpdateStructure();

    if (updated.raw_pmt) {
        //Log.ScrFile(" Updated raw_pmt detected\n");
        }

    CopyAll_RawEsToAnalizerEs();               // and parse headers si, pes
    Sorting();

    // ***********************************************************************************************

    // move to AnalizerES ?
    // all es set minimal traffic                                                       old SetAutoMinimalTrafficEs();
    // pes->SetAutoMinimalTraffic()
    // compute es state (ui ..  )
    // pes->ComputeEsState()

    // compute pmt state from all es's (ui , cc, traffic)
    // compute & add to statistic from pmt (cc,traffic,seconds)
    // if period -> statistic_prev
    // this !filled  pes->es.stream_info.type_stream  STREAM_TYPE_AUDIO_MPEG1 ....

    ts_statistic_second.reset();

    // compute pmt from all es state per 1 second
    TAnalizerES *pes=nullptr;
    unsigned ts_second_ccerror     = 0;
    unsigned ts_second_bad_traffic = 0;

    // Add all root es
    for (unsigned i=0;i<CountRoot();++i) {
        pes=GetRoot(i);
        if (!pes) continue;
        ts_statistic_second.summary_bytes    += pes->es.bytes;
        ts_statistic_second.summary_ccerrors += pes->es.cc_errors;
        }

    // Add stuff es
    pes = GetStuff();
    if (pes)  {
        ts_statistic_second.summary_bytes    += pes->es.bytes;
        //ts_statistic_1sec_ui.summary_ccerrors += pes->es.cc_errors;
        }

    // Add all unexpected es
    size_t cnt = CountUnexpected();
    for (size_t i=0;i<cnt;++i) {
        pes = GetUnexpected(i);
        if (!pes) continue;
        ts_statistic_second.summary_bytes    += pes->es.bytes;
        //ts_statistic_1sec_ui.summary_ccerrors += pes->es.cc_errors;
        }

    size_t count_pmt = CountPmt();

//Log.Scr("COUNT_PMT=%lu\n",count_pmt);

    // Add all pmt and all es in pmt
    for (size_t idx_pmt=0;idx_pmt< count_pmt;++idx_pmt) {

        unsigned pmt_bytes = 0;
        unsigned pmt_cc_errors = 0;
        bool     pmt_updated   = false;

        es_ui_state minimal_state  = UI_STREAM_OK;

        size_t count_es  = CountEs(idx_pmt);

//Log.Scr("COUNT_ES=%lu\n",count_es);

        for (size_t idx_es=0;idx_es<count_es;++idx_es) {
            pes = GetEs(idx_pmt,idx_es);
            if (!pes) continue;

            // compute all es state
            pes->ComputeEsMinimalTraffic();   // new
            pes->ComputeEsState();

            pmt_bytes       += pes->es.bytes;
            //pmt_cc_errors += pes->es.cc_errors;
            pmt_cc_errors += pes->RcvErrors_ForTS();

            if (pes->StateUpdated())
                pmt_updated = true;

// **********************************
//Log.Scr("PID %u  StateUI %u  minimal_state %u\n",pes->Pid(),pes->StateUI(),minimal_state);
// **********************************

            if (minimal_state > pes->es.es_ui_current_state) //StateUI())
                minimal_state = pes->es.es_ui_current_state;//pes->StateUI();
            }

        // set to pmt
        TAnalizerES *ppmt  = GetPmt(idx_pmt);
         if (!ppmt) continue;

         ppmt->pmt.statistic_second.reset();

         if (minimal_state==UI_STREAM_NO) {
             ppmt->pmt.statistic_second.second_bad_traffic++;
             ts_second_bad_traffic++;
            }
         if (pmt_cc_errors) {
             minimal_state=UI_QUALITY;
             ppmt->pmt.statistic_second.second_ccerror++;
             ts_second_ccerror++;

//Log.Scr("CC PID %u  StateUI %u  minimal_state %u\n",ppmt->Pid(),ppmt->StateUI(),minimal_state);
             }

         ppmt->pmt.ui_state    = minimal_state;
         ppmt->pmt.pmt_updated = pmt_updated;

         ppmt->pmt.statistic_second.summary_bytes      = pmt_bytes;
         ppmt->pmt.statistic_second.summary_ccerrors  = pmt_cc_errors;

         ts_statistic_second.summary_bytes    += pmt_bytes;
         ts_statistic_second.summary_ccerrors += pmt_cc_errors;

// **********************************
Log.Scr("PMT %u  StateUI %u pmt.ui_state %u pmt_cc_errors %u\n",ppmt->Pid(),ppmt->StateUI(),ppmt->pmt.ui_state,pmt_cc_errors);
// **********************************
        }

    if (ts_second_ccerror)      ts_statistic_second.second_ccerror++;
    if (ts_second_bad_traffic)  ts_statistic_second.second_bad_traffic++;

    ts_statistic_period_accumulate += ts_statistic_second;

    if (Time.IsMeasureEnd()) {
        ts_statistic_period_final = ts_statistic_period_accumulate;
        ts_statistic_period_accumulate.reset();
        }

    if (start_mode) {
        ts_statistic_second.reset();
        ts_statistic_period_accumulate.reset();
        ts_statistic_period_final.reset();
        }


    // ***********************************************************************************************

    // *************************************** Set UI Name
    for (size_t idx_pmt=0; idx_pmt < count_pmt; ++idx_pmt) {
        TAnalizerES *ppmt = GetPmt(idx_pmt);
        if (!ppmt) continue;

        if (count_pmt==1) {
            // SPTS
            if (Info.Name().empty())
                ppmt->PmtNameUISet(ppmt->PmtServiceName());
             else
                ppmt->PmtNameUISet(Info.Name());
            }
        else // MPTS
            ppmt->PmtNameUISet(ppmt->PmtServiceName());
        } // next pmt
    // *********************************************

    if (out.print_analizer_out)
        PRINT_ANALIZED_TS(this,"Analized\n");
}





