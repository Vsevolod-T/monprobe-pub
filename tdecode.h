#pragma once


#include "theader.h"

#include "trawes.h"
#include "trawts.h"
#include "tanalizeres.h"
#include "tanalizerts.h"

#include "ES_MPEGVideo.h"
#include "ES_MPEGAudio.h"
#include "ES_h264.h"
#include "ES_hevc.h"
#include "ES_AAC.h"
#include "ES_AC3.h"
#include "ES_Subtitle.h"
#include "ES_Teletext.h"


size_t VisualLength(const string &str);
int  Decode588595v2(unsigned max_size_out,char *dst,uint8_t *src,unsigned insize);


void ParsePesHeader(TAnalizerES *pes);
const string GetTypeText(TAnalizerES *pes);
const string GetStreamText(TAnalizerES *pes);
const char* GetStreamCodecName(es_stream stream_type);

void PRINT_ANALIZED_TS(TAnalizerTS  *p,const string& hdr);



class TDecode
{
public:
    TDecode() { }
};

extern ES_MPEG2Audio  m2audio;
extern ES_MPEG2Video  m2video;
extern ES_AC3         ac3audio;
extern ES_AAC         aacaudio;
extern ES_Teletext    mteletext;
extern ES_Subtitle    msubtitle;
extern ES_h264        mh264;
extern ES_hevc        mhevc;

extern TDecode Decode;




