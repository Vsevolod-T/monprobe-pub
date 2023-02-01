#include "tconfig.h"
#include "tini.h"

 TConfig  Config;




 unsigned TConfig::m_find_checked(TIPAddress ip, TIPPort port)
 {
     for (unsigned i=0; i<m_es_checked.size();++i) {
         if (m_es_checked[i].ip == ip && m_es_checked[i].port==port)
             return i;
         }
     return BADINDEX;
 }


void TConfig::m_fill_checked(const string &section)
{
    //239.255.50.92 5500   Сарафан_MSK-207      2848  SD
    //239.255.250.180 5503 BOOMERA_TMN_MP4      1512  SD
    //235.72.207.1 2015 RTRS1
    //. Первый   1010 1011 500 1012 25
    //. Россия   1020 1021 500 1022 25

    string last_name_if;
    TIPAddress last_if;
    TIPAddress last_ip;
    TIPPort last_port;
    string name;


    unsigned access_idx = 0;
    unsigned size_ini = Ini.Count();

    while (access_idx < size_ini) {

        string first = Ini.GetStrIdx(section,access_idx,"",0);

        if (first=="")  {  break; }

        // NO sub channel = SPTS
        if  (first  !=  "." ) {
            last_name_if = Ini.GetStr(section,"interface","");
            last_if = Ini.GetIP(section,"interface");
            last_ip =  Ini.GetIPIdx(section,access_idx,0);
            last_port =  Ini.GetUIntIdx(section,access_idx,0,1);
            name =  Ini.GetStrIdx(section,access_idx,"",2);

//unsigned GetUInt16Idx(const string & section, const unsigned idx_val, const unsigned default_value,const unsigned idx_par=0);

            //if (last_if.Bad())  { access_idx++; continue; }
            if (last_ip.Bad())  { access_idx++; continue; }
            if (last_port.Bad()) { access_idx++; continue; }

             _checked ch;
             ch.section = section;
             ch.name_if = last_name_if;
             ch.ip_if = last_if;
             ch.ip   = last_ip;
             ch.port = last_port;
             ch.name = name;

             m_es_checked.push_back(ch);
            }



        access_idx++;
    } // while () next record


}




