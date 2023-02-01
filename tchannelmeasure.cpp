#include "tchannelmeasure.h"
#include "tini.h"
#include "tchannel.h"
#include "ttime.h"

TChannelMeasure Measure;



void TChannelMeasure::Create(TChannel *p)
{
    Info.SetIF(p->Info.IF());
    Info.SetIP(p->Info.IP());
    Info.SetPort(p->Info.Port());

    string section="MEASURE";
    p->measurer = true;
    p->measurer = Ini.GetBoolean(section,"from_gen",false);
    p->measure_pid = Ini.GetUInt(section,"from_pid",200);
}

void TChannelMeasure::Timer()
{
    if (change_generator) {
        //Time.MeasureSetTime(hot.time);  // if new generator
        change_generator = false;
        }
    if (change_stat_period) {
        //Time.MeasureStartRemote();
        change_stat_period = false;
        }
    accumulate = hot;
}

bool   TChannelMeasure::IsValid()         { return accumulate.valid; }
string TChannelMeasure::Packets_Summary() { return to_string(accumulate.packets); }
string TChannelMeasure::Packets_ok()      { return to_string(accumulate.pkt_ok); }
string TChannelMeasure::Packets_drop()    { return to_string(accumulate.pkt_drop); }
string TChannelMeasure::Packets_reordered()    { return to_string(accumulate.pkt_reorder); }

string TChannelMeasure::ErrPercent()
{
    double ok;
    if (accumulate.pkt_ok)
        ok = accumulate.pkt_ok; // accumulate.packets ?
    else
        ok = 0.000001;

    double err = double(accumulate.pkt_drop + accumulate.pkt_reorder);
    double percent = err * 100.0 / ok;
    return to_string(percent);
}


void TChannelMeasure::Reset()
{
    //hot.valid       = hot.valid;
    //hot.time        = hot.time;
    //hot.prev_seq    = 0;
    hot.packets    = 0;
    hot.pkt_ok     = 0;
    hot.pkt_drop   = 0;
    hot.pkt_reorder   = 0;
}

void TChannelMeasure::Packet(uint8_t *rcv_info)
{
    _check_info *pinfo = (_check_info *)rcv_info;


    hot.valid = false;

     // check magic
    if (pinfo->magic != 0x89123890840) return;

    hot.time       = pinfo->timestamp;
    uint16_t id    = pinfo->id_generator;
    uint16_t seq   = pinfo->sequence;


    if (id_generator != id) {
        change_generator = true;
        id_generator = id;
        cycle        = pinfo->cycle;
        sequence     = seq;
        count_188  = 0;
        change_stat_period=false;
        num_stat_period = pinfo->period;
        return;
        }


    if (!g.RunMode()) {
        id_generator = id;
        cycle = pinfo->cycle;
        sequence = seq;
        count_188  = 0;

        change_stat_period=false;
        num_stat_period= pinfo->period;
        return;
        }



    if (num_stat_period != pinfo->period) {
        num_stat_period = pinfo->period;
        change_stat_period=true;   // reset in timer
Log.Scr("new stat period\n");
        }




    if (cycle != pinfo->cycle) {
        cycle        = pinfo->cycle;
        sequence     = seq - 1;  // new cycle from gen seq ==1
        count_188    = 1;
Log.Scr("new cycle gen %u seq=%u\n",cycle,seq);
        //return;
        }

    hot.valid = true;

    if (0) {
        struct tm tstruct = *localtime(&hot.time);
        char text_date[64];
        char text_time[64];
        strftime(&text_date[0],sizeof(text_date),"%d.%m.%Y",&tstruct);
        strftime(&text_time[0],sizeof(text_time),"%X",&tstruct);
Log.Scr("%s:%s seq=%u prev_seq=%u\n",text_date,text_time,seq,sequence);
        }


    if (seq == sequence) {
        return;
    }


    uint16_t elapsed = sequence + 1;

    hot.packets++;

    if (elapsed == seq) {
        hot.pkt_ok++;
        sequence = seq;
        return;
        }

    if (elapsed > seq) {
        hot.pkt_reorder++;
        sequence = seq;
        return;
        }

    if (elapsed < seq) {
        hot.pkt_drop += (seq - elapsed);
        sequence = seq;
        return;
        }
}


