#pragma once

#include "theader.h"


class TIni
{
    const unsigned BADINDEX = 99999999;

    vector < string> strings;   //original file

   struct _element {
        string section;
        string key;
        string val;
        string comment;
        unsigned access_idx;
        };

    vector < vector <_element> > m_data;


    unsigned m_find_by_key(const string &sect,const string &key);
    unsigned m_find_by_idx(const string &sect,unsigned access_idx);


    void    m_insert_val_by_key(const string &sect,const string &key,const string &val,const string &comment);
    void    m_insert_val_by_idx(const string &sect,unsigned access_idx,const string &val,const string &comment);

    string  m_get_val_by_key(const string &sect,const string &key,unsigned idx_par);
    string  m_get_val_by_idx(const string &sect,unsigned access_idx,unsigned idx_par);


    string  m_get_comment(string s);
    void    m_removeComment(string &s);

    bool    m_file_read(const string &fn);
    bool    m_file_read_include(const string &in_line);
    bool    m_file_write(const string &fn);
    void    m_parseString();

    string  m_get_section(const string &line);
    string  m_get_key(const string &line);
    string  m_get_val(const string &line);
    void    m_parse_val(const string &section,const string &key,unsigned access_idx,const string &val,const string &comment);

    string  m_insert_comment(const string &str, const string &comment);
    string  m_insert_to_pos(const string &str,unsigned pos,const string &val);

    string m_removeSpaces(string &str) { str.erase(std::remove(str.begin(), str.end(), ' '), str.end()); return str; }

    string m_toUpper(string &str) { std::transform(str.begin(), str.end(), str.begin(), ::toupper); return str; }
    string m_toLower(string &str) { std::transform(str.begin(), str.end(), str.begin(), ::tolower); return str; }

    void ltrim(string &s) { s.erase(s.begin(), std::find_if(s.begin(), s.end(),std::not1(std::ptr_fun<int, int>(std::isspace)))); }
    void rtrim(string &s) { s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end()); }
    void trim(string &s) { ltrim(s); rtrim(s); }

public:

    void Clear() { m_data.clear(); }
    bool Read(string fn);
    bool Write(string fn);

    void GenerateConfig();

    // called from Switch
    void SetCommentIdx(const string &section,unsigned access_idx,const string &comment);


    void Print() {
        std::cout << "Ini table:" << std::endl;
        for (unsigned i=0;i<m_data.size();++i) {
            _element *p = &m_data[i][0];
            std::cout  << "EL: s:" << p->section  << " k:" << p->key  << " v:" << p->val  << " idx:" << p->access_idx;

            for (unsigned j=1;j<m_data[i].size();j++) {
               p = &m_data[i][j];
               std::cout  << " p:" << p->val;
               }
            std::cout  << std::endl;

        }
    }


   unsigned Count() { return m_data.size(); }
    //size_t GetCountSection() { return mData.size(); }
    //size_t GetCountKeys(string section)  { return mData[section].size(); }


   // BASE FUNC

    string GetStr(const string & section, const string & key,const string & default_value, unsigned idx_par=0);
    string GetStrIdx(const string &section, unsigned access_idx,const string &default_value, unsigned idx_par=0);


    string  GetRawStr(const string & section, const string & key);

    int GetInt(const string & section, const string & key, int default_value, unsigned idx_par=0);
    int GetIntIdx(const string & section, unsigned idx_val, int default_value, unsigned idx_par=0);


    unsigned GetUInt(const string & section, const string & key, unsigned default_value,unsigned idx_par=0);
    unsigned GetUIntIdx(const string & section, unsigned idx_val, unsigned default_value,unsigned idx_par=0);

    bool GetBoolean(const string & section, const string & key,bool default_value,unsigned idx_par=0);
    bool GetBooleanIdx(const string & section, unsigned idx_val,bool default_value,unsigned idx_par=0);

    TIPAddress GetIP(const string & section, const string & key,unsigned idx_par=0);
    TIPAddress GetIPIdx(const string & section, unsigned idx_val,unsigned idx_par=0);

};

extern  TIni Ini;
