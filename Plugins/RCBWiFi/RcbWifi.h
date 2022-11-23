#ifndef __RCBWIFIH__
#define __RCBWIFIH__

#include <DataThreadHeaders.h>
#include <ProcessorHeaders.h>

//#include "rhythm-api/rhd2000evalboard.h"
//#include "rhd2000registers.h"
//#include "rhythm-api/rhd2000datablock.h"
//#include "rhythm-api/okFrontPanelDLL.h"

#include <list>
#include <vector>
#include <string>
#include <iostream>


// new from ephysSocket
const int DEFAULT_PORT = 4416;
const float DEFAULT_SAMPLE_RATE = 20639.834; //for 16 at 20k 20768.433f;
const float DEFAULT_DATA_SCALE = 0.195f;
const uint16_t DEFAULT_DATA_OFFSET = 32768;
const int DEFAULT_NUM_SAMPLES = 21; // this is num samples in udp rxbuffer.  
const int DEFAULT_NUM_CHANNELS = 32; // 32;
// end

const int DEFAULT_UPPERBW = 7500;
const int DEFAULT_LOWERBW = 1;

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
        
        void updateSettings(OwnedArray<ContinuousChannel>* continuousChannels,
            OwnedArray<EventChannel>* eventChannels,
            OwnedArray<SpikeChannel>* spikeChannels,
            OwnedArray<DataStream>* sourceStreams,
            OwnedArray<DeviceInfo>* devices,
            OwnedArray<ConfigurationObject>* configurationObjects);

        int getNumChannels() const;

        // User defined
        int port = 4416;
        float sample_rate = 20639.834; //10010.010; // 20768.433; // 30e3; 20639.834;
        float data_scale;
        uint16_t data_offset;

        bool transpose = true;
        int num_samp; // = 21; // 21; // 39; //250
        int num_channels; // = 32;
        int rhdNumTsItems;

        int rhdNumChInt;
        int rhdFsInt;
        int numChannelsEnabled;
        int rhdNumSampPkt;
        uint32_t bitrate;

        int64 total_samples;
        float relative_sample_rate;

        uint64 eventState;
        
        void resizeChanSamp(); //oe
        void tryToConnect(); //oe

        std::unique_ptr<GenericEditor> createEditor(SourceNode* sn); //oe
        static DataThread* createDataThread(SourceNode* sn); //oe

        // dspw User defined
        String ipNumStr;
        String myHostStr;
        String rcbMsgStr;
        String rcbPaStr;
        String rhdNumChStr;
        String rhdFsStr;

        bool isGoodIntan;
        bool isGoodRCB;
        bool initPassed;
        bool firstPacket;
        int upperBw = 7500;
        int lowerBw = 1;
        int numAmps;
        String chipId;

        int recvBufSize;// = 40 + (((num_channels + 2) * num_samp) * 2);
        int convBufSize;// = 0 + (((num_channels) * num_samp) * 4);

        //int recvBufSize = 40 + (((num_channels + 2) * num_samp) * 2);
       // int convBufSize = 0 + (((num_channels)*num_samp) * 4);

        String getIntanStatusInfo();
        String batteryStatusInfo;
        String rhdStatusInfo;
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
        
        
// struct did not get used
        struct rcbReturns
        {
            uint16_t vbat;
            uint32_t seqNum;
            uint32_t hit;
            uint32_t miss;
            uint32_t delayed;
        };

        void RcbWifi::setRCBTokens();
        void sendRCBTriggerPost(String ipNumStr, String msgStr);
        String getResultText (const URL& url);
        bool isReady();
        float updateSampleRate();

    private:
      
        bool updateBuffer() override;
        bool startAcquisition() override;
        bool stopAcquisition()  override;
        void timerCallback() override;
       
        bool connected =  false;

       ScopedPointer<DatagramSocket> socket;

        uint16_t *recvbuf;
        float *convbuf;

        Array<int64> sampleNumbers;
        Array<double> timestamps;
        Array<uint64> ttlEventWords;

        int64 currentTimestamp;

        //bool isReady() override;
        //String ipNumStr = "192.168.0.93";
       // String ipNumStr;
        StringPairArray responseHeaders;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RcbWifi);
    };
}
#endif