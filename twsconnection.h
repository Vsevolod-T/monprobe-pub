#pragma once

#include "theader.h"

#include "json.hpp"
using json = nlohmann::json;

#include "civetweb.h"
//using namespace CivetWeb;
#include "tauth.h"


class TWSConnection
{
    static const unsigned MAX_PING_TIMEOUT=10;

        bool m_lock_input = false;

    struct _ws_conn {

        _info_auth      info_auth;

        mg_connection  *conn=nullptr;

        unsigned        user_id=0;    // current index
        unsigned        timeout_ping=0;

        mutex           lock_connect;
        void lock()     { unique_lock<mutex> locker(lock_connect); }
        void unlock()   { lock_connect.unlock(); }

        unsigned        detail_channel=0;         // selected channel
        unsigned        detail_subchannel=0;      // selected subchannel if mpts , if spts=1

        bool            require_full_info=false;  // user login ...
        bool            structures_updated=false; // changed count pmt
        bool            detail_started=false;     // used Detail

        bool            require_stat_info=false;

        void StopDataMode() {
            detail_started = false;
            detail_channel = 0;
            detail_subchannel = 0;
            }
       };//__attribute__ ((aligned (16)));

    array<_ws_conn,MAX_WSCONNECTIONS> connect_array;

    bool CheckUserLogin(const string &login,const TIPAddress ip);
    bool CheckUserToken(const string &token);

    _ws_conn *FindConnection(const mg_connection *conn);

    void Send(_ws_conn *p,const string &v);

    void user_Login(_ws_conn *conn,const json &j);
    void user_Logout(_ws_conn *conn,const json &j);
    void user_ServerInfo(_ws_conn *conn,const json &j);
    void user_Detail(_ws_conn *conn,const json &j);
    void user_DetailStop(_ws_conn *conn,const json &j);

    void user_Stat(_ws_conn *conn,const json &j);

    void ParseCommand(_ws_conn *conn,char *data, size_t data_len);

public:

    int  WS_NewConnect(const mg_connection *conn);
    //void WS_NewThread(const mg_connection *conn) {}
    void WS_Close(const mg_connection *conn);
    int  WS_Data(mg_connection *conn, int flags,char *data, size_t data_len);

    void Read();
    void Run();

    void  WS_CloseAll();


};

extern TWSConnection WSConnection;
