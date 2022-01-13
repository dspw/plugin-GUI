#ifndef __RCBWIFIH__
#define __RCBWIFIH__

#include <DataThreadHeaders.h>
#include <ProcessorHeaders.h>

//#include "../../Source/Utils/Utils.h"

//#include "rhythm-api/rhd2000evalboard.h"
#include "rhd2000registers.h"
//#include "rhythm-api/rhd2000datablock.h"
//#include "rhythm-api/okFrontPanelDLL.h"

#include <list>
#include <vector>
#include <string>
#include <iostream>
#include <string>

// new from ephysSocket
const int DEFAULT_PORT = 4416;
const float DEFAULT_SAMPLE_RATE = 20768.433f;
const float DEFAULT_DATA_SCALE = 0.195f;
const uint16_t DEFAULT_DATA_OFFSET = 32768;
const int DEFAULT_NUM_SAMPLES = 21;
const int DEFAULT_NUM_CHANNELS = 32;
// end

const int DEFAULT_UPPERBW = 7500;
const int DEFAULT_LOWERBW = 7500;

//#define CHIP_ID_RHD2132  1
//#define CHIP_ID_RHD2216  2

namespace RcbWifiNode
{
    
    class RcbWifi : public DataThread, public Timer
    {

    public:
        RcbWifi(SourceNode* sn);
        ~RcbWifi();

        // Interface fulfillment
        bool foundInputSource() override;
        int getNumDataOutputs(DataChannel::DataChannelTypes type, int subProcessor) const override;
        int getNumTTLOutputs(int subprocessor) const override;
        float getSampleRate(int subprocessor) const override;
        float getBitVolts(const DataChannel* chan) const override;
        int getNumChannels() const;

        // User defined
        String myHostStr;
        String rcbMsgStr;
        String ipNumStr;

        bool isGoodIntan;
        bool initPassed;
        int port = 4416;
       // int port;
        float sample_rate =  20768.433; //10010.010; // 20768.433; // 30e3; 20639.834;
        int upperBw = 7500;
        int lowerBw = 1;
        bool transpose =  true;
        int num_samp = 21; // 21; // 39; //250
        int num_channels = 32;

        // new from ephysSocket
        int64 total_samples;
        float relative_sample_rate;
        // end

        void resizeChanSamp();
        void tryToConnect();

        GenericEditor* createEditor(SourceNode* sn);
        static DataThread* createDataThread(SourceNode* sn);

        int recvBufSize = 40 + (((num_channels + 2) * num_samp) * 2);
        int convBufSize = 0 + (((num_channels) * num_samp) * 4);

        String getIntanStatusInfo();
        String batteryStatusInfo;
        String getPacketInfo();
        String packetInfo;
        String getBatteryInfo();
        uint16_t batteryVolts;
        String batteryInfo;

        uint8_t magicNum;
        uint32_t seqNum;  
        uint32_t nextsn;
        uint32_t hit;
        uint32_t miss;
        uint32_t delayed;
        
        bool firstPacket;

      //  uint32_t nextsn = 0;
     //   uint32_t hit = 0;
      //  uint32_t miss = 0;
      //  uint32_t delayed = 0;
      //  String packetInfo = "";
      //  bool firstPacket = 1;
      //  uint32_t seqNum = 0;

        
// did not get used
        struct rcbReturns
        {
            uint16_t vbat;
            uint32_t seqNum;
            uint32_t hit;
            uint32_t miss;
            uint32_t delayed;
        };

        void sendRCBTriggerPost(String ipNumStr, String msgStr);

        String getResultText (const URL& url);

    private:
      
        bool updateBuffer() override;
        bool startAcquisition() override;
        bool stopAcquisition()  override;
        void timerCallback() override;
        bool isReady() override;
       
        bool connected =  false;

        

       ScopedPointer<DatagramSocket> socket;

        uint16_t *recvbuf;
        float *convbuf;

        //String ipNumStr = "192.168.0.93";
       // String ipNumStr;
        StringPairArray responseHeaders;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RcbWifi);
    };
}
#endif