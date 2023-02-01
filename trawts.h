#pragma once

#include "theader.h"
#include "trawes.h"



class TRawTS
{
    friend class TAnalizerTS;


    vector<TRawES> pool;

    TRawES  *find_pid(unsigned pid) {
        for (unsigned i=0;i<pool.size();++i) {
            if (pool[i].pid==pid) return &pool[i];
            }
        return nullptr;
        }

    TRawES  *find_state_idx(unsigned state,unsigned idx) {
        for (unsigned i=0;i<pool.size();++i) {
            if (pool[i].pool_state==state && pool[i].pool_idx_es==idx)
                    return &pool[i];
            }
        return nullptr;
        }

    TRawES  *find_data(unsigned pmt_idx,unsigned es_idx) {
        for (unsigned i=0;i<pool.size();++i) {
            if (pool[i].pool_state==POOL_ES_DATA && pool[i].pool_idx_pmt==pmt_idx && pool[i].pool_idx_es==es_idx) return &pool[i];
            }
        return nullptr;
        }

    unsigned   m_time_alive;

    unsigned countPmt=0;


     uint32_t  ip_src;


    void FreePatPmtData();

public:

    void SetSrc(uint32_t src) { ip_src = src; }
    uint32_t GetSrc() { return ip_src; }


    // ************************************************* 0x0000-0x001f COMMON (ROOT)
    TRawES *CInsertCommon(unsigned pid) {

        TRawES  *p = find_pid(pid);
        if (!p) {
            // not pid, insert
            TRawES x;
            pool.push_back(x);
            p=&pool.back();
            // fill
            p->pool_state = POOL_ES_COMMON;
            p->pool_timeout = m_time_alive;
            p->pool_idx_pmt=0; // used
            p->pool_idx_es=pid;  // pid as idx
            p->pid =pid;
            return p;
            }

        p->pool_state = POOL_ES_COMMON;
        p->pool_timeout = m_time_alive;
        p->pool_idx_pmt=0; // used
        p->pool_idx_es=pid;  // pid as idx
        p->pid =pid;
        return p;
        }

    TRawES *GetIdxCommon(unsigned idx_common) {
        TRawES  *p = find_state_idx(POOL_ES_COMMON,idx_common);
        return p;
        }


     // ************************************************* 0x0020-0x1ffe  DATA (PMT+ES)
    TRawES *CInsertData(unsigned pid,unsigned idx_pmt,unsigned idx_es) {


        TRawES  *p = find_pid(pid);
        if (!p) {
            // not pid, insert
            TRawES x;
            pool.push_back(x);
            p=&pool.back();
            // fill
            p->pool_state = POOL_ES_DATA;
            p->pool_timeout = m_time_alive;
            p->pool_idx_pmt=idx_pmt; // used
            p->pool_idx_es=idx_es;  // ??
            p->pid =pid;
            return p;
            }

        p->pool_state = POOL_ES_DATA;
        p->pool_timeout = m_time_alive;
        p->pool_idx_pmt=idx_pmt; // used
        p->pool_idx_es=idx_es;  // ??
        p->pid =pid;
        return p;
        }

    TRawES *CGetData(unsigned pid) {
        TRawES  *p = find_pid(pid);
        return p;
        }

    TRawES *GetIdxData(unsigned pmt_idx,unsigned es_idx) {
        TRawES  *p = find_data(pmt_idx, es_idx);
        return p;
        }

    void        CSetCountPmt(unsigned count)        { countPmt = count;    }
    unsigned    GetCountPmt() const                    { return countPmt;     }

    // ************************************************* 0x1fff  STUFF
    TRawES *CInsertStuff() {

        TRawES  *p = find_pid(0x1fff);
        if (!p) {
            // not pid, insert
            TRawES x;
            pool.push_back(x);
            p=&pool.back();
            // fill
            p->pool_state = POOL_ES_STUFF;
            p->pool_timeout = m_time_alive;
            p->pool_idx_pmt=0; // used
            p->pool_idx_es=0;  // ??
            p->pid =0x1fff;
            return p;
            }

        // fill
        p->pool_state = POOL_ES_STUFF;
        p->pool_timeout = m_time_alive;
        p->pool_idx_pmt=0; // used
        p->pool_idx_es=0;  // ??
        p->pid =0x1fff;

        return p;
        }

    TRawES *GetStuff() {
        return find_pid(0x1fff);
        }

    // *************************************************  0x0020-0x1ffe   UNEXPECTED
    TRawES *CInsertUnexpected(unsigned pid) {

        TRawES  *p = find_pid(pid);
        if (!p) {
            // not pid, insert
            TRawES x;
            pool.push_back(x);
            p=&pool.back();
            // fill
            p->pool_state = POOL_ES_UNEXPECTED;
            p->pool_timeout = m_time_alive;
            p->pool_idx_pmt=0; // used
            p->pool_idx_es=0;  // ??
            p->pid =pid;
            return p;
            }

        if (p->pool_state == POOL_ES_UNEXPECTED  || p->pool_state == POOL_ES_UNUSED)  {
            p->pool_state = POOL_ES_UNEXPECTED;
            p->pool_timeout = m_time_alive;
            p->pool_idx_pmt=0; // used
            p->pool_idx_es=0;  // ??
            p->pid =pid;
            return p;
            }
        return nullptr;
        }

    unsigned     CountUnexpected() const { return 0;   }

    TRawES *GetIdxUnexpected(unsigned idx_unexp) {
        TRawES  *p = find_state_idx(POOL_ES_UNEXPECTED,idx_unexp);
        return p;
        }

    // *************************************************
    void TimeoutedDelete();
    void Clear();

    TRawTS& operator = (TRawTS &other);

    void Initialize(unsigned second_alive);

    TRawTS() { Initialize(5); }

};
