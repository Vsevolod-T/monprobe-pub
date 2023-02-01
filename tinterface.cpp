
#include "tinterface.h"

#include "tini.h"
#include "tlog.h"

#include "tglobal.h"

vector <TInterface *>     pInterfacesArray;

void thread_recieve_af(TInterface *arg);




TInterface *FindInterface(TIPAddress ip)
{
    for (auto & p : pInterfacesArray) {
        if (p->ip_if == ip) return p;
        }
    return nullptr;
}


void TInterface::Run()
{
    char buff[64] {};

    string ssrc;

     //ssrc = "/sys/class/net/" + name_sys + "/statistics/tx_bytes";
     ssrc = "/sys/class/net/" + name_sys + "/statistics/rx_bytes";

    FILE *fp=fopen(ssrc.c_str(),"r");
    if (!fp)  return;

    if (!fgets(&buff[0],sizeof(buff),fp)) {
        fclose(fp);
        return;
        }
    fclose(fp);

    sscanf(&buff[0],"%llu",&bytes_curr);

    if (flag_first) {
        bytes_prev  = bytes_curr;
        traffic     = 0.0;
        flag_first  = false;
        }

    if (bytes_curr < bytes_prev) {
        bytes_prev = bytes_curr;
        return;
        }

    double new_traffic = double ((bytes_curr - bytes_prev) * 8) /1024.0/1024.0;
    bytes_prev = bytes_curr;

    const double coeff_flatness = 9.0;
    traffic = (coeff_flatness * traffic + new_traffic) / (coeff_flatness + 1.0);
}

void TInterface::ThreadStart()
{
    af_mode = g.AF_Mode();

    is_running  = 0;
    is_error      = 0;
    th_running = 1;
    err_message = "";

    std_thread = thread(thread_recieve_af,this);

   while (true) {
       if (is_running) break;
       if (is_error)     break;
       usleep(1000);
       }

if (is_error) {
    is_running = 0;
    //is_error = 0;
    th_running = 0;
    Log.ScrFile("\nError create interface %s thread => %s \n",ip_if.c_str(),err_message.c_str());
    }
if (is_running) {

    Log.ScrFile("create interface %s thread ok\n",ip_if.c_str());

   // if (!g.RootPermission() && cpu_core ) {
   //     Log.ScrFile("interface %s cpu=%d affinity ignore (root permission required)\n",ip_if.c_str(),cpu_core);
   //     }
     }

}




void TInterface::ThreadStop()
{
    th_running = 0;

   //Log.ScrFile("cmd ThreadStop interface %s\n",ip_if.c_str());
    std_thread.join();
}



