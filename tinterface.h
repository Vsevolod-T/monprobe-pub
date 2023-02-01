#pragma once


#include "theader.h"

#include "thread_rcv_af.h"



class TAnalizerTS;
class TChannel;
class TChannelOut;


// Phisical INTERFACE

class TInterface
{
    // ---------------------------------------------- calc traffic
    bool                    flag_first  = true;

    long long unsigned      bytes_prev  = 0;
    long long unsigned      bytes_curr  = 0;

    double                  traffic     = 0.0;      // summary interface traffic  в мегабитах

public:

    double      Traffic()   { return traffic; }

    vector <TChannel *>     list_channel_for_timer;    // filled in CreateAnalizers

public:

    // Work
    atomic_uint    flag_timer;     //1 sec

    string         name_sys;       // eth0 ...
    int            cpu_core;       //cpu core (2-N) or 0
    int            irq;
    TIPAddress     ip_if;          // interface=10.10.10.10

    void            Run();         //calc traffic
    //void          WriteToConfig(unsigned num_if);



    //  THREAD
    atomic_uint    th_running;
    atomic_uint    is_error;
    atomic_uint    is_running;

    string           err_message;

    bool              af_mode = false;
    thread          std_thread;

    void ThreadStart();
    void ThreadStop();

};


extern vector <TInterface *>     pInterfacesArray;

extern TInterface *FindInterface(TIPAddress ip);
