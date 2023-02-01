#include "twsconnection.h"

#include "tini.h"
#include "tlog.h"


#include "tui.h"

TWSConnection   WSConnection;


bool TWSConnection::CheckUserLogin(const string &login,const TIPAddress ip)
{
    for (auto & connect : connect_array) {
        if (connect.info_auth.session_ip == ip &&
            connect.info_auth.session_login==login)
            return true;
        }
    return false;
}

bool TWSConnection::CheckUserToken(const string &token)
{
    for (auto & connect : connect_array) {
        if (connect.info_auth.session_token == token) {
            /*
            //определим текущую длительность сессии
            time_t timeNow=std::time(nullptr);
            time_t session_time_start = connect.info_auth.session_time_start;
            double session_length = connect.info_auth.session_length;
            double session_current_length=difftime(timeNow,session_time_start);
            if (session_length < session_current_length) { //expired ?
                 //Log.Scr("******** Session EXPIRED ***********\n");
                 return false;   //закроем сессию
                 }
            */
            return true;
            }
        }
    return false;
}

TWSConnection::_ws_conn *TWSConnection::FindConnection(const mg_connection *conn)
{
    for (auto & connect : connect_array) {
        if (connect.conn == conn) return &connect;
        }
    return nullptr;
}

void TWSConnection::Send(TWSConnection::_ws_conn *p, const string &v)
{
//Log.Scr("SND:%s\n",v.c_str());
    mg_websocket_write(p->conn, WEBSOCKET_OPCODE_TEXT, v.data(),v.size());
}


void TWSConnection::user_Login(_ws_conn *conn,const json &j)
{
    string login;
    string passwd;
    string unique_id;
    try {
        login     = j.value("login", "");
        passwd    = j.value("passwd", "");
        unique_id = j.value("unique_id", "");
    } catch(...) {
        string answer =  R"({"cmd":"login","msg":"rejected: bad parameter"})";
        Send(conn,answer);
        return;
        }


    TIPAddress ip;
    const mg_request_info *ptr_info=mg_get_request_info(conn->conn);
    if (ptr_info)
        ip = ptr_info->remote_addr;

    if (CheckUserLogin(login,ip)) {
        string answer =  R"({"cmd":"login","msg":"rejected: this account used.."})";
        Send(conn,answer);
        return;
        }

    conn->lock();

    _info_auth auth = Auth.Session_Login(login,passwd,unique_id);
    if (auth.session_token.empty()) {
        conn->unlock();
//Log.Scr("Bad user_Login %s, pass=%s\n",login.c_str(),passwd.c_str());
        string answer =  R"({"cmd":"login","msg":"rejected: invalid login:password"})";
        Send(conn,answer);
        return;
        }

    auth.session_login = login;
    auth.session_ip = ip;
    // User connection
    conn->info_auth = auth;

    if (login==LOGIN_STATISTIC && passwd==PASSW_STATISTIC) {
        //conn->stat = true;
        conn->require_full_info = true;
        conn->require_stat_info = false;
        }
    else {
        //conn->stat = false;
        conn->require_full_info = true;
        conn->require_stat_info = false;
        }

    conn->unlock();

    string answer =  R"({"cmd":"login","token":")" + auth.session_token + R"("})";

Log.Scr("[HTTP] session start %s [%u]\n",ip.c_str(),conn->user_id);
    Send(conn,answer);
}

void TWSConnection::user_Logout(_ws_conn *conn,const json &j)
{
    string token;
    try {
        token  = j.value("token", "");
    } catch(...) {
        string answer =  R"({"cmd":"logout","msg":"rejected: bad parameter"})";
        Send(conn,answer);
        return;
        }
    conn->lock();
    if (!CheckUserToken(token)) {
        conn->unlock();
        string answer =  R"({"cmd":"logout","msg":"rejected: ..."})";
        Send(conn,answer);
        return;
        }

    conn->info_auth = {};

    conn->unlock();
Log.Scr("Ok user_Logout\n");
    string answer =  R"({"cmd":"logout","msg":"logout ok"})";
    Send(conn,answer);
}

void TWSConnection::user_ServerInfo(_ws_conn *conn,const json &j)
{
    string token;
    try {
        token = j.value("token","");
        }
    catch(...) {
        string answer =  R"({"cmd":"serverinfo","msg":"rejected: bad parameter"})";
        Send(conn,answer);
        return;
        }
    conn->lock();
    if (CheckUserToken(token)) {
        conn->require_full_info = true;
        conn->unlock();
//Log.Scr("Ok user_ServerInfo\n");
        return;
        }
    conn->unlock();
    string answer =  R"({"cmd":"serverinfo","msg":"rejected: ..."})";
    Send(conn,answer);
}

void TWSConnection::user_Detail(_ws_conn *conn,const json &j)
{
    string token;
    unsigned channel;   //selected channel
    unsigned subchannel;       //selected subchannel if mpts , if spts=0
    try {
        token      = j.value("token","");
        channel    = unsigned(j.value("channel",0));
        subchannel = unsigned(j.value("subchannel",0));
        }
    catch(...) {
        string answer =  R"({"cmd":"detail","msg":"rejected: bad parameter"})";
        Send(conn,answer);
        return;
        }
    conn->lock();
    if (CheckUserToken(token)) {

        //unsigned  index_ts  = UI.GetIndexTS(channel);       // ws channel
        //unsigned  index_pmt = UI.GetIndexPMT(subchannel);   // ws subchannel

        if (conn->detail_channel != channel)  { }

        conn->detail_started = true;
        conn->detail_channel = channel;
        conn->detail_subchannel  = subchannel;

        conn->unlock();
//Log.Scr("Ok user_Detail channel=%u subchannel=%u\n",channel,subchannel);
        return;
        }
    conn->unlock();
    string answer =  R"({"cmd":"detail","msg":"rejected: ..."})";
    Send(conn,answer);
}

void TWSConnection::user_DetailStop(_ws_conn *conn,const json &j)
{
    string token;
    try {
        token = j.value("token","");
        }
    catch(...) {
        string answer =  R"({"cmd":"detailstop","msg":"rejected: bad parameter"})";
        Send(conn,answer);
        return;
        }
    conn->lock();
    if (CheckUserToken(token)) {

        conn->detail_started=false;

        conn->unlock();
//Log.Scr("Ok user_DetailStop\n");
        return;
        }
    conn->unlock();
    string answer =  R"({"cmd":"detailstop","msg":"rejected: ..."})";
    Send(conn,answer);
}


void TWSConnection::user_Stat(_ws_conn *conn,const json &j)
{
    string token;
    try {
        token = j.value("token","");
        }
    catch(...) {
        string answer =  R"({"cmd":"stat","msg":"rejected: bad parameter"})";
        Send(conn,answer);
        return;
        }
    conn->lock();
    if (CheckUserToken(token)) {

        conn->require_stat_info=true;

        conn->unlock();

//Log.Scr("Ok user_Stat\n");
/*
        if (UI.StatisticInfo().empty()) {
            string answer =  R"({"cmd":"stat","msg":"empty ..."})";
            Send(conn,answer);
            return;
            }

        //Send(conn,UI.StatisticInfo());
        */
        return;
        }
    conn->unlock();

    string answer = R"({"cmd":"stat","server_date":"0.0.0","server_time":"0:0:0","server_count":0,"period":0})";
    //string answer = R"({"cmd":"stat","msg":"rejected: ..."})";
    Send(conn,answer);
}



// ===================================================
//
// PARSE COMMAND (Run Asinc)
//
void TWSConnection::ParseCommand(_ws_conn *conn,char *data, size_t data_len)
{
    if ((!data)||(data_len==0)) return;
    char tmp_buffer[256]{};

    if (data_len > (sizeof(tmp_buffer)-2)) {
        string answer = R"({"cmd":"error","msg":"command too large"})";
        Send(conn,answer);
        return;
        }
    memcpy(tmp_buffer,data,data_len); // Clear input
    //tmp_buffer[data_len]=0;           // !!!
    //tmp_buffer[data_len+1]=0;      // !!!
    json j;
    try {
        j=json::parse(tmp_buffer);   // Create JSON object from string (deserialized)
        }
    catch(...) {
        string answer = R"({"cmd":"error","msg":"unknown format command"})";
        Send(conn,answer);
        return;
        }
    string cmd;
    try {
        cmd = j.value("cmd","");
        }
    catch(...) {
        string answer =  R"({"cmd":"error","msg":"unknown command"})";
        Send(conn,answer);
        return;
        }

//Log.Scr("recieved cmd %s\n",cmd.c_str());

    if (cmd=="login")       { user_Login(conn,j);       return; } else
    if (cmd=="logout")      { user_Logout(conn,j);      return; } else
    if (cmd=="serverinfo")  { user_ServerInfo(conn,j);  return; } else
    if (cmd=="detail")      { user_Detail(conn,j);      return; } else
    if (cmd=="detailstop")  { user_DetailStop(conn,j);  return; } else
    if (cmd=="stat")        { user_Stat(conn,j);        return; }

    string answer =  R"({"cmd":"error","msg":"not implemented"})";
    Send(conn,answer);
}

// ===================================================
//
// READ
//
void TWSConnection::Read()
{
    unsigned num=0;
    for(auto & connect : connect_array) {
        connect.info_auth   = {};
        connect.user_id             = num++;
        connect.conn                = nullptr;
        connect.timeout_ping        = 0;
        connect.require_full_info   = false;
        connect.structures_updated  = false;
        connect.detail_started      = false;
        connect.detail_channel      = 0;
        connect.detail_subchannel   = 0;
        //connect.stat                = false;
        connect.require_stat_info   = false;
        connect.StopDataMode();
        }
}


// ===================================================
//
// RUN (main loop)
//
void TWSConnection::Run()
{
    for (auto & connect : connect_array)
        {

        // if established user connection
        if (connect.conn && !connect.info_auth.session_token.empty())
            {
            //send ping if timeout
            if ((connect.timeout_ping++)>MAX_PING_TIMEOUT) {
                char dping[]="ping";
                mg_websocket_write(connect.conn, WEBSOCKET_OPCODE_PING, dping, sizeof(dping));
                connect.timeout_ping=0;
                }

            connect.lock();
            //account timeout ?
            if (!CheckUserToken(connect.info_auth.session_token)) {

                connect.info_auth   = {};

                connect.StopDataMode();
                //connect.stat = false;
                connect.unlock();
                string answer =  R"({"cmd":"logout","msg":"timeout"})";
                Send(&connect,answer);
                continue;
                }

            // ===================

            if (connect.require_stat_info ) {
                connect.require_stat_info=false;

                if (UI.StatisticInfo().empty()) {
                    connect.unlock();
                    string answer =  R"({"cmd":"stat","msg":"not filled , please waiting"})";
                    Send(&connect,answer);
                }
                else {
                    connect.unlock();
                    Send(&connect,UI.StatisticInfo());
                }



                connect.lock();
                }

            // ===================

            if (UI.IsUpdatedStructure())
                connect.structures_updated=true;

            if (connect.require_full_info ) {
                connect.require_full_info=false;
                connect.structures_updated=false;
                connect.unlock();
                Send(&connect,UI.ServerInfo());
                continue;
                }

            if (connect.structures_updated) {
                connect.require_full_info=false;
                connect.structures_updated=false;
                connect.unlock();
                Send(&connect,UI.ServerInfo());
                continue;
                }

            if (connect.detail_started) {
                unsigned channel = connect.detail_channel;
                connect.unlock();
                Send(&connect,UI.StatusInfoAndDetail(channel));
                continue;
                }
            // Detail not requested
            connect.unlock();

            Send(&connect,UI.StatusInfo());   //send only status
            } // if established connection
        } // next connection
}



int TWSConnection::WS_NewConnect(const mg_connection *conn)
{
    if (m_lock_input) return 1;

    for (auto & connect : connect_array) {
        connect.lock();
        if (connect.conn == nullptr) {
            connect.conn  =const_cast<mg_connection *>(conn);
            connect.info_auth   = {};
            connect.timeout_ping=0;
            //connect.stat = false;
            connect.unlock();
            return 0;
            }
        connect.unlock();
        }
//Log.Scr("ws refused connection: max connections exceeded\n");
    return 1;
 }



void TWSConnection::WS_CloseAll()
{
    m_lock_input = true;
    for (auto & p : connect_array) {
        if (p.conn) WS_Close(p.conn);
        }
}


void TWSConnection::WS_Close(const struct mg_connection *conn)
{
     _ws_conn *p=FindConnection(conn);
    if (!p) {
// Log.Scr("ws close unknown connection\n");
        return;
        }

    if (p->info_auth.session_token.empty()) {
//Log.Scr("ws close connection bad user\n");

        p->lock();
        p->conn=nullptr;
        p->unlock();
        return;
        }

//Log.Scr("ws close connection and logout\n");
    const mg_request_info *req_info = mg_get_request_info(conn);
    Log.Scr("[HTTP] session stop %s [%u]\n",req_info->remote_addr,p->user_id);
    p->lock();

    p->info_auth   = {};
    p->StopDataMode();
    p->conn=nullptr;
    p->unlock();
}


int TWSConnection::WS_Data(struct mg_connection *conn, int flags,char *data, size_t data_len)
{
    if (m_lock_input) return 1;

     _ws_conn *p=FindConnection(conn);

    if (!p) {
//Log.Scr("Received ws data from unknown connection\n");
        return 1;
        }
    if (flags & 0x80) {
        flags &= 0x7f;
        switch (flags) {
            case WEBSOCKET_OPCODE_CONTINUATION:   //fprintf(stderr, "CONTINUATION...\n");
                break;
            case WEBSOCKET_OPCODE_TEXT: //Command
                if (strncmp("ping", data, data_len)== 0) {
                    mg_websocket_write(conn, WEBSOCKET_OPCODE_PONG, data, data_len);
                    break;
                    }
                ParseCommand(p,data,data_len);
                break;
            case WEBSOCKET_OPCODE_BINARY: //fprintf(stderr, "BINARY...\n");
                break;
            case WEBSOCKET_OPCODE_CONNECTION_CLOSE: //fprintf(stderr, "CLOSE...\n");
                 mg_websocket_write(conn, WEBSOCKET_OPCODE_CONNECTION_CLOSE, data, data_len);
                return 0; // close connection
            case WEBSOCKET_OPCODE_PING:// client send PING, respond with PONG
                mg_websocket_write(conn, WEBSOCKET_OPCODE_PONG, data, data_len);
                break;
            case WEBSOCKET_OPCODE_PONG:// received PONG, no action
                break;
            default:
                //Log.Scr("Unknown ws flags: %02x\n", flags);
            break;
            }
        }
    return 1;   // keep connection open
}
