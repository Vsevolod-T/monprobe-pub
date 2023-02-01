#pragma once

#include "theader.h"
#include <sys/time.h>

class TChannel;

class TChannelMeasure
{

    // from generator
    struct _check_info {
            uint64_t magic;        // unique num
            time_t   timestamp;    // current time (sec)
            uint16_t sequence;     //
            uint16_t id_generator; // id this generator (if restart gen)
            uint16_t cycle;        // if change -> start new sequence
            uint16_t period;       // if change -> start new measure
        }__attribute__((packed));

    struct _measured {
        bool     valid=false;
        time_t   time {};
        uint64_t packets=0;
        uint64_t pkt_ok=0;
        uint64_t pkt_drop=0;
        uint64_t pkt_reorder=0;
    };

    bool  change_stat_period=false;
    uint16_t num_stat_period=0;

    bool time_valid = false;
    time_t rcv_time {};
    bool  change_generator=false;
    uint16_t id_generator=0;
    uint16_t cycle=0;

    uint16_t sequence=0;
    uint16_t count_188=0;

    _measured hot {};
    _measured accumulate {};


public:
    TChannelMeasure() {};

    void Create(TChannel *p);
    void Packet(uint8_t *rcv_info);
    void Timer();

    bool IsValid();
    string Packets_Summary();
    string Packets_ok();
    string Packets_drop();
    string Packets_reordered();

    string ErrPercent();
    void   Reset();

     TInfo Info;
};

extern TChannelMeasure Measure;
