#include "tglobal.h"
#include "tlog.h"

#include <sys/utsname.h>

TGlobal g;


void TGlobal::Initialize(const char *name_program)
{
    char buf[PATH_MAX + 1] {}; // not sure about the "+ 1"
    string fullpath  = realpath(name_program, &buf[0]);
    size_t pos       = fullpath.find_last_of("/");
    m_current_folder = fullpath.substr(0,pos) ;
    m_current_name   = fullpath.substr(pos+1);
    m_current_ini    = m_current_name + ".ini";
}

void TGlobal::OutSystemInfo()
{
    OutListInterfaces();

    // ------------------ out system info
    struct utsname info {};
    if (uname(&info)==0) {
        Log.ScrFile("found os=%s,kernel=%s\n",&info.sysname[0],&info.release[0]);
        Log.ScrFile("found version=%s,hw=%s\n",&info.version[0],&info.machine[0]);
        return;
        }
    Log.ScrFile("sysinfo not found\n");
    // ------------------

    OutCPUName();
}


//return cpu model name
void TGlobal::OutCPUName()
{
    ifstream finfo("/proc/cpuinfo");

    int cores = GetCPUCores();
    string line;
    while(getline(finfo,line)) {
        stringstream str(line);
        string itype;
        string info;
        if ( getline( str, itype, ':' ) && getline(str,info) && itype.substr(0,10) == "model name" ) {
           Log.ScrFile("found cpu = %s, %d cores\n",info.c_str(),cores);
           return;
           }
        }
    Log.ScrFile("found cpu =undefined, %d cores\n",cores);
}

//return 1...n or 0
int TGlobal::GetCPUCores()
{
    cpu_set_t set;
    CPU_ZERO(&set);
    if (sched_getaffinity(0,sizeof(cpu_set_t),&set)<0)
        return 0;

    int cpu_count=0;
    for(int i=0;i<CPU_SETSIZE;i++) {
        if (CPU_ISSET(i,&set)) ++cpu_count;
        }
    return cpu_count;
}

void TGlobal::OutListInterfaces()
{
    int sck = socket(AF_INET, SOCK_DGRAM, 0);
    if(sck < 0) return;

    struct ifreq    ifr[48]; //max 48 interfaces
    struct ifconf   ifconf {};
    ifconf.ifc_len = sizeof(ifr);
    ifconf.ifc_req = ifr;

    if(ioctl(sck, SIOCGIFCONF, &ifconf) < 0)
        return;

    close(sck);

    unsigned n = unsigned(ifconf.ifc_len) / sizeof(struct ifreq);

    for(unsigned i = 0; i < n; i++)  {
        struct ifreq *p = &ifr[i];
        unsigned  int idx = if_nametoindex(p->ifr_name);
        //if (idx==1) continue;  //skip 127.0.0.1
        Log.ScrFile("found netif %s: ip %s  idx=%u\n",p->ifr_name,inet_ntoa(((struct sockaddr_in *)&p->ifr_addr)->sin_addr),idx);
        }
}

// arg:"enp1s0"
TIPAddress TGlobal::GetInterfaceIp(const char *name_if)
{
    if (!name_if) return TIPAddress();

//Log.Scr("Find %s\n",name_if);
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct ifreq ifr {};
    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name , name_if , IFNAMSIZ-1);

    int ret = ioctl(fd, SIOCGIFADDR, &ifr);
    close(fd);
    if (ret<0) {
        //perror("xxx");
        return TIPAddress();
        }
    TIPAddress ip = inet_ntoa(( (struct sockaddr_in *)&ifr.ifr_addr )->sin_addr);
    return ip;
}

unsigned TGlobal::GetInterfaceIndex(const char * addr)
{
    in_addr ia {};
    if (inet_aton(addr,&ia)==0)  //bad input addr
        return 0;

    int sck = socket(AF_INET, SOCK_DGRAM, 0);
    if(sck < 0) return 0;

    struct  ifreq ifr[48]; //max 48 interfaces
    struct ifconf ifconf {};
    struct sockaddr_in *sin;

    ifconf.ifc_len = sizeof(ifr);
    ifconf.ifc_req = ifr;

    if(ioctl(sck, SIOCGIFCONF, &ifconf) < 0)
        return 0;

    close(sck);

    unsigned n = unsigned(ifconf.ifc_len) / sizeof(struct ifreq);

    for(unsigned i = 0; i < n; i++)  {
        sin = (struct sockaddr_in*)(&ifr[i].ifr_addr);
        if(sin->sin_addr.s_addr == ia.s_addr) {
                struct ifreq *p = &ifr[i];
                unsigned  int idx = if_nametoindex(p->ifr_name);
                return idx;
                }
        }
    return 0; //not found
}

string TGlobal::GetInterfaceName(const char * addr)
{
    in_addr ia {};
    if (inet_aton(addr,&ia)==0)  //bad input addr
        return "";

    int sck = socket(AF_INET, SOCK_DGRAM, 0);
    if(sck < 0) return "";

    struct  ifreq ifr[48]; //max 48 interfaces
    struct ifconf ifconf {};
    struct sockaddr_in *sin;

    ifconf.ifc_len = sizeof(ifr);
    ifconf.ifc_req = ifr;

    if(ioctl(sck, SIOCGIFCONF, &ifconf) < 0)
        return "";

    close(sck);

    unsigned n = unsigned(ifconf.ifc_len) / sizeof(struct ifreq);

    for(unsigned i = 0; i < n; i++)  {
        sin = (struct sockaddr_in*)(&ifr[i].ifr_addr);
        if(sin->sin_addr.s_addr == ia.s_addr) {
                struct ifreq *p = &ifr[i];
                string ret = p->ifr_name;
                return ret;
                }
        }
    return ""; //not found
}

int TGlobal::GetInterfaceIrq(const char * addr)
{
    string name =  GetInterfaceName(addr);
    if (name.empty()) return 0;

    string line;
    ifstream finfo("/proc/interrupts");
    while(getline(finfo,line)) {
        stringstream str(line);
        if (line.find(name) < line.size()) {
            string itype;
            getline( str, itype, ':' );
            return stoi(itype);
            }
        }
    return 0;
}

bool TGlobal::ExistsInterface(TIPAddress addr)
{
    string sys_name_if = GetInterfaceName(addr.c_str());
    if (sys_name_if.empty())
        return false;
    return true;
}

bool TGlobal::GetNameIf_IpIf(const char *inp, string &out_name_if, TIPAddress &out_ip_if)
{
    string sif    = inp;
    string name   = GetInterfaceName(sif.c_str());
    TIPAddress ip = GetInterfaceIp(sif.c_str());

    if (ip.Bad()) {
        // name ?
        ip = GetInterfaceIp(name.c_str());
        }
    else {
        // ip ?
        name = GetInterfaceName(ip.c_str());
        }

    if (!ExistsInterface(ip))
        return false;

    out_name_if = name;
    out_ip_if   = ip;
    return true;
}







