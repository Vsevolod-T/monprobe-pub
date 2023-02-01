#include "tini.h"

TIni Ini;



unsigned TIni::m_find_by_key(const string &sect, const string &key)
{
    unsigned sz=m_data.size();
    for (unsigned i=0; i<sz;++i) {

        _element *p = &m_data[i][0];
        if (p->section == sect && p->key==key)
            return i;
    }
    return BADINDEX;
}

unsigned TIni::m_find_by_idx(const string &sect,unsigned access_idx)
{
    if (access_idx == BADINDEX ) return BADINDEX;
    unsigned sz=m_data.size();

    for (unsigned i=0; i<sz;++i) {

        _element *p = &m_data[i][0];
        if (p->section == sect && p->access_idx==access_idx)
            return i;
    }
    return BADINDEX;
}


void TIni::m_insert_val_by_key(const string &sect, const string &key, const string &val, const string &comment)
{
    if (sect.empty() && key.empty()) {
        // comment
        size_t sz = m_data.size();
        m_data.resize(sz+1);
        m_data[sz].resize(1);
        _element *p= &m_data[sz][0];
        p->section=sect;
        p->key="";
        p->val="";
        p->access_idx=BADINDEX;
        p->comment=comment;
        return;
    }

    unsigned idx = m_find_by_key(sect,key);

//std::cout  << "m_insert_val_by_key s:" << sect  << " key:" << key << " v:" << val << " c:" << comment <<  " idx:" << idx << std::endl;
    if (idx == BADINDEX)  {
        // new key
        size_t sz = m_data.size();
        m_data.resize(sz+1);
        m_data[sz].resize(1);
        _element *p= &m_data[sz][0];
        p->section=sect;
        p->key=key;
        p->val=val;
        p->access_idx=BADINDEX;
        p->comment=comment;
        return;
        }

    // new par
    size_t sz = m_data[idx].size();
    m_data[idx].resize(sz+1);
    _element *p= &m_data[idx][sz];
    p->section=sect;
    p->key=key;
    p->val=val;
    p->access_idx=BADINDEX;
    p->comment="";//comment;
}

void TIni::m_insert_val_by_idx(const string &sect,unsigned access_idx,const string &val, const string &comment)
{
    unsigned idx = m_find_by_idx(sect,access_idx);

    if (idx == BADINDEX)  {
        // new key
        size_t sz = m_data.size();
        m_data.resize(sz+1);
        m_data[sz].resize(1);
        _element *p= &m_data[sz][0];
        p->section=sect;
        p->key="";
        p->val=val;
        p->access_idx=access_idx;
        p->comment=comment;
//std::cout  << "m_insert_val_by_idx F s:" << sect << " v:" << val << " c:" << comment <<  " idx:" << idx << std::endl;
        return;
        }

    // new par
    size_t sz = m_data[idx].size();
    m_data[idx].resize(sz+1);
    _element *p= &m_data[idx][sz];
    p->section="";
    p->key="";
    p->val=val;
    p->access_idx=access_idx;
    p->comment="";//comment;
//std::cout  << "m_insert_val_by_idx N s:" << sect << " v:" << val << " c:" << comment <<  " idx:" << idx << std::endl;
}

string TIni::m_get_val_by_key(const string &sect, const string &key,unsigned idx_par)
{
    unsigned idx = m_find_by_key(sect,key);
    if (idx == BADINDEX) return "";

    if (idx_par < m_data[idx].size())
        return m_data[idx][idx_par].val;

    return "";
}

string TIni::m_get_val_by_idx(const string &sect, unsigned access_idx, unsigned idx_par)
{
    unsigned idx=m_find_by_idx(sect,access_idx);
    if (idx == BADINDEX) return "";

    if (idx_par < m_data[idx].size())
        return m_data[idx][idx_par].val;

    return "";
}

bool TIni::m_file_read(const string &fn)
{

    // read full cfg file to strings

    std::string line;
    ifstream infile(fn);

    // Error opening input file
    if(!infile)  return false;

    while (std::getline(infile, line)) {
        ltrim(line);
        rtrim(line);
        if(line.empty()) continue;

        // find include=xxx
        if (m_file_read_include(line))
            continue;
        strings.push_back(line);
        }

    m_parseString();

    strings.clear();
    strings.shrink_to_fit();

    m_data.shrink_to_fit();

    return true;
}

bool TIni::m_file_read_include(const string &in_line)
{
    size_t pos   = in_line.find_last_of("=");
    string cmd   = in_line.substr(0,pos) ;
    string fname = in_line.substr(pos+1);

    // skip if not include
    if (cmd!="include")
        return false;

    //printf("cmd=%s  arg=%s\n",cmd.c_str(),fname.c_str());

    std::string line;
    ifstream infile(fname);

    // Error opening input file
    if(!infile)  {
        printf("NO include file %s\n",fname.c_str());
        line = "# no include -> " + fname;
        strings.push_back(line);
        return true;
        }

    line = "# ---------- " + fname;
    strings.push_back(line);
    while (std::getline(infile, line)) {
        ltrim(line);
        rtrim(line);
        if(line.empty()) continue;
        strings.push_back(line);
        }
    line = "# ---------- ";
    strings.push_back(line);

    return true;
}



string TIni::m_insert_comment(const string &str, const string &comment)
{
    if (comment.empty()) return str;
    if (str.empty()) { return comment;}

    const size_t pos_comment = 32;
    size_t sz_orig = str.size();
    size_t sz_add=0;

    if  (sz_orig < pos_comment)
           sz_add =  pos_comment - sz_orig;

    string str_add(sz_add,' ');
    return str + str_add + comment;
}

string TIni::m_insert_to_pos(const string &str, unsigned pos, const string &val)
{
    wchar_t buff[512] {};
    std::mbstowcs(&buff[0],str.c_str(),str.size());
    wstring s=buff;

    size_t sz_add=0;

    if  (s.size() < pos)
        sz_add =  pos - s.size();

    string str_add(sz_add,' ');
    return str + str_add + val;
}


bool TIni::m_file_write(const string &fn)
{
    string current_line;
    string section;
    string key;
    string out_str;

    for (size_t i=0;i<m_data.size();++i)
        {
        current_line = "";

        _element *p = &m_data[i][0];

        // comment line ?
        if (p->section.empty() && p->key.empty()) {

            current_line = m_insert_comment(current_line,p->comment);
            out_str += current_line += "\n";
            continue;
            }

        // check new section
        if (section != p->section) {

            if (p->section.empty()) continue;

            section =  p->section;
            current_line  =  "[" +  section  + "]";

            current_line = m_insert_comment(current_line,p->comment);
            out_str += current_line += "\n";
            continue;
            }

         // check key
        if (!p->key.empty()) {
            // key -> access_key
            key =  p->key;
            current_line  +=  key  + "=";
            }
        else {
            // no key -> access_idx
            key="";
            }

        // write all par
        for (size_t j=0;j<m_data[i].size();++j) {

            _element *par = &m_data[i][j];

            if (section == SECTION_INP_PAR)
            {
                if (j==0) current_line = m_insert_to_pos(current_line, 0,par->val); // name
                if (j==1) current_line = m_insert_to_pos(current_line,10,par->val); // offset
                if (j==2) current_line = m_insert_to_pos(current_line,15,par->val); // min
                if (j==3) current_line = m_insert_to_pos(current_line,20,par->val); // max
                if (j==4) current_line = m_insert_to_pos(current_line,25,par->val); // scr
                if (j==5) current_line = m_insert_to_pos(current_line,30,par->val); // cc
                continue;
            }


            if (section.substr(0,5) == "IPINP")
            {
                if (key == "interface") {
                    if (j==0) current_line += par->val;        // ip
                    if (j==1) current_line += " " + par->val;  // name
                    continue;
                }

                if (m_data[i][0].val!=".") {
                if (j==0) current_line = m_insert_to_pos(current_line, 0,par->val); // ip
                if (j==1) current_line = m_insert_to_pos(current_line,16,par->val); // port
                if (j==2) current_line = m_insert_to_pos(current_line,22,par->val); // name
                if (j>2)  current_line = m_insert_to_pos(current_line,40 + j*6,par->val); // pmt ....
                continue;
                }
            //else {
                if (j==0) current_line = m_insert_to_pos(current_line,0,par->val); // .
                if (j==1) current_line = m_insert_to_pos(current_line,2,par->val); // name
                if (j==2) current_line = m_insert_to_pos(current_line,37,par->val); // pmt
                if (j>2)  current_line = m_insert_to_pos(current_line,27 + j*6,par->val); // ....
                continue;
             //   }
            }

            if (section.substr(0,6)=="SWITCH" && key.empty()) {
                if (j==0) current_line = m_insert_to_pos(current_line, 0,par->val); // ip
                if (j==1) current_line = m_insert_to_pos(current_line,16,par->val); // port
                continue;
            }

// std::cout  << "test s:" << section << " k:" << key << " v:" << par->val  << " idx:" << p->access_idx << " sz:" << m_data[i].size() << std::endl;

            if (j) current_line += " ";
            current_line += par->val + " ";
            } // PAR

        current_line = m_insert_comment(current_line,p->comment);
        out_str += current_line + "\n";
        } // for string

    out_str += "\n";

    //std::cout  << "OUT:\n" << out_str << std::endl;
    std::ofstream out;
    out.open(fn,ios_base::out);
    out.imbue(locale("ru_RU.utf8"));
    out << out_str;
    out.close();

    return true;
}

string TIni::m_get_comment(string s)
{
    std::string::size_type n;
    n = s.find("#");  // поиск с начала строки
    if (n == std::string::npos)
        return "";
    s = s.substr(n,s.size());
    rtrim(s);
    ltrim(s);
    return s;
}

void TIni::m_removeComment(string &s)
{
    std::string::size_type n;
    n = s.find("#");  // поиск с начала строки
    if (n == std::string::npos)
        return;
    s = s.substr(0,n);
    rtrim(s);
}

string TIni::m_get_section(const string &line)
{
    string tline = line;

    std::string::size_type pos_start;
    std::string::size_type pos_end;
    pos_start = tline.find("[");  // поиск
    pos_end = tline.find("]");   // поиск
    if (pos_start ==- string::npos || pos_end== string::npos)
        return "";

    tline = tline.substr(pos_start+1,(pos_end-pos_start)-1);
    tline=m_removeSpaces(tline);
    tline=m_toUpper(tline);

    return tline;
}

string TIni::m_get_key(const string &line)
{
    string tline = line;

    std::string::size_type pos_end;
    pos_end = tline.find("=");   // поиск
    if (pos_end== string::npos)
        return "";

    tline = tline.substr(0,(pos_end));
    tline=m_removeSpaces(tline);
    tline=m_toLower(tline);

    return tline;
}

string TIni::m_get_val(const string &line)
{
    string tline = line;

    std::string::size_type pos_start;
    pos_start = tline.find("=");   // поиск
    if (pos_start== string::npos)
        return tline;

    tline = tline.substr(pos_start+1);

    return tline;
}

void TIni::m_parseString()
{
    unsigned current_str=0;
    string section;
    unsigned access_idx=0;

    while(current_str < strings.size()){

        string in_str = strings[current_str++];

        if (in_str.empty()) continue;

        string comment = m_get_comment(in_str);
        m_removeComment(in_str);

        if (in_str.empty()) {

            // only comment
            m_insert_val_by_key("","","",comment);
            continue;
            }

        //-----------------
        string sect =  m_get_section(in_str);

        // new section ?
        if (!sect.empty()) {
            section = sect;
            m_insert_val_by_key(sect,"","",comment);
            access_idx=0;
            continue;
            }

        // skip all if no section
        if (section.empty())
            continue;

        // std::cout  << "Section=" << section << " idx_key=" << idx_key << " tok: " << tok << std::endl;

        string key =  m_get_key(in_str);
        string val;

        // found key ?
         if (!key.empty())
             val = m_get_val(in_str);
         else
             val=in_str;

           // found val ?
          if (val.empty())
              continue;

       //std::cout  << "XXX s:" << section << "  k:" << key << " access_idx:" << access_idx << " inp:" << in_str << std::endl;

       m_parse_val(section,key,access_idx,val,comment);

       if (key.empty())
          access_idx++;

      //-----------------
      } // while
}





void TIni::m_parse_val(const string &section,const string &key,unsigned access_idx,const string &val,const string &comment)
{
   //std::cout  << "Parse: s:" << section  << " k:" << key  << " v:" << val  << " access_idx:" << access_idx << std::endl;

    string par_val;

    const std::string m_string=val;
    size_t m_offset=0;
    const string m_delimiters=" \t\n\r";
    std::string m_token;

    while (true) {
            size_t i = m_string.find_first_not_of(m_delimiters, m_offset);
            if (std::string::npos == i)
            {
                m_offset = m_string.length();
                return;
            }

            size_t j = m_string.find_first_of(m_delimiters, i);
            if (std::string::npos == j)
            {
                m_token = m_string.substr(i);
                m_offset = m_string.length();
            }
            else {
                 m_token = m_string.substr(i, j - i);
                 m_offset = j;
            }

        par_val = m_token;

        if (!key.empty())
            m_insert_val_by_key(section,key,par_val,comment);
        else
            m_insert_val_by_idx(section,access_idx,par_val,comment);
        }
}


bool TIni::Read(string fn) { return m_file_read(fn); }
bool TIni::Write(string fn) { return m_file_write(fn); }

void TIni::GenerateConfig()
{
    return;
    /*
    m_insert_val_by_key("","","","#");
    m_insert_val_by_key("","","","# config mpeg ts analizer");
    m_insert_val_by_key("","","","#");

    m_insert_val_by_key("COMMON","","","# comment");
    m_insert_val_by_key("COMMON","af_mode","1","# comment");
    m_insert_val_by_key("COMMON","log_folder","./log","# comment");
    m_insert_val_by_key("COMMON","base_oid",".3.6.1.2.1.1.1.2.0.1","# comment");
    m_insert_val_by_key("COMMON","access_list2","-0.0.0.0/0,+127.0.0.1,+192.168.0.0/16,+10.10.72.0/24","# comment");

    m_insert_val_by_key("","","","# comment");
    m_insert_val_by_key("Switch1","","","# comment");
    m_insert_val_by_key("Switch1","eq8096","1 1 80","# comment");
    m_insert_val_by_idx("Switch1",0,"239.255.250.1 5510","# comment");
    m_insert_val_by_idx("Switch1",1,"239.255.40.21 5500","# comment");

    m_insert_val_by_key("","","","# comment");
    m_insert_val_by_key("Monitoring1","","","# comment");
    m_insert_val_by_key("Monitoring1","interface","17.16.16.10 Local0","# comment");
    m_insert_val_by_key("Monitoring1","cpu","2","# comment");

    m_insert_val_by_idx("Monitoring1",0,"239.255.250.1  5510  Первый_PTPC-1       80   82 500   83 25","# comment");
    m_insert_val_by_idx("Monitoring1",1,"239.255.72.139 5500 ПервыйHD_URFO-600  1184 1186 500 1187 25","# comment");

    m_insert_val_by_idx("Monitoring1",2,"235.72.207.1 2015 RTRS1","# comment");
    m_insert_val_by_idx("Monitoring1",3,". Первый   1010 1011 500 1012 25","# comment");
    m_insert_val_by_idx("Monitoring1",4,". Россия   1020 1021 500 1022 25","# comment");
    m_insert_val_by_idx("Monitoring1",5,". Нтв      1040 1041 500 1042 25","# comment");
    */
}

void TIni::SetCommentIdx(const string &section, unsigned access_idx, const string &comment)
{
    if (comment.empty()) return;
    string m_sect = section; m_toUpper(m_sect);

    unsigned idx = m_find_by_idx(m_sect,access_idx);
    if (idx == BADINDEX ) return;
     m_data[idx][0].comment= "# " + comment;
}

string TIni::GetStr(const string & section, const string & key,const string & default_value, unsigned idx_par)
{
    string m_sect = section; m_toUpper(m_sect);
    string m_key  = key;       m_toLower(m_key);
    string s = m_get_val_by_key(m_sect,m_key,idx_par);
    if (s.empty()) return default_value;
    return s;
}

string TIni::GetStrIdx(const string &section, unsigned access_idx, const string &default_value,  unsigned idx_par)
{
    string m_sect = section; m_toUpper(m_sect);
    string s = m_get_val_by_idx(m_sect,access_idx,idx_par);
    if (s.empty()) return default_value;
    return s;
}


string TIni::GetRawStr(const string & section, const string & key)
{
    string s;
    unsigned idx=0;
    while(true) {
        string val = GetStr(section,key,"",idx++);
        if (val.empty()) break;
        if (idx) s+= ",";
        s += val;
    }
    return s;
}


int TIni::GetInt(const string &section, const string &key, int default_value, unsigned idx_par)
{
    string s = GetStr(section,key,"",idx_par);
    if (s.empty()) return default_value;

    const char* value = s.c_str();
    char* end;
    // This parses "1234" (decimal) and also "0x4D2" (hex)
    int n = int(strtol(value, &end, 10));   // ? base 0
    return end > value ? n : default_value;
}

int TIni::GetIntIdx(const string &section, unsigned idx_val, int default_value, unsigned idx_par)
{
   string s = GetStrIdx(section,idx_val,"",idx_par);
    if (s.empty()) return default_value;

    const char* value = s.c_str();
    char* end;
    // This parses "1234" (decimal) and also "0x4D2" (hex)
    int n = int(strtol(value, &end, 10));   // ? base 0
    return end > value ? n : default_value;
}



unsigned TIni::GetUInt(const string &section, const string &key, unsigned default_value, unsigned idx_par)
{
    string s = GetStr(section,key,"",idx_par);
    if (s.empty()) return default_value;

    const char* value = s.c_str();
    char* end;
    // This parses "1234" (decimal) and also "0x4D2" (hex)
    unsigned n = unsigned(strtol(value, &end, 10));   // ? base 0
    return end > value ? n : default_value;
}

unsigned TIni::GetUIntIdx(const string &section, unsigned idx_val, unsigned default_value, unsigned idx_par)
{
   string s = GetStrIdx(section,idx_val,"",idx_par);
    if (s.empty()) return default_value;

    const char* value = s.c_str();
    char* end;
    // This parses "1234" (decimal) and also "0x4D2" (hex)
    unsigned n = unsigned(strtol(value, &end, 10));   // ? base 0
    return end > value ? n : default_value;
}


bool TIni::GetBoolean(const string &section, const string &key, bool default_value, unsigned idx_par)
{
    string s = GetStr(section,key,"",idx_par);
    if (s.empty()) return default_value;
    m_toLower(s);
    if (s == "true" || s == "yes" || s == "on" || s == "1")
        return true;
     if (s == "false" || s == "no" || s == "off" || s == "0")
        return false;
    return default_value;
}


bool TIni::GetBooleanIdx(const string &section, unsigned idx_val, bool default_value, unsigned idx_par)
{
     string s = GetStrIdx(section,idx_val,"",idx_par);
    if (s.empty()) return default_value;
    m_toLower(s);
    if (s == "true" || s == "yes" || s == "on" || s == "1")
        return true;
    if (s == "false" || s == "no" || s == "off" || s == "0")
        return false;
    return default_value;
}


TIPAddress TIni::GetIP(const string &section, const string &key, unsigned idx_par)
{
    string s = GetStr(section,key,"",idx_par);
    if (s.empty()) return INADDR_NONE;

    TIPAddress value=s.c_str();
    return value;
}

TIPAddress TIni::GetIPIdx(const string &section, unsigned idx_val, unsigned idx_par)
{
     string s = GetStrIdx(section,idx_val,"",idx_par);
    if (s.empty()) return INADDR_NONE;

    TIPAddress value=s.c_str();
    return value;
}





