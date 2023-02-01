#pragma once


#include "theader.h"

#include "civetweb.h"
//using namespace CivetWeb;

class THttp
{
  int   http_on;

  string  listen_port;
  string  webroot;

  string access_log_file;
  string error_log_file;
  string access_control_list;

  struct mg_context *ctx;
  struct mg_callbacks callbacks;

public:

  void Read(void);          //read ini
  void Start(void);         //start main thread
  void Stop(void);

};

extern THttp Http;
