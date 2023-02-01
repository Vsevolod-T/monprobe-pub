#pragma once

#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <deque>
#include <algorithm>
#include <atomic>
#include <thread>
#include <mutex>

#include <cstring>
#include <ctime>
#include <cstdlib>
#include <cstdio>
#include <cstdarg>
#include <cerrno>
#include <random>

#include <unistd.h>
#include <resolv.h>   // PATH_MAX
#include <fcntl.h>
#include <byteswap.h>

#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>

using namespace std;

static const char  SERVER_VERSION[]  = "p11";

static const char  LOGIN_STATISTIC[] = "stat";
static const char  PASSW_STATISTIC[] = "stat";



struct _mpeg_ts_header {
    unsigned    sync;           //  : 8;    синхробайт = 0x47
    unsigned    error;          //  : 1;    ошибка транспорта
    unsigned    start_section;  //  : 1;    начало данных (начало section!)
    unsigned    priority;       //  : 1;    приоритетный пакет
    unsigned    pid;            //  : 13;   Packet Identifierpes->bytes
    unsigned    scrambled;      //  : 2;    '00' = Not scrambled
    unsigned    adaptation;     //  : 1;    содержит Adaptation field (при PAT,PMT...)
    unsigned    payload;        //  : 1;    содержит данные
    unsigned    counter;        //  : 4;    увеличивается при наличии данных 0x0 - 0xf
    };

// ****************************************** low level
// get 8 , byte pos+=1
uint8_t get8(uint_fast8_t *&position);
// get 16 , byte pos+=2
uint16_t get16(uint_fast8_t *&position);


enum es_type : unsigned
{
    //  (channel) common
    PAT     = 0x0000,
    CAT     = 0x0001,
    TSDT    = 0x0002,
    IPMP    = 0x0003,
    NIT     = 0x0010,
    SDT     = 0x0011,
    EIT     = 0x0012,
    RST     = 0x0013,
    TDT     = 0x0014,

    STUFF   = 0x1fff,

    // (channel)
    PMT     = 0x3000,

    VIDEO   = 0x3001,
    AUDIO   = 0x3002,
    VBI   = 0x3003,
    DATA   = 0x3004,

    // (analizer)
    EMM     = 0x2000,
    ECM     = 0x2001,

    UNDEF  = 0xffff
};




// RAW ES TYPE
enum es_stream : unsigned
{
    STREAM_TYPE_UNKNOWN = 0,
    STREAM_TYPE_VIDEO_MPEG1,
    STREAM_TYPE_VIDEO_MPEG2,
    STREAM_TYPE_AUDIO_MPEG1,
    STREAM_TYPE_AUDIO_MPEG2,
    STREAM_TYPE_AUDIO_AAC,
    STREAM_TYPE_AUDIO_AAC_ADTS,
    STREAM_TYPE_AUDIO_AAC_LATM,
    STREAM_TYPE_VIDEO_H264,
    STREAM_TYPE_VIDEO_HEVC,
    STREAM_TYPE_AUDIO_AC3,
    STREAM_TYPE_AUDIO_EAC3,
    STREAM_TYPE_DVB_TELETEXT,
    STREAM_TYPE_DVB_SUBTITLE,
    STREAM_TYPE_VIDEO_MPEG4,
    STREAM_TYPE_VIDEO_VC1,
    STREAM_TYPE_AUDIO_LPCM,
    STREAM_TYPE_AUDIO_DTS,
    STREAM_TYPE_PRIVATE_DATA
};

struct _stream_info
{
  es_type type;
  es_stream type_stream;

  char     language[4];

  int         SampleRate;
  int         Channels;
  int         BitRate;
  int mpeg;
  int layer;
  int mode;

  int Heigh;
  int Width;
  int aspect;
  int frate;
};


class TIPAddress
{
    char m_saddr[(3+1)*4+1] {0};
    uint32_t m_addr = INADDR_NONE;

    const char *get_addr_() {
        auto *p = reinterpret_cast<unsigned char *>(&m_addr);
        memset(m_saddr,0,sizeof(m_saddr));
        snprintf(m_saddr, sizeof(m_saddr), "%d.%d.%d.%d", p[0], p[1], p[2], p[3]);
        return m_saddr;
        }

    void set_addr_(const char *s) {
        in_addr addr;
        if (inet_aton(s, &addr)==0)
            addr.s_addr=INADDR_NONE;
        m_addr = addr.s_addr;
        }

public:
    TIPAddress(uint32_t addr=INADDR_NONE) { m_addr = htonl(addr);  }
    TIPAddress(const char *s)         { set_addr_(s); }

    uint32_t bin()                    { return ntohl(m_addr); }
    const char *c_str()               { return get_addr_(); }

    bool Bad()                                  { return m_addr == INADDR_NONE;  }

    bool operator == (const TIPAddress &other) 	{ return m_addr == other.m_addr; }
    bool operator != (const TIPAddress &other) 	{ return m_addr != other.m_addr; }

    // оператор присваивания
    TIPAddress& operator = (const TIPAddress &other) {
       if (this == &other) return *this;
       m_addr = other.m_addr;
       return *this;
       }

    TIPAddress(const TIPAddress&) = default;
    TIPAddress(TIPAddress&&) = default;
    //TIPAddress& operator=(const TIPAddress&) = default;
    TIPAddress& operator=(TIPAddress&&) = default;
};



class TIPPort
{
    char m_sport[5+1] {0};
    uint16_t m_port = 0;

    const char *get_port_() {
        memset(m_sport,0,sizeof(m_sport));
        snprintf(m_sport, sizeof(m_sport), "%d",m_port);
        return m_sport;
        }

public:
    TIPPort(uint16_t port=0)  { m_port = port; }

    uint16_t bin()          { return m_port; }
    const char *c_str() { return get_port_(); }

    bool Bad()              { return bool(m_port == 0); }

    bool operator == (const TIPPort &other) { return m_port == other.m_port; }
    bool operator != (const TIPPort &other) { return m_port != other.m_port; }

    TIPPort& operator = (const TIPPort &other) {
       if (this == &other) return *this;
       m_port = other.m_port;
       return *this;
       }

    TIPPort(const TIPPort&) = default;
    TIPPort(TIPPort&&) = default;
    //TIPPort& operator=(const TIPPort&) = default;
    TIPPort& operator=(TIPPort&&) = default;

};


class  TPacket
{
public:
    uint8_t     *ptr;  //data
    unsigned    len;   //

    // from
    TIPAddress  ip_if;
    TIPAddress  ip;
    TIPPort     port;
    TIPAddress  ip_src;
};


class  TInfo
{
     string  m_section;
     string  m_name_if;
     string  m_ts_name;
     TIPAddress m_ts_ip_if;
     TIPAddress m_ts_ip;
     TIPPort m_ts_port;

public:

      TInfo() { }

      void SetNameIF(const string v) { m_name_if = v; }
      const string&  NameIF()    { return m_name_if;  }

      void SetSection(const string v) { m_section = v; }
      void SetName(const string v) { m_ts_name = v; }
      void SetIF(const TIPAddress v) {m_ts_ip_if = v; }
      void SetIP(const TIPAddress v) {m_ts_ip = v; }
      void SetPort(const TIPPort v) {m_ts_port = v; }

      const string&  Section() { return m_section;  }
      const string&  Name()    { return m_ts_name;  }

      TIPAddress&   IF()          { return m_ts_ip_if; }
      TIPAddress&   IP()         { return m_ts_ip;   }
      TIPPort&      Port()      { return m_ts_port; }

};








//************************************************************************************************* MPTS







static const string STUB_STRING = "";


static const double SYSTEM_CLOCK_FREQUENCY  = 27000000.0;



// -------------- POOL RawTS , AnalizedTS -----------------
static const unsigned POOL_ES_UNUSED = 0xffff;
static const unsigned POOL_ES_COMMON = 1;
static const unsigned POOL_ES_UNEXPECTED = 2;
static const unsigned POOL_ES_STUFF = 3;
//static const unsigned POOL_ES_PMT = 4;
static const unsigned POOL_ES_DATA = 4;

// --------------TChannel & TAnalized ES & TS -----------------

static const unsigned MAX_PES_SIZE        = 128;  // size si & pes headers
static const unsigned MAX_ES_TEXT_LEN     = 64;   //len Provider & serviceName

static const unsigned MAX_INTERFACES      = 16;
static const unsigned MAX_WSCONNECTIONS   = 16 ;    // up to 16 independent client connections


//TAnalizedESState
enum es_ui_state : unsigned
{
UI_STREAM_NO      = 0,
UI_STREAM_LOST    = 1,
UI_STREAM_RESTORE = 2,
UI_STREAM_OK      = 3,
UI_UNCHECKED      = 4,

UI_OUT_LOCKED       = 5,
UI_QUALITY        = 6,

UI_MAX_VALUE  = 4
};

//************************************************************************************************* MPTS

static const string SECTION_INP_PAR  = "INP_PAR";
static const string SECTION_INP = "INP";




struct _es_timeouts {
    unsigned    time_to_off;          // time to off
    unsigned    time_to_on;    // time to on
};


static const uint16_t DROP_PID = 0x1fff;


struct _statistic {

    unsigned  second_bad_traffic;   // second low traffic
    unsigned  second_ccerror;       // second if cc
    unsigned  summary_bytes;        // traffic
    unsigned  summary_ccerrors;     // ccerrors in second

    _statistic() {
        second_bad_traffic =
        second_ccerror     =
        summary_bytes      =
        summary_ccerrors   = 0;
    }

    void reset() {
        second_bad_traffic =
        second_ccerror     =
        summary_bytes      =
        summary_ccerrors   = 0;
        }

    _statistic & operator += (const _statistic &other) {
        if (this==&other) return *this;
        second_bad_traffic += other.second_bad_traffic;
        second_ccerror      += other.second_ccerror;
        summary_bytes     += other.summary_bytes;
        summary_ccerrors += other.summary_ccerrors;
        return *this;
        }

    _statistic(const _statistic&) = default;
    _statistic(_statistic&&) = default;
    _statistic& operator=(const _statistic&) = default;
    _statistic& operator=(_statistic&&) = default;
};





//************************************************************************************************* MPTS
void PrintX(uint8_t *p,unsigned N);
void PrintX(const uint8_t *p,unsigned N);


int         BindThisThreadToCore(int core_id);
int         BindIrqToCore(int core_id,int irq);
int         SetNonBlocking(int soc);

double      format_double(double value);

string      format_traffic(unsigned bytes);
string      format_02f(double val);
string      Transliterate(const string& text);

[[ noreturn ]] void ExitWithError(const char*msg);

uint8_t     crc8(uint8_t const *buffer, size_t len);
uint16_t    crc16(uint16_t crc, uint8_t const *buffer, size_t len);
uint32_t    crc32 (uint8_t const *buffer, size_t len);


/*
measure timer example
    {
    // TTimer<std::chrono::seconds> timer;
    TTimer<std::chrono::milliseconds> timer;
    // TTimer<std::chrono::microseconds> timer;
    // TTimer<std::chrono::nanoseconds> timer;

    // function or code block here

    timer.stop();
    }


template<class Resolution = std::chrono::milliseconds>
class TTimer {
public:
    using Clock = std::conditional_t<std::chrono::high_resolution_clock::is_steady,
                                     std::chrono::high_resolution_clock,
                                     std::chrono::steady_clock>;
private:
    const Clock::time_point mStart = Clock::now();

public:
    TTimer() = default;
    ~TTimer() {
        //const auto end = Clock::now();
        //std::ostringstream strStream;
        //strStream << "Destructor Elapsed: " << std::chrono::duration_cast<Resolution>( end - mStart ).count() << std::endl;
        //std::cout << strStream.str() << std::endl;
    }

    inline void stop() {
        const auto end = Clock::now();
        std::ostringstream strStream;
        strStream << "Stop Elapsed: " << std::chrono::duration_cast<Resolution>(end - mStart).count();// << std::endl;
        std::cout << strStream.str() << std::endl;
    }

    inline void stop(string text) {
        const auto end = Clock::now();
        std::ostringstream strStream;
        strStream << text << " execute: " << std::chrono::duration_cast<Resolution>(end - mStart).count();// << std::endl;
        std::cout << strStream.str() << std::endl;

    }

}; // ExecutionTimer
*/
