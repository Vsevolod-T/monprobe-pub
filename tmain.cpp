#include "tmain.h"
#include "tini.h"
#include "tlog.h"
#include "tauth.h"
#include "thttp.h"
#include "tinterface.h"
#include "tchannel.h"
#include "tanalizerts.h"

#include "twsconnection.h"
#include "tui.h"

#include "thread_rcv_af.h"

#include "tstatistic.h"
#include "tconfig.h"

#include "tchannelmeasure.h"

#include "ttime.h"



// ******************************************************************************************
void CreateChannelsIn(unsigned second_alive)
{
    unsigned count_ip_inp = Config.CountIP();

   //Log.ScrFile("CreateChannelsIn count_ip_inp =>  %u\n",count_ip_inp);

    for( unsigned idx_ip=0; idx_ip < count_ip_inp;++idx_ip) {

        TChannel *pChannel = new (nothrow) TChannel;
        if (!pChannel) ExitWithError("No memory for Channels");
        pChannelsArray.push_back(pChannel);

        _checked *param = Config.GetIP(idx_ip);

        // ------------------------------------------------
        // name_if <-> ip_if

        string sif  = param->name_if;
        bool rezult = g.GetNameIf_IpIf(sif.c_str(),param->name_if,param->ip_if);
        if (!rezult) {
            Log.ScrFile("err: section [%s] not exists interface %s\n",param->section.c_str(),sif.c_str());
            exit(0);
        }

        //Log.ScrFile("ok: section [%s] interface %s:%s\n",param->section.c_str(),param->name_if.c_str(),param->ip_if.c_str());

        // ------------------------------------------------

        if (!g.ExistsInterface(param->ip_if)) {
            Log.ScrFile("err: section [%s] not exists interface %s\n",param->section.c_str(),param->ip_if.c_str());
            exit(0);
            }

        TChannel *pC = FindChannel(param->ip_if,param->ip,param->port);
        if (pC) {
            Log.ScrFile("CreateChannelsIn double in  %s:%s:%s\n",param->ip_if.c_str(),param->ip.c_str(),param->port.c_str());
            ExitWithError("\n");
            }

//Log.ScrFile("CreateChannelsIn [%s]%s:%s:%s\n",param->section.c_str(),param->ip_if.c_str(),param->ip.c_str(),param->port.c_str());

        TInfo s;
        s.SetSection(param->section);
        s.SetNameIF((param->name_if));
        s.SetName(param->name);
        s.SetIF(param->ip_if);
        s.SetIP(param->ip);
        s.SetPort(param->port);
        pChannel->Initialize(&s,second_alive);

//Log.ScrFile("CreateChannelsIn =>  [%s]%s:%s:%s name=%s\n",s.Section().c_str(),s.IF().c_str(),s.IP().c_str(),s.Port().c_str(),s.Name().c_str());
        }
}




void CreateInterfaces()
{
    for (auto & pch : pChannelsArray) {

        TInfo s = pch->Info;

        int cores    = g.GetCPUCores();
        int cpu_core = Ini.GetInt(s.Section(),"cpu",0); //valid
        if (cpu_core > cores) { Log.ScrFile("bad cpu number in %s (exists only %d cpu)\n",s.Section().c_str(),cores); ExitWithError("\n"); }

        TInterface *p = FindInterface(s.IF());
        if (p) { continue; }

        // new interface
        TInterface *pif = new (nothrow) TInterface;
        if (!pif) ExitWithError("no memory for Interface");

        pif->name_sys  = g.GetInterfaceName(s.IF().c_str());
        pif->cpu_core   = cpu_core;
        pif->irq             = g.GetInterfaceIrq(s.IF().c_str());
        pif->ip_if           = s.IF();
        pInterfacesArray.push_back(pif);
    }
}




void CreateAnalizers()
{
    unsigned idx_ip = 0;

    for (auto & pch : pChannelsArray) {
        TAnalizerTS *pAnalizer = new (nothrow) TAnalizerTS;
        if (!pAnalizer) ExitWithError("no memory for Analizer");
        pAnalizersArray.push_back(pAnalizer);

        // set callback
        pChannelsArray[idx_ip]->m_pAnalizer=pAnalizer;

        TInfo s = pch->Info;
        pAnalizer->Info = s;

        pAnalizer->Initialize(s);

        TInterface * pI = FindInterface(s.IF());
        if (pI) {
            pI->list_channel_for_timer.push_back(pch);
            }
        else {
            Log.Scr("not found interface\n");
            }

        idx_ip++;
        }
}


void PrintChannels()
{
    Log.Scr("Analizers:Channels\n");
    for (auto  p : pChannelsArray) {
        TInfo s = p->Info;
        Log.ScrFile("[%s][%s:%s]",s.Section().c_str(),s.IP().c_str(),s.Port().c_str());
        Log.ScrFile("\n");
    }
}


void PrintInterfaces()
{
   Log.ScrFile("Interfaces:\n");
   for (auto & p : pInterfacesArray) {
       unsigned sz = unsigned(p->list_channel_for_timer.size());
        Log.Scr("interface [%s] contained %u channels\n",p->ip_if.c_str(),sz);
        for (unsigned i=0;i<sz;++i) {
            TInfo s = p->list_channel_for_timer[i]->Info;
            Log.ScrFile("    (%u) sect=[%s][%s] [%s:%s]\n",i,s.Section().c_str(),s.IF().c_str(),s.IP().c_str(),s.Port().c_str());
            }
        //Log.Scr("\n");
       }
}

void PrintAnalizers()
{
    // ============ PRINT
    Log.ScrFile("Analizers:\n");

    unsigned sz=unsigned(pAnalizersArray.size());
    for (unsigned i=0;i<sz;++i) {

        TChannel    *c=pChannelsArray[i];
        TAnalizerTS *p=pAnalizersArray[i];
        TInfo t = c->Info;
        TInfo s = p->Info;

        if (c->m_pAnalizer != p) Log.Scr("C BAD PTR :  ");
         Log.ScrFile("C[%s][%s:%s] => ",t.Section().c_str(),t.IP().c_str(),t.Port().c_str());
         Log.ScrFile("A[%s][%s:%s]\n",s.Section().c_str(),s.IP().c_str(),s.Port().c_str());
        }
}









int main_loop(bool console_mode)
{
    Log.Scr("*******************************************\n");
    Log.Scr("iptv analizer\n");
    Log.Scr("Copyright (C) 2015-2021\n");
    Log.Scr("Trofimov V.R. Tyumen\n");
    Log.Scr("*******************************************\n");
    Log.Scr("version=%s\n",SERVER_VERSION);

    Auth.Read();
    Config.Initialize();

    g.OutSystemInfo();

    g.area = Ini.GetStr("Common","area","");
    g.place = Ini.GetStr("Common","place","");

    unsigned time_alive = Ini.GetUInt("Common","time_alive",25);
    if (time_alive <  5) time_alive=5;
    if (time_alive > 60) time_alive=60;
    g.set_TimeAlive(time_alive);

    g.ignore_audio_cc = Ini.GetBoolean("COMMON","ignore_audio_cc",false);

    _es_timeouts taconfig{};
    taconfig.time_to_off = Ini.GetUInt("Common","time_to_off",5);
    taconfig.time_to_on  = Ini.GetUInt("Common","time_to_on",3);
    unsigned channel_timeout=max(taconfig.time_to_off,taconfig.time_to_on);
    Log.ScrFile("channel: time to off=%u,time to on=%u -> on\n",taconfig.time_to_off,taconfig.time_to_on);




    // ------------------------------------------- Create All Object

    CreateChannelsIn(time_alive);

    unsigned count_channel_in = unsigned(pChannelsArray.size());
    if (count_channel_in==0) {
        Log.ScrFile("not input channels\n");
        exit(1);
        }
    else Log.ScrFile("created %u input channels\n",count_channel_in);

    // Measure - always FIRST channel
    Measure.Create(pChannelsArray[0]);

    CreateInterfaces();
    CreateAnalizers();  // add to if

    for (auto & pAnalizer : pAnalizersArray)
        pAnalizer->SetTimeouts(taconfig);

    if (0) {
        Log.Scr("\n");
        PrintChannels();
        PrintInterfaces();
        PrintAnalizers();
       Log.Scr("\n");
       }

    Log.ScrFile("read [UI]\n");    UI.Read();
    Log.ScrFile("read [WS]\n");    WSConnection.Read();
    Log.ScrFile("read [HTTP]\n");  Http.Read();

    //string cfgname = g.getCurrentName() + "_readed.ini";
    //Ini.Write(cfgname);

//exit(0);

    g.setRunMode(false);

    g.channels_ready      = 0;
    g.channels_lock         = PTHREAD_MUTEX_INITIALIZER;
    g.channels_cond_end = PTHREAD_COND_INITIALIZER;

    if (g.AF_Mode()) {
        for (auto & pif : pInterfacesArray) {
            Log.ScrFile("input interface assign core %d [ip=%s sys=%s irq=%d]\n",pif->cpu_core,pif->ip_if.c_str(),pif->name_sys.c_str(),pif->irq);
            }
        }

    //============================================ START INP CHANNELS
    for (auto  p : pChannelsArray) {
        TInfo s = p->Info;
        if (p->ReceiveStart()<0) {
            Log.Scr("error start rcv: [%s]\ninterface=%s [%s:%s]\n",s.Section().c_str(),s.IF().c_str(),s.IP().c_str(),s.Port().c_str());
            ExitWithError("exit\n");
            }
        //Log.Scr("start rcv: [%s] if=%s [%s:%s]\n",s.Section().c_str(),s.IF().c_str(),s.IP().c_str(),s.Port().c_str());
        }



    //============================================ START THREADS
    Time.Start();


    for (auto & pInterface : pInterfacesArray)
         pInterface->ThreadStart();

    string current_time = Time.getCurrentDate() + " " + Time.getCurrentTime();
    Log.ScrFile("begin %s\n",current_time.c_str());

    unsigned sz = unsigned(pChannelsArray.size());
    int delay_second = sz / 10;   //задержка на открытие каналов
    if (delay_second < 1) delay_second=1;

    unsigned  run_delay=Ini.GetUInt("Common","run_delay",1);
    delay_second += run_delay;
    delay_second += channel_timeout;

    Http.Start();
    Statistic.Start();

    if (console_mode) {
        Log.Scr("console mode started ..\n");
        Log.setServiceMode(false);
        }
    else {
        Log.Scr("service mode started ..\n");
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        // close(STDERR_FILENO); // use backtrace
        Log.setServiceMode(true);
        }


    while(g.th_running) {
        //ожидание готовности приема
        pthread_mutex_lock(&g.channels_lock);

        //ожидаем готовности всех интерфейсов
        while(g.channels_ready < pInterfacesArray.size())  //while(!channels_ready)
            pthread_cond_wait(&g.channels_cond_end,&g.channels_lock);
        pthread_mutex_unlock(&g.channels_lock);

        // есть готовность
        pthread_mutex_lock(&g.channels_lock);
        g.channels_ready=0;
        pthread_mutex_unlock(&g.channels_lock);

        Time.ReadSystemDateTime();

        // calc traffic for interfaces
        for (auto & pInterface : pInterfacesArray)
            pInterface->Run();

        // delay start mode
        if (delay_second) {
             if (--delay_second==0) {
                 Log.Scr("running ...\n");

                 for (auto & pAnalizer : pAnalizersArray)
                     pAnalizer->Run(false);

                 Statistic.RunFirst();
                 g.setRunMode(true);
                 }
            else {
                 Log.Scr("wait %d\n",delay_second);

                 for (auto & pAnalizer : pAnalizersArray)
                     pAnalizer->Run(true);

                 }
            continue;
            }



        // WORK SECTION


        try {

             for (auto & pAnalizer : pAnalizersArray)
                 pAnalizer->Run(false);

        } catch (const std::exception& e) {
            Log.ScrFile("catch rAnalizer %s\n", e.what());
        } catch (...) {
            Log.ScrFile("catch rAnalizer %s:%s\n",Time.getCurrentDate().c_str(),Time.getCurrentTime().c_str());
        }


        try {

            UI.Run();

        } catch (const std::exception& e) {
            Log.ScrFile("catch UI %s\n", e.what());
        } catch (...) {
            Log.ScrFile("catch UI %s:%s\n",Time.getCurrentDate().c_str(),Time.getCurrentTime().c_str());
        }

        try {

            Statistic.Run();

        } catch (const std::exception& e) {
            Log.ScrFile("catch Statistic %s\n", e.what());
        } catch (...) {
            Log.ScrFile("catch Statistic %s:%s\n",Time.getCurrentDate().c_str(),Time.getCurrentTime().c_str());
        }


        // ----------------------------------------------------------------------------------------------------

        try {

        // summary traffic for all interfaces (Mbit)
        double summary_traffic =0.0;
        for (auto & pInterface : pInterfacesArray)
            summary_traffic += pInterface->Traffic();

        } catch (const std::exception& e) {
            Log.ScrFile("catch Traffic %s\n", e.what());
        } catch (...) {
            Log.ScrFile("catch Traffic %s:%s\n",Time.getCurrentDate().c_str(),Time.getCurrentTime().c_str());
        }

        // ----------------------------------------------------------------------------------------------------

        try {

            WSConnection.Run();

        } catch (const std::exception& e) {
            Log.ScrFile("catch WS %s\n", e.what());
        } catch (...) {
            Log.ScrFile("catch WS %s:%s\n",Time.getCurrentDate().c_str(),Time.getCurrentTime().c_str());
        }


        if (Time.IsMeasureEnd()) {
            Time.MeasureEnd();
            Statistic.Reset();
            }



        //Log.Scr("%s\n",format_02f(1.556).c_str());


        } //th_running

    main_stop(1);

    return 0;
}
//==========================================================================

int main_stop(int signo)
{
    static bool reenter=false;

    if (reenter) return 0;
    reenter = true;

    g.th_running = 0;

    Log.ScrFile(" \n signal %d please wait stop...\n",signo);

    WSConnection.WS_CloseAll();

Log.ScrFile("wsconn stopped\n");

    Statistic.Stop();

Log.ScrFile("Statistic stopped\n");

    Http.Stop();

Log.ScrFile("http stopped\n");

    // stop all inp threads wait end
    for (auto & pInterface : pInterfacesArray)
        pInterface->ThreadStop();

//Log.ScrFile(" => ThreadStop stoped\n");


//Log.ScrFile(" => SendStop stoped\n");

    // stop all inp channels
    for (auto & pChannel : pChannelsArray)
        pChannel->ReceiveStop();

//Log.ScrFile(" => ReceiveStop stoped\n");

    g.th_running = 0;

    string current_time = Time.getCurrentDate() + " " + Time.getCurrentTime();
    Log.ScrFile("end %s\n",current_time.c_str());

    return 1;
}




