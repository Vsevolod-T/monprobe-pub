#include "trawts.h"
#include "tglobal.h"
#include "tlog.h"



void TRawTS::FreePatPmtData()
{
    TRawES  *p = GetIdxCommon(0);
    if (!p) return;
    if (p->pool_timeout) return;

    // Del PAT -> Del All
    for (unsigned i=0;i<pool.size();++i) {
        if (pool[i].pool_state==POOL_ES_COMMON || pool[i].pool_state==POOL_ES_DATA)
            pool[i].pool_state=POOL_ES_UNUSED;
        }
    //unsigned szY = CountPmt();
    //CSetCountData(idx_pmt,0);
  CSetCountPmt(0);
}



void TRawTS::TimeoutedDelete()
{
    //
    // timeouts ..
    //
    FreePatPmtData();

    unsigned sz=pool.size();
    for (unsigned i=0;i<sz;++i) {
        TRawES *p = &pool[i];
        if (p->pool_state == POOL_ES_UNUSED)
            continue;
        if (p->pool_timeout) {
            p->pool_timeout--;
            }
        else {
            // timeout end , delete all
            p->pool_state = POOL_ES_UNUSED;
            }
        } // for POOL
}

void TRawTS::Clear()
{
    unsigned sz=pool.size();
    for (unsigned i=0;i<sz;++i) {
        TRawES *p = &pool[i];
        if (p->pool_state == POOL_ES_UNUSED) continue;
        p->Clear();
        }
}

TRawTS &TRawTS::operator = (TRawTS &other)
{
    if (this == &other) return *this;

    pool = other.pool;

    return *this;
}

void TRawTS::Initialize(unsigned second_alive)
{
    m_time_alive = second_alive;
}
