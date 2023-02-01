#pragma once

#include <inttypes.h>
#include <stddef.h>

 #include <byteswap.h>


  class TBitstream
  {
  private:
    uint8_t       *m_data;
    size_t         m_offset;
    const size_t   m_len;
    bool           m_error;
    const bool     m_doEP3;   // only hevc

  public:
    TBitstream(uint8_t *data, size_t bits)
    : m_data(data)
    , m_offset(0)
    , m_len(bits)
    , m_error(false)
    , m_doEP3(false)
    {}

    // this is a bitstream that has embedded emulation_prevention_three_byte
    // sequences that need to be removed as used in HECV.
    // Data must start at byte 2
    TBitstream(uint8_t *data, size_t bits,  bool doEP3)
    : m_data(data)
    , m_offset(16) // skip header and use as sentinel for EP3 detection
    , m_len(bits)
    , m_error(false)
    , m_doEP3(doEP3)
    {}

    void         skipBits(unsigned int num);
    unsigned int readBits(int num);
    unsigned int showBits(int num);                   // only ES_AC3::Parse
    unsigned int readBits1() { return readBits(1); }
    unsigned int readGolombUE(int maxbits = 32);   // h264 , hevc
    signed int   readGolombSE();             // only h264
    size_t       length() { return m_len; }
    bool         isError() { return m_error; }
  };

