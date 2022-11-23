#ifdef _WIN32
#include <Windows.h>
#endif

#include "RcbWifi.h"
#include "RcbWifiEditor.h"


//#include <stdio.h>
//#include <string>

//#include <iostream>

bool initPassed = false;

uint8_t magicNum = 0;
uint32_t seqNum = 0;
uint32_t nextsn = 0;
uint32_t hit = 0;
uint32_t miss = 0;
uint32_t delayed = 0;
bool firstPacket = 1;

uint32_t bitrate;
int rhdNumChInt = 0;
int rhdFsInt = 0;
int numChannelsEnabled = 0;
int rhdNumSampPkt = 0;

String s = "";

//String ipNumStr = "192.168.0.93";
String ipNumStr = "";
String myHostStr = "";
String rcbMsgStr = "";
String rcbPaStr = "";
String rhdNumChStr = "";
String rhdFsStr = "";

// total number of 16-bit samples in this packet is numTs * (num chan + aux)
//int numTsItems[9] = { 21, 23, 27, 32, 39, 51, 71, 119, 179 }; // numCh[32,28, 24,20, 16, 12, 8, 4, 2];
int numTsItems[8] = { 21, 23, 27, 32, 39, 51, 71, 119 }; // numCh[32,28, 24,20, 16, 12, 8, 4];

int rhdNumTsItems = 0;

//  channel mask is sent to RCB during init so it knows which chaneels to send in UDP packet.
// channel mask should also be used to power down unused amplifier channels in RHD regs.  Not done yet.
String chMask[32] = { "1", "3", "7", "F",
"1F", "3F", "7F", "FF",
"1FF", "3FF", "7FF", "FFF",
"1FFF", "3FFF", "7FFF", "FFFF",
"1FFFF", "3FFFF", "7FFFF", "FFFFF",
"1FFFFF", "3FFFFF", "7FFFFF", "FFFFFF",
"1FFFFFF", "3FFFFFF", "7FFFFFF", "FFFFFFF",
"1FFFFFFF", "3FFFFFFF", "7FFFFFFF", "FFFFFFFF" };


//RHD lower bandwidth DAC register settings
//lowBwDac1 = [0x0D, 0x0F, 0x11, 0x12, 0x15, 0x19, 0x1C, 0x22, 0x2C, 0x30, 0x36, ...
//0x3E, 0x05, 0x12, 0x28, 0x14, 0x2A, 0x08, 0x09, 0x2C, 0x31, 0x23, 0x01, 0x38, 0x10];

//lowBwDac2 = [0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, ...
//0x00, 0x01, 0x01, 0x01, 0x02, 0x02, 0x03, 0x04, 0x06, 0x09, 0x11, 0x28, 0x36, 0x7C];

// not used
// lowBwDac3 = [0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, ...
//    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01];

//RHD upper bandwidth DAC register settings
/*upBwRh1Dac1 = [0x08, 0x0B, 0x11, 0x16, 0x21, 0x03, 0x0D, 0x1B, 0x01, 0x2E, 0x29, ...
0x1E, 0x06, 0x2A, 0x18, 0x2C, 0x26];

upBwRh1Dac2 = [0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x02, 0x02, 0x03, ...
0x05, 0x09, 0x0A, 0x0D, 0x11, 0x1A];

upBwRh2Dac1 = [0x04, 0x08, 0x10, 0x17, 0x25, 0x0D, 0x19, 0x2c, 0x17, 0x1E, 0x24, ...
0x2B, 0x02, 0x05, 0x07, 0x08, 0x05];

upBwRh2Dac2 = [0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x02, 0x03, 0x04, ...
0x06, 0x0B, 0x0D, 0x10, 0x15, 0x1F];
*/

using namespace RcbWifiNode;

DataThread* RcbWifi::createDataThread(SourceNode *sn)
{
    return new RcbWifi(sn);
}

//RcbWifi::RcbWifi(SourceNode* sn) : DataThread(sn)
RcbWifi::RcbWifi(SourceNode* sn) : DataThread(sn),
    port(DEFAULT_PORT),
    num_channels(DEFAULT_NUM_CHANNELS),
    num_samp(DEFAULT_NUM_SAMPLES),
    data_offset(DEFAULT_DATA_OFFSET),
    data_scale(DEFAULT_DATA_SCALE),
    sample_rate(DEFAULT_SAMPLE_RATE)
{
    socket = new DatagramSocket();
    socket->bindToPort(port);
    connected = (socket->waitUntilReady(true, 1000) == 1); // Try to automatically open, dont worry if it does not work
    std::cout << "[dspw]" << "Connected = " + String(int(connected)) << std::endl;
    std::cout << "[dspw]" << "Port = " + String(port) << std::endl;
    CoreServices::sendStatusMessage("Connected = " + String(int(connected)));
  
    sourceBuffers.add(new DataBuffer(num_channels,10000)); // start with 2 channels and automatically resize

  //  int recvBufSize = 40 + (((num_channels + 2) * num_samp) * 2);
 //   int convBufSize = 0 + (((num_channels)*num_samp) * 4);

    //32 20k
   // recvbuf = (uint16_t*)malloc(1468);  // correct for 32ch
    recvBufSize = 40 + (((num_channels + 2) * num_samp) * 2);
    recvbuf = (uint16_t*)malloc(recvBufSize);
    std::cout << "[dspw]" << "recBufSize = " + String(recvBufSize) << std::endl;

  //  convbuf = (float*)malloc(2688);
    convBufSize = 0 + (((num_channels)*num_samp) * 4);
    convbuf = (float*)malloc(convBufSize);
    std::cout << "[dspw]" << "convBufSize = " + String(convBufSize) << std::endl;
  
    //timestamps.resize(num_samp);
    //ttlEventWords.resize(num_samp);
}

std::unique_ptr<GenericEditor> RcbWifi::createEditor(SourceNode* sn)
{
    std::unique_ptr<RcbWifiEditor> editor = std::make_unique<RcbWifiEditor>(sn, this);

    return editor;
}

RcbWifi::~RcbWifi()
{
    //LOGD("RCB WiFi interface destroyed.");
    free(recvbuf);
    free(convbuf);
    socket->shutdown();

}

void RcbWifi::resizeChanSamp()
{
   // sourceBuffers[0]->resize(num_channels, num_channels * num_samp * 4 * 5);
    sourceBuffers[0]->resize(num_channels, 10000);

    std::cout << "[dspw]" << "num_channels = " + String(num_channels) << std::endl;
    std::cout << "[dspw]" << "num_samp = " + String(num_samp) << std::endl;

    recvBufSize = 40 + (((num_channels + 2) * num_samp) * 2);
    recvbuf = (uint16_t*)realloc(recvbuf,recvBufSize);
    std::cout << "[dspw]" << "resize recBufSize = " + String(recvBufSize) << std::endl;

    convBufSize = 0 + (((num_channels)*num_samp) * 4);
    convbuf = (float*)realloc(convbuf,convBufSize);
    std::cout << "[dspw]" << "resize convBufSize = " + String(convBufSize) << std::endl;

  //  recvbuf = (uint16_t *)realloc(recvbuf, num_channels * num_samp * 2);
  //  convbuf = (float *)realloc(convbuf, num_channels * num_samp * 4);

    sampleNumbers.resize(num_samp);
    timestamps.clear();
    timestamps.insertMultiple(0, 0.0, num_samp);
    ttlEventWords.resize(num_samp);

    timestamps.resize(num_samp);
    ttlEventWords.resize(num_samp);
}

// checked =
int RcbWifi::getNumChannels() const
{
    std::cout << "[dspw]" << "getNumChannels num_channels = " << String(num_channels) << std::endl;
    return num_channels;
}


void RcbWifi::updateSettings(OwnedArray<ContinuousChannel>* continuousChannels,
    OwnedArray<EventChannel>* eventChannels,
    OwnedArray<SpikeChannel>* spikeChannels,
    OwnedArray<DataStream>* sourceStreams,
    OwnedArray<DeviceInfo>* devices,
    OwnedArray<ConfigurationObject>* configurationObjects)
{
    std::cout << "[dspw]" << "In updateSettings()" << std::endl;

    //if (!deviceFound)
    //    return; // from rhythm

    // from rhythm. needed so do not create duplicate data streams.
    continuousChannels->clear();
    eventChannels->clear();
    spikeChannels->clear();
    sourceStreams->clear();
    devices->clear();
    configurationObjects->clear();
    
    //channelNames.clear();

    DataStream::Settings dataStreamSettings
    {
        "RCBWifiStream",
        "description",
        "identifier",

        sample_rate

    };

    std::cout << "[dspw]" << "updateSettings num_channels = "<< String(num_channels) << std::endl;
    DataStream* stream = new DataStream(dataStreamSettings);
    sourceStreams->add(stream);

    for (int ch = 0; ch < num_channels; ch++)
    {
        ContinuousChannel::Settings channelSettings{
            ContinuousChannel::Type::ELECTRODE,
            "CH" + String(ch + 1),
            "description",
            "identifier",

            data_scale,

            sourceStreams->getFirst()
        };

        continuousChannels->add(new ContinuousChannel(channelSettings));
       // continuousChannels->getLast()->setUnits("uV"); //?
    }

}


bool RcbWifi::foundInputSource()
{
    //std::cout << "[dspw]" << "foundInputSource initPassed = " + initPassed << std::endl;

    if (initPassed == true)
    {
        if (connected == 0)
        {
            tryToConnect();
        }
           //std::cout <<"[dspw]" << "Connected = " + String(int(connected)) << std::endl;
        return connected;
    }
}

void  RcbWifi::tryToConnect()
{
    socket->shutdown();
    socket = new DatagramSocket();
    socket->setEnablePortReuse(true);
    bool bound = socket->bindToPort(port);
    connected = bound;  // this needs more cleanup and thought

    if (bound)
    {
        std::cout << "[dspw]" << "Socket bound to port " << port << std::endl;
        // connected = (socket->waitUntilReady(true, 500) == 1); // ephysSocket has this but not good for RCB
    }
    else {
        std::cout << "[dspw]" << "Could not bind socket to port " << port << std::endl;
    }

    isReady();
}

bool RcbWifi::isReady()
{
    // tryToConnect();
    if (initPassed == true)
    {
        return connected;  // this needs more cleanup and thought
    }
    //return connected;
}



bool RcbWifi::startAcquisition()
{
    //std::cout << "[dspw]" << "startAcquisition initPassed = " + String(int(initPassed)) << std::endl;
    if (initPassed == true)
    {
        //sourceBuffers[0]->clear();
        firstPacket = 1;
        hit = 0;
        miss = 0;
        delayed = 0;

      // josh  
        resizeChanSamp();
        total_samples = 0;
        startTimer(2000);
        startThread();
        tryToConnect(); 

        // send HTTP Post message to RCB - Init host ip and port ex. 192.168.0.102:4416
        rcbMsgStr = "__SL_P_UUU=" + myHostStr;
        std::cout << "[dspw]" << "Host is  " + myHostStr << std::endl;
        std::cout << "[dspw]" << "msg is  " + rcbMsgStr << std::endl;
        // sendRCBTriggerPost(ipNumStr, "__SL_P_UUU=192.168.0.102:4416");
        sendRCBTriggerPost(ipNumStr, rcbMsgStr);

        // send HTTP Post message to RCB - RUN
        sendRCBTriggerPost(ipNumStr, "__SL_P_ULD=ON");

        std::cout << "[dspw]" << "Start Wifi UDP Stream.  " + ipNumStr << std::endl;
        return true;
    }
    else {
        AlertWindow::showMessageBox(AlertWindow::NoIcon,
            "RCB-LVDS Module not Initialized. \r\n"
            "Please check your IP and Host Address and Host Port settings. \r\n",
            "Press Init Button to restart.",
            "OK", 0);

        //return false;
    }
    return false;
}


bool RcbWifi::stopAcquisition()
{
    seqNum = 0;
   // sendRCBTriggerPost(ipNumStr, "__SL_P_ULD=OFF");
   
    if (isThreadRunning())
    {
        signalThreadShouldExit();
        notify();
        //std::cout << "[dspw]" << "thread should exit" << std::endl;
    }
  
    if (waitForThreadToExit(1000))
    {
        LOGC("RCB WiFi data thread exited.");
    }
    else
    {
        LOGC("RCB WiFi data thread failed to exit, continuing anyway...");
    }

    sendRCBTriggerPost(ipNumStr, "__SL_P_ULD=OFF");
   // Time::waitForMillisecondCounter(Time::getMillisecondCounter() + 1000);

  //  Time::waitForMillisecondCounter(1000);
    socket->shutdown(); // important to be after stopping thread
    //sourceBuffers[0]->clear();

    stopTimer(); //josh
    sourceBuffers[0]->clear();

    firstPacket = 1;
    return true;
}


bool RcbWifi::updateBuffer()
{  
    //std::cout << "[dspw]" << "updateBuffer recBufSize = " + String(recvBufSize) << std::endl;
    //std::cout << "[dspw]" << "updateBuffer convBufSize = " + String(convBufSize) << std::endl;
	// int rc = socket->read(recvbuf, num_channels * num_samp * 2, true);
	// int rc = socket->read(recvbuf, (num_channels + 2) * num_samp + 40, true); //1444 1468
	// int rc = socket->read(recvbuf, ((num_channels + 2) * num_samp * 2) + 40, true); //1444 1468
//	int rc = socket->read(recvbuf, 1468, true); //1444 1468
    int rc = socket->read(recvbuf, recvBufSize, true); //1444 1468

    //uint8* bytes = (uint8*)&recvbuf;
    //CoreServices::sendStatusMessage("mNum = " + (String(bytes[1])));

	if (rc == -1) {
		//CoreServices::sendStatusMessage("RC = " + String(rc));
       // CoreServices::sendStatusMessage("RCB WiFi : Data shape mismatch"); 
        std::cout << "[dspw]" << "RCB WiFi : Data shape mismatch " << std::endl;
		return false;
	}

    seqNum = ((uint32_t)recvbuf[5] << 16) + recvbuf[4];
    batteryVolts = recvbuf[18];
    //CoreServices::sendStatusMessage("recvbuf[4,5] = " + String(seqNum));

    if (firstPacket == 1)
    {
        std::cout << "[dspw]" << "updateBuffer recBufSize = " + String(recvBufSize) << std::endl;
        std::cout << "[dspw]" << "updateBuffer convBufSize = " + String(convBufSize) << std::endl;
        nextsn = seqNum;
        hit = 0;
        miss = 0;
        delayed = 0;
        firstPacket = 0;
        magicNum = (uint8_t)(recvbuf[0] & 0x00ff);
      
        std::cout << "[dspw]" << "mNum = " + (String::toHexString(magicNum)) << std::endl;
    }

    if (nextsn == 0 || seqNum == nextsn) {
        nextsn = seqNum + 1;
        hit++;
    } 
    else if (seqNum < nextsn) {
        delayed++;
       
        std::cout << "[dspw]" << "delayed = " + String(delayed) << std::endl;
    }
    else {
        miss += seqNum - nextsn;
        nextsn = seqNum + 1;
        hit++;
    }

    // using the transpose version from EphysSocket
	int k = 0;
	for (int i = 0; i < num_samp; i++) {
		for (int j = 0; j < num_channels; j++) {
			convbuf[k++] = 0.195 * (float)(recvbuf[(j + 22) + (i * (num_channels + 2))] - 32768);
		}
      //  timestamps.set(i, total_samples + i);  // timestamp needs be tied to seqnum
        //std::cout << timestamps[i]  << std::endl;

        sampleNumbers.set(i, total_samples + i);
        ttlEventWords.set(i, eventState);

        if ((total_samples + i) % 15000 == 0)
        {
            if (eventState == 0)
                eventState = 1;
            else
                eventState = 0;

            //std::cout << eventState << std::endl;
        }
	}

	//sourceBuffers[0]->addToBuffer(convbuf, &timestamps.getReference(0), &ttlEventWords.getReference(0), num_samp, 1);

    sourceBuffers[0]->addToBuffer(convbuf,
        sampleNumbers.getRawDataPointer(),
       // timestamps.getRawDataPointer(),
        ttlEventWords.getRawDataPointer(),
        num_samp,
        1);

    total_samples += num_samp;

    return true;
}

void RcbWifi::timerCallback()
{
    //  stopTimer();
}


void RcbWifi::sendRCBTriggerPost(String ipNumStr, String msgStr)
{
    this->ipNumStr = ipNumStr;
    //URL urlPost("http://192.168.0.93");
    //std::cout << "[dspw]" << "ipNumStr  " + ipNumStr << std::endl;
    URL urlPost("http://" + ipNumStr);

    int statusCode = 0;
    urlPost = urlPost.withPOSTData(msgStr);
   // ScopedPointer<InputStream> stream(urlPost.createInputStream(true, nullptr, nullptr, String(), 100, &responseHeaders, &statusCode, 5, "POST"));

    std::unique_ptr<InputStream> postStream(urlPost.createInputStream(true, nullptr, nullptr, String(), 100, &responseHeaders, &statusCode, 5, "POST"));
    if (postStream != nullptr)
    {
        postStream->readEntireStreamAsString();
        //std::cout << "[dspw]" << "Post Stream = " + postStream->readString() << std::endl; 
    }
    else
    {
        //enableState = false;
       // changeEnable(false);
        AlertWindow::showMessageBox(AlertWindow::NoIcon,
            "RCB-LVDS Module not found at IP address " + ipNumStr,
            "Please check your IP address setting. \r\n\r\n"
            "Press Init button to try again.",
            "OK", 0);
    }
}

void RcbWifi::setRCBTokens()
{
    // now that we have good RCB WiFi and good Intan, send multiple initialization http post messages to RCB WiFi Module

    // send HTTP Post message to RCB - Init host ip and port 192.168.0.102:4416
    rcbMsgStr = "__SL_P_UUU=" + myHostStr;
    std::cout << "[dspw]" << "Host is  " + myHostStr << std::endl;
    std::cout << "[dspw]" << "msg is  " + rcbMsgStr << std::endl;
    // sendRCBTriggerPost(ipNumStr, "__SL_P_UUU=192.168.0.102:4416");
    sendRCBTriggerPost(ipNumStr, rcbMsgStr);

    //set RCB WiFi Power Amp value
    rcbMsgStr = "__SL_P_UPA=" + rcbPaStr;
    std::cout << "[dspw]" << "RCB PA =  " + rcbPaStr << std::endl;
    sendRCBTriggerPost(ipNumStr, rcbMsgStr);

   // get number of channels from dropdown box
   // set rhd channel mask
    std::cout << "[dspw]" << "rhdNumChStr -  " + rhdNumChStr << std::endl;
    rhdNumChInt = rhdNumChStr.getIntValue();
    std::cout << "[dspw]" << "rhdNumChInt -  " << rhdNumChInt << std::endl;
    String rhdMaskStr = (chMask[rhdNumChInt - 1]) + " 6"; //the "6" is needed in all masks for correct aux sequence
    std::cout << "[dspw]" << "rhdMaskStr -  " << rhdMaskStr << std::endl;
    rcbMsgStr = "__SL_P_U00=" + rhdMaskStr;
    sendRCBTriggerPost(ipNumStr, rcbMsgStr);

    // set rhd aux channel sequence
    // need to figure out what we did in Qt GUI ??

    float actualFs = updateSampleRate();
    std::cout << "[dspw]" << "actualFs -  " << std::fixed << actualFs << std::endl;
    sample_rate = actualFs;

    // get number of samples in each UDP packet
    // used to compute size of recbuf and convbuff
    //recvBufSize = 40 + (((num_channels + 2) * num_samp) * 2);
    //convBufSize = 0 + (((num_channels)*num_samp) * 4);
    std::cout << "[dspw]" << "rhdNumTsItems -  " << rhdNumTsItems << std::endl;
    rhdNumSampPkt = numTsItems[rhdNumTsItems];
    std::cout << "[dspw]" << "rhdNumSampPkt -  " << rhdNumSampPkt << std::endl;
    num_channels = rhdNumChInt;
    num_samp = rhdNumSampPkt;
   // now call resize
    //CoreServices::updateSignalChain(this);
    resizeChanSamp();
    getNumChannels();

   
    // send SPI Bit Rate command to RCB
 //   sendRCBTriggerPost(ipNumStr, bitRateStr);
        // [s, status] = urlread(urlIpStr, 'post', { '__SL_P_URB', '6666666' });
    
        //[~, status] = urlread(urlIpStr, 'post', { '__SL_P_URB',bitRateStr }, 

    //uint32_t bitrate = 4e7 / divider;         // actual spi clk rate that is sent to RCB
    std::cout << "[dspw]" << "SPI bitrate -  " << bitrate << std::endl;
    String bitRateStr = String(bitrate);
    rcbMsgStr = "__SL_P_URB=" + bitRateStr;
    std::cout << "[dspw]" << "SPI bitRateStr -  " << bitRateStr << std::endl;
    sendRCBTriggerPost(ipNumStr, rcbMsgStr);

}

float RcbWifi::updateSampleRate()
{
    //set RCB SPI bit rate
    // Calc the SPI bit rate to use during streaming
    // convert sample rate to SPI bit rate
    // SPI word period Tw = 200 ns + 16.5Tb
    // Sample period = numChannelsEnabled Tw
    // We always also enable two additional slots per frame for aux sequences

    // get number of channels from dropdown box
    //String rcbNumCh = chanCbox->getText();
    std::cout << "[dspw]" << "rhdNumCh -  " + rhdNumChStr << std::endl;
    numChannelsEnabled = rhdNumChStr.getIntValue();
    std::cout << "[dspw]" << "rhdNumChInt -  " << numChannelsEnabled << std::endl;

    // get sample rate from dropdown box
    std::cout << "[dspw]" << "rhdFsStr -  " + rhdFsStr << std::endl;
    rhdFsInt = rhdFsStr.getIntValue();
    std::cout << "[dspw]" << "rhdFsInt -  " << rhdFsInt << std::endl;

    numChannelsEnabled += 2;
    
    float bitrequest = 16.5 * numChannelsEnabled * rhdFsInt / (1.0 - 2e-7 * numChannelsEnabled * rhdFsInt);

    // compute actual bit rate and return
    // replicate computations on rcblvds module
    uint32_t divider = roundf(4e7 / bitrequest);
    if (divider < 2) divider = 2;
    std::cout << "[dspw]" << "divider -  " << divider << std::endl;

    bitrate = 4e7 / divider;         // actual spi clk rate that is sent to RCB
    std::cout << "[dspw]" << "bitrate -  " << bitrate << std::endl;

    double Ts;
    if (0 == (divider & 1))
    {// clock divider is even, use 200ns delay
        Ts = numChannelsEnabled * (200e-9 + 16.5 / bitrate);   // actual sample period
        std::cout << "[dspw]" << "Ts even -  " << std::fixed << + Ts << std::endl;
    }
    else
    {// clock divider is odd, use 187.5ns delay
        Ts = numChannelsEnabled * (187.5e-9 + 16.5 / bitrate);   // actual sample period
        std::cout << "[dspw]" << "Ts odd -  " << std::fixed << + Ts << std::endl;
    }

   // somehow return [bitrate, adcBias, muxBias, dspCutoff] = 

    return 1.0 / Ts;      // actual sample rate

  //  bitRateStr = sprintf('%d', bitrate);
   // fprintf('SPI bitRateStr - %s\n', bitRateStr);
    //fprintf('ADC Bias - %d\n', adcBias);
    //fprintf('Mux Bias - %d\n', muxBias);
    //fprintf('DSP Cutoff - %x\n', dspCutoff);

}


String RcbWifi::getIntanStatusInfo()
{
    isGoodIntan = false;
    isGoodRCB = false;

   // URL urlInit("http://192.168.0.93/intan_status.html");
    URL urlInit("http://" + ipNumStr + "/intan_status.html");

    auto result = getResultText(urlInit);
    std::cout << "[dspw]" << "msg is  " + result << std::endl;
    if (result.length() > 20)
    {
        //check that status code = 200, indicates RCB is available
        auto lines = StringArray::fromLines(result);
        std::cout << "line 0 is  " + lines[0] << std::endl;
        if (lines[0] == "Status code: 200")
        {
            //RCB is available at specified IP Addr
            isGoodRCB = true;

            // get RCB battery voltage
            auto voltStr = lines[8].substring(11, 15);
            batteryStatusInfo = ("Battery\n" + voltStr + "V\nOK");
            //batteryLabel = new Label("batteryVolts", "RCB Battery\n0.0 volts");
            std::cout << "[dspw]" << "RCB Battery = " + voltStr + "V" << std::endl;

            // format and display RHD registers 40-44,and 60-63.  See Intan RHD data
            // sheet page 23 for description of register values.

            // used to check that Intan RHD is connected and we can read regs over SPI
            auto intanStr = lines[12].substring(8, 28);
            const char* intanChar = static_cast<const char*>(intanStr.toRawUTF8());
            /// auto intanRaw = intanStr.toUTF8();  //sscanf(intanStr, "%4x", 5);
            std::cout << "str =" + intanStr << std::endl;
            //   std::cout << "char =" + String((intanChar)) << std::endl;
            if (intanStr == "0049004e00540041004e") // Spells INTAN
            {
                isGoodIntan = true;
                std::cout << "[dspw]" << "isGoodIntan = " + String(int(isGoodIntan)) << std::endl;

                auto dieRev = lines[12].substring(30, 32);
                std::cout << "[dspw]" << "" + dieRev << std::endl;

                auto uniBi = lines[12].substring(34, 36);
                std::cout << "[dspw]" << "" + uniBi << std::endl;

                numAmps = (lines[12].substring(38, 40).getHexValue32());
                std::cout << "[dspw]" << "" + String(numAmps) << std::endl;
                //   auto foo =     numAmps.getHexValue32();
                //   std::cout << "foo = " + String(foo) << std::endl;

                auto chipId = lines[12].substring(42, 44);
                std::cout << "[dspw]" << "" + chipId << std::endl;

                if (chipId == "01")
                {
                    // is RHD2132
                    chipId = "RHD2132";
                    int maxNumCh = 32;  // need to expand on this to limit and set up the channels cbox
                }
                else if (chipId == "02")
                {
                    // is RHD2216
                    chipId = "RHD2216";
                    int maxNumCh = 16;
                }

                if (uniBi == "00")
                {
                    // is uni polar
                    uniBi = "Diff";
                }
                else if (uniBi == "01")
                {
                    // is differential
                    uniBi = "UniPolar";
                }

                rhdStatusInfo = (chipId + "\n" + String(numAmps) + " Ch\n" + uniBi);
              //  rhdStatusInfo = (chipId + " " + String(numAmps) + "Ch " + uniBi);
                //auto myIntanIs = sprintf("%s - %s - %s - %s ", dieRev, uniBi, numAmps, chipId);
                //std::cout << "" + myIntanIs << std::endl;
            }
            else
            {
                isGoodIntan = false;
                //rhdStatusInfo = ("RCB init failed.");
                AlertWindow::showMessageBox(AlertWindow::NoIcon,
                    "Intan Headstage not found.",
                    "Please check SPI cable connections. \r\n\r\n"
                    "Press Init button to try again.",
                    "OK", 0);

                return "Intan init failed.";

                //std::cout << "isGoodIntan = " + String(int(isGoodIntan)) << std::endl;
            }

            // none of below is needed ??
            if (isGoodIntan == true)
            {
                return batteryStatusInfo;  /// why ???
            }
            else
            {
                AlertWindow::showMessageBox(AlertWindow::NoIcon,
                    "Intan Headstage not found.",
                    "Please check SPI cable connections. \r\n\r\n"
                    "Press Init button to try again.",
                    "OKay", 0);
                
                return "Intan init failed.";
            }
               
        }
    } else 
    { 
        AlertWindow::showMessageBox(AlertWindow::NoIcon,
            "RCB-LVDS Module not found at IP address " + ipNumStr,
            "Please check RCB IP Address setting,\nWiFi router configuration,\nand RCB battery power.\r\n\r\n"
            "Press Init button to try again.",
            "OK", 0);

        return "RCB init failed.";
    }
   // return isGoodIntan;
}



String RcbWifi::getPacketInfo()
{
    packetInfo = ("Packet Info\nSQ N-" + String(seqNum));
    packetInfo.append(("\nGood-" + String(hit)), 100);
    packetInfo.append(("\nMiss-" + String(miss)), 100);
    //return ("SN - " + String(seqNum));
    return packetInfo;
}

String RcbWifi::getBatteryInfo()
{
    // convert battery voltage from fixed point to float, accounting also for resistor divider
    float b = (0xfff & (batteryVolts >> 2)) * 1.467 / 4096 * 62.0 / 15.0;

    batteryInfo = ("Battery\n" + String(b, 2) + "V\nOK");

    return batteryInfo;
}


// from Juce network demo 
String RcbWifi::getResultText(const URL& url)
{
    StringPairArray responseHeaders;
    int statusCode = 0;

    //ScopedPointer<InputStream> stream = url.createInputStream(false, nullptr, nullptr, String(), 1000, &responseHeaders, &statusCode);

    std::unique_ptr<InputStream> urlStream = url.createInputStream(false, nullptr, nullptr, String(), 100, &responseHeaders, &statusCode);

    //if (auto stream = url.createInputStream(false, nullptr, nullptr, String(), 1000, &responseHeaders, &statusCode))
    if (urlStream != nullptr)
    {
        return (statusCode != 0 ? "Status code: " + String(statusCode) + newLine : String())
            + "Response headers: " + newLine
            + responseHeaders.getDescription() + newLine
            + "----------------------------------------------------" + newLine
            + urlStream->readEntireStreamAsString();
    }

    if (statusCode != 0)
        return "Failed to connect, status code = " + String(statusCode);

    return "Failed to connect!";
}

/*
if (sampleRate < 3334.0) {
    muxBias = 40;
    adcBufferBias = 32;
}
else if (sampleRate < 4001.0) {
    muxBias = 40;
    adcBufferBias = 16;
}
else if (sampleRate < 5001.0) {
    muxBias = 40;
    adcBufferBias = 8;
}
else if (sampleRate < 6251.0) {
    muxBias = 32;
    adcBufferBias = 8;
}
else if (sampleRate < 8001.0) {
    muxBias = 26;
    adcBufferBias = 8;
}
else if (sampleRate < 10001.0) {
    muxBias = 18;
    adcBufferBias = 4;
}
else if (sampleRate < 12501.0) {
    muxBias = 16;
    adcBufferBias = 3;
}
else if (sampleRate < 15001.0) {
    muxBias = 7;
    adcBufferBias = 3;
}
else {
    muxBias = 4;
    adcBufferBias = 2;
}
*/