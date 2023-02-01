#include "tstatistic.h"
#include "tglobal.h"

#include "tanalizerts.h"

#include "tinterface.h"
#include "tchannel.h"
#include "tchannelmeasure.h"

#include "tini.h"
#include "tlog.h"
#include "ttime.h"



TStatistic Statistic;



void TStatistic::Start()
{
}

void TStatistic::RunFirst()
{
    OutStateChannelsToLog();
}


void TStatistic::Run()
{
    if (!Time.IsMeasureEnd()) return;
    if (!Time.IsStatisticLocal()) return;

    string td = Time.getCurrentDate();
    replace(td.begin(),td.end(), '.', '_');
    string stat_filename = g.getLogFolder() + "/" + "stat_" + td + ".log"; // =  /path/fileXXXXXXX.log
    OutStatistic(stat_filename);
}

void TStatistic::Stop()
{
    if (!Time.IsStatisticLocal()) return;

    string td = Time.getCurrentDate();
    replace(td.begin(),td.end(), '.', '_');
    string stat_filename = g.getLogFolder() + "/" + "stat_" + td + ".log"; // =  /path/fileXXXXXXX.log

    OutStatistic(stat_filename);
}

void TStatistic::Reset()
{
    if (Measure.IsValid())
        Measure.Reset();
}




void TStatistic::OutStatistic(const string &filename)
{
    FILE *m_file=fopen(filename.c_str(),"a+");
    if (!m_file) return;

    char buf[512];
    sprintf(buf,"\nStatistic start %s %s seconds %u\n",Time.getCurrentDate().c_str(),Time.getCurrentTime().c_str(),Time.GetMeasureSecond());
    fputs(buf,m_file);

    if (Measure.IsValid())
    {
    string meas;
    string mIp = string(Measure.Info.IP().c_str());
    string mPort = Measure.Info.Port().c_str();
    meas += "Measure: [" + mIp + ":" + mPort + "]\n";
    meas += "pkt    summ:" + Measure.Packets_Summary() + "\n";
    meas += "pkt      ok:" + Measure.Packets_ok() + "\n";
    meas += "pkt    drop:" + Measure.Packets_drop() + "\n";
    meas += "pkt reorder:" + Measure.Packets_reordered() + "\n";
    meas += "Err percent:" + Measure.ErrPercent() + "\n";
    meas += "---------------------------------------------\n";
    Measure.Reset();
    fputs(meas.c_str(),m_file);
    }

    sprintf(buf,"[traff,   cc] <= bad seconds\n");
    fputs(buf,m_file);

    for (auto & pAnalizer : pAnalizersArray) {

        size_t count_pmt = pAnalizer->CountPmt(); //count pmt in ip

        unsigned ts_cc_summary = pAnalizer->ts_statistic_period_final.summary_ccerrors;

         string textIp = string(pAnalizer->Info.IP().c_str());
         string textPort = pAnalizer->Info.Port().c_str();

         string str_addIp(15 - textIp.length(),' ');
         textIp    += str_addIp;

         // IF NO PMT
         if (count_pmt==0) {
             sprintf(buf,"%s %5s   ----  cc=%5u",textIp.c_str(),textPort.c_str(),ts_cc_summary);
             char buf1[256];
             sprintf(buf1,"[%5u,%5u]",
                     pAnalizer->ts_statistic_period_final.second_bad_traffic,
                     pAnalizer->ts_statistic_period_final.second_ccerror);
             char buf2[256];
             sprintf(buf2,"%u:[%s] (No PMT)",0,pAnalizer->Info.Name().c_str());
             fprintf(m_file,"%s   %s -> %s\n",buf1,buf,buf2);
             continue;
             }

         if (count_pmt==1) sprintf(buf,"%s %5s   SPTS  cc=%5u",textIp.c_str(),textPort.c_str(),ts_cc_summary);
         else              sprintf(buf,"%s %5s   MPTS  cc=%5u",textIp.c_str(),textPort.c_str(),ts_cc_summary);

        // all channels
        for (unsigned j=0; j<count_pmt; ++j) {
            TAnalizerES *ppmt = pAnalizer->GetPmt(j);
            if (!ppmt) continue;
            char buf1[256];
            sprintf(buf1,"[%5u,%5u]",
                    pAnalizer->ts_statistic_period_final.second_bad_traffic,
                    pAnalizer->ts_statistic_period_final.second_ccerror);
            char buf2[256];
            sprintf(buf2,"%u:[%s]",j+1,ppmt->PmtNameUI().c_str());
            fprintf(m_file,"%s   %s -> %s\n",buf1,buf,buf2);
            } // next pmt
        } // next ip

    fputs( "---------------------------------------------\n",m_file);

    fclose(m_file);
    chmod(filename.c_str(),0666);
}

void TStatistic::OutStateChannelsToLog()
{
    Log.File("state channels:\n");

    int count_checked_bad = 0;
    int count_checked_ok = 0;
    int count_unchecked_bad = 0;
    int count_unchecked_ok = 0;

    for (auto & pAnalizer : pAnalizersArray) {

        size_t count_pmt = pAnalizer->CountPmt();

        string textIpPort = string(pAnalizer->Info.IP().c_str()) + ":" + pAnalizer->Info.Port().c_str();
        //string str_add(20 - textIpPort.length(),' ');
        //textIpPort += str_add;

        if (count_pmt==0) {
            Log.ScrFile("%s %s - no pmt\n",textIpPort.c_str(),pAnalizer->Info.Name().c_str());
            ++count_unchecked_bad;
            continue;  // Next IP
            }

        for(unsigned k=0;k<count_pmt;k++) {

            TAnalizerES *ppmt = pAnalizer->GetPmt(k);
            if (!ppmt) continue;

            if (ppmt->StateUI() == UI_UNCHECKED) {
               // sprintf(buff,"%s:%d %s -> unchecked_ok",pts->TsIP().c_str(),pts->TsPort(),ppmt->ChannelName().c_str());
               // Log.ScrFile("%s\n",buff);
                ++count_unchecked_ok;
                continue;
                }

            if (ppmt->StateUI() == UI_STREAM_OK) {
               // sprintf(buff,"%s:%d %s -> count_checked_ok",pts->TsIP().c_str(),pts->TsPort(),ppmt->ChannelName().c_str());
               // Log.ScrFile("%s\n",buff);
                ++count_checked_ok;
                continue;
                }

            if (ppmt->StateUI() != UI_STREAM_OK) {
                // sprintf(buff,"%s:%d %s -> count_checked_bad",pts->TsIP().c_str(),pts->TsPort(),ppmt->ChannelName().c_str());
                // Log.ScrFile("%s\n",buff);
                ++count_checked_bad;
                continue;
                }

            } //Next PMT

        }  // Next IP


  Log.File("checked: %u ok, %u bad\n",count_checked_ok,count_checked_bad);
  Log.File("unchecked: %u ok, %u bad\n",count_unchecked_ok,count_unchecked_bad);
}
