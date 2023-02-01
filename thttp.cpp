#include "thttp.h"

#include "tini.h"
#include "tlog.h"

#include "twsconnection.h"

THttp           Http;

// V1.9


int WebSocketConnectHandler(const struct mg_connection *conn, void * /*cbdata*/)
{
  //Log.Scr("ws connect\n");
  return WSConnection.WS_NewConnect(conn);
}

void WebSocketReadyHandler(struct mg_connection * /*conn*/, void * /*cbdata*/)
{
// __attribute__((unused))
  //Log.Scr("ws ready\n");
  //WSConnection.WS_NewThread(conn);
}


void WebSocketCloseHandler(const struct mg_connection *conn, void * /*cbdata*/)
{
  //Log.Scr("ws close\n");
  WSConnection.WS_Close(conn);
}

int WebsocketDataHandler(struct mg_connection *conn, int bits, char *data, size_t len, void * /*cbdata*/)
{
  //Log.Scr("ws data\n");
  return WSConnection.WS_Data(conn,bits,data,len);
}

/* Called when civetweb is about to log a message. If callback returns non-zero, civetweb does not log anything. */
int http_log_message_handler(const struct mg_connection * /*conn*/, const char * /*message*/)
{
    //Log.Scr("log_message=%s\n",message);
    return 1;
}

/* Called when civetweb is about to log access. If callback returns non-zero, civetweb does not log anything. */
int http_log_access_handler(const struct mg_connection * /*conn*/, const char  * /*message*/ )
{
  //Log.Scr("log_access=%s\n",message);
  return 1;
}

//-----------------------------------------------------


void THttp::Read(void)
{
    http_on=0;

    webroot             = Ini.GetStr("HTTP","webroot","httproot");
    listen_port         = Ini.GetStr("HTTP","listen_port","");
    access_control_list = Ini.GetStr("HTTP","access_list","");

    string log_dir      = Ini.GetStr("HTTP","log_folder","./httplog");
    access_log_file     = log_dir + "/access.log";
    error_log_file      = log_dir + "/error.log";

    struct stat st;


Log.ScrFile("http: access_control_list %s\n",access_control_list.c_str());

    memset(&st,0,sizeof(st));
    if (stat(webroot.c_str(), &st) == -1) {
            Log.ScrFile("http: not folder %s  -> http off\n\n",webroot.c_str());
            return;
            }

    if (listen_port=="") {
        Log.ScrFile("http: server v.%s, webroot=%s, no listen port -> http off\n",mg_version(),webroot.c_str());
        return;
        }

    if (access_control_list=="") {
        Log.ScrFile("http: server v.%s, webroot=%s, listen port=%s, access list empty -> http off\n",mg_version(),webroot.c_str(),listen_port.c_str());
        return;
        }

    memset(&st,0,sizeof(st));
    if (stat(log_dir.c_str(), &st) == -1) {
        if (mkdir(log_dir.c_str(), 0777)!=0) {
            Log.ScrFile("http: error make log folder %s -> http off\n",log_dir.c_str());
            return;
            }
        }

    Log.ScrFile("http: server v.%s, webroot=%s, listen port=%s -> http on\n",mg_version(),webroot.c_str(),listen_port.c_str());
    http_on=1;
}

void THttp::Start(void)
{
    if (http_on==0) return;

    const char *options[] = {
        "document_root"      , webroot.c_str(),
        "listening_ports"    , listen_port.c_str(),
        "request_timeout_ms" , "30000",
        "access_log_file"    , access_log_file.c_str(),   //
        "error_log_file"     , error_log_file.c_str(),
        "access_control_list", access_control_list.c_str(),
        nullptr};

    memset(&callbacks, 0, sizeof(callbacks));
    callbacks.log_message = http_log_message_handler;
    callbacks.log_access  = http_log_access_handler;

    ctx = mg_start(&callbacks, 0, options);

    if (!ctx) {
        Log.ScrFile("failed start HTTP -> Off\n");
        http_on = 0;
        return;
        }

    mg_set_websocket_handler(ctx,
                             "/ws",
                             WebSocketConnectHandler,
                             WebSocketReadyHandler,
                             WebsocketDataHandler,
                             WebSocketCloseHandler,
                             nullptr);
}

void THttp::Stop(void)
{
    if (http_on==0) return;
    http_on=0;
    mg_stop(ctx); // Stop the server
}
