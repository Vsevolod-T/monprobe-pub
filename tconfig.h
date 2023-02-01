#pragma once

#include "theader.h"


struct _checked {
    string section;
    string name_if;
    TIPAddress  ip_if;
    TIPAddress  ip;
    TIPPort  port;
    string name="";                 // ts name
};


class TConfig
{
    const unsigned BADINDEX = 99999999;

   vector <_checked>  m_es_checked;

    void  m_fill_checked(const string& section);

    unsigned m_find_checked(TIPAddress  ip, TIPPort port);


public:
TConfig() { }

unsigned CountIP() { return unsigned(m_es_checked.size()); }
_checked *GetIP(unsigned idx_ip) { return &m_es_checked[idx_ip]; }


void Initialize() {

        for (unsigned i=0;i<MAX_INTERFACES;++i) {
            string section = SECTION_INP + to_string(i);
            m_fill_checked(section);
            }
        }
};



extern  TConfig  Config;
