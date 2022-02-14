#ifdef _WIN32
#include <Windows.h>
#endif

#include "RcbWifi.h"
#include "RcbWifiEditor.h"


//#include <stdio.h>
//#include <string>

bool initPassed = false;

uint32_t seqNum = 0;
uint32_t nextsn = 0;
uint32_t hit = 0;
uint32_t miss = 0;
uint32_t delayed = 0;
bool firstPacket = 1;
String s = "";

//String ipNumStr = "192.168.0.93";
String ipNumStr = "";
String myHostStr = "";
String rcbMsgStr = "";

using namespace RcbWifiNode;

DataThread* RcbWifi::createDataThread(SourceNode *sn)
{
    return new RcbWifi(sn);
}

//RcbWifi::RcbWifi(SourceNode* sn) : DataThread(sn)
RcbWifi::RcbWifi(SourceNode* sn) : DataThread(sn)//, port(DEFAULT_PORT)
{
    socket = new DatagramSocket();
    socket->bindToPort(port);
    connected = (socket->waitUntilReady(true, 1000) == 1); // Try to automatically open, dont worry if it does not work
    std::cout << "Connected = " + String(int(connected)) << std::endl;
    std::cout << "Port = " + String(port) << std::endl;
    CoreServices::sendStatusMessage("Connected = " + String(int(connected)));
  //  CoreServices::sendStatusMessage("Connected = " + ((connected)));

  //  sourceBuffers.add(new DataBuffer(num_channels, num_channels * num_samp * 4 * 1)); // start with 2 channels and automatically resize
    sourceBuffers.add(new DataBuffer(num_channels,10000)); // start with 2 channels and automatically resize
  //  recvbuf = (uint16_t *)malloc(num_channels * num_samp * 2);
 //   convbuf = (float *)malloc(num_channels * num_samp * 4);
  

  //  recvbuf = (uint16_t*)malloc(1444);
  //  convbuf = (float*)malloc(2496);

    //32 20k
   // recvbuf = (uint16_t*)malloc(1468);
    recvbuf = (uint16_t*)malloc(recvBufSize);
    std::cout << "recBufSize = " + String(recvBufSize) << std::endl;

  //  convbuf = (float*)malloc(2688);
    convbuf = (float*)malloc(convBufSize);
    std::cout << "convBufSize = " + String(convBufSize) << std::endl;
  
    timestamps.resize(num_samp);
    ttlEventWords.resize(num_samp);
}

std::unique_ptr<GenericEditor> RcbWifi::createEditor(SourceNode* sn)
{
    //return new RcbWifiEditor(sn, this);
    std::unique_ptr<RcbWifiEditor> editor = std::make_unique<RcbWifiEditor>(sn, this);
    return editor;
}

RcbWifi::~RcbWifi()
{
    LOGD("RCB WiFi interface destroyed.");
    free(recvbuf);
    free(convbuf);
    socket->shutdown();

    //signalThreadShouldExit();
    //notify();
}

void RcbWifi::resizeChanSamp()
{
   // sourceBuffers[0]->resize(num_channels, num_channels * num_samp * 4 * 5);
    sourceBuffers[0]->resize(num_channels, 10000);
  //  recvbuf = (uint16_t *)realloc(recvbuf, num_channels * num_samp * 2);
  //  convbuf = (float *)realloc(convbuf, num_channels * num_samp * 4);
    //timestamps.resize(num_samp);
    //ttlEventWords.resize(num_samp);
}

int RcbWifi::getNumChannels() const
{
    return num_channels;
}


/*  v5.5.53 not v6
int RcbWifi::getNumDataOutputs(DataChannel::DataChannelTypes type, int subproc) const
{
    if (type == DataChannel::HEADSTAGE_CHANNEL)
        return num_channels;
    else
        return 0; 
}  

int RcbWifi::getNumTTLOutputs(int subproc) const
{
    return 0; 
}

float RcbWifi::getSampleRate(int subproc) const
{
    return sample_rate;
}

float RcbWifi::getBitVolts (const DataChannel* ch) const
{
    return 0.195f;
}  */

void RcbWifi::updateSettings(OwnedArray<ContinuousChannel>* continuousChannels,
    OwnedArray<EventChannel>* eventChannels,
    OwnedArray<SpikeChannel>* spikeChannels,
    OwnedArray<DataStream>* sourceStreams,
    OwnedArray<DeviceInfo>* devices,
    OwnedArray<ConfigurationObject>* configurationObjects)
{
    std::cout << "In updateSettings()" << std::endl;

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
        continuousChannels->getLast()->setUnits("uV"); //?
    }

}


bool RcbWifi::foundInputSource()
{
    //std::cout << "foundInputSource initPassed = " + initPassed << std::endl;

    if (initPassed == true)
    {
        if (connected == 0)
        {
            tryToConnect();
        }
           //std::cout <<"Connected = " + String(int(connected)) << std::endl;
        return connected;
    }
}

/*
bool RcbWifi::isReady()
{
   // tryToConnect();
    if (initPassed == true)
    {
        return connected;
    }
    //return connected;
} */

bool RcbWifi::startAcquisition()
{
    //std::cout << "startAcquisition initPassed = " + String(int(initPassed)) << std::endl;
    if (initPassed == true)
    {
        //sourceBuffers[0]->clear();
        firstPacket = 1;
        hit = 0;
        miss = 0;
        delayed = 0;

      // josh  
        startTimer(2000);
        startThread();
        tryToConnect();

        // want to do this at init button
      //  URL urlInit("http://192.168.0.93/intan_status.html");
      //  auto result = getResultText(urlInit);
     //   std::cout << "msg is  " + result << std::endl;

     //   auto lines = StringArray::fromLines(result);
      ///  std::cout << "line 4 is  " + lines[4] << std::endl;
      //  std::cout << "line 8 is  " + lines[8] << std::endl;
      //  auto voltStr = lines[8].substring(11, 15);
      //  //batteryInfo = ("Battery(v)\n" + String(b, 2) + " V");
      //  //batteryLabel = new Label("batteryVolts", "RCB Battery\n0.0 volts");

     //   std::cout << "" + voltStr + " V" << std::endl;


        // send HTTP Post message to RCB - Init host ip and port 192.168.0.102:4416
        rcbMsgStr = "__SL_P_UUU=" + myHostStr;
        std::cout << "Host is  " + myHostStr << std::endl;
        std::cout << "msg is  " + rcbMsgStr << std::endl;
        // sendRCBTriggerPost(ipNumStr, "__SL_P_UUU=192.168.0.102:4416");
        sendRCBTriggerPost(ipNumStr, rcbMsgStr);

        // send HTTP Post message to RCB - RUN
        sendRCBTriggerPost(ipNumStr, "__SL_P_ULD=ON");
        //tryToConnect();

        std::cout << "Start Wifi.  " + ipNumStr << std::endl;
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

void  RcbWifi::tryToConnect()
{
    socket->shutdown();
    socket = new DatagramSocket();
    socket->setEnablePortReuse(true);
    bool bindOK = socket->bindToPort(port);
    connected = bindOK;
    //connected = (socket->waitUntilReady(true, 100) == 1);

   // CoreServices::sendStatusMessage("BindOk = " + String(int(connected)));

    //sendRCBTriggerPost(ipNumStr, "__SL_P_ULD=ON");
    isReady();
}

bool RcbWifi::isReady()
{
    // tryToConnect();
    if (initPassed == true)
    {
        return connected;
    }
    //return connected;
}

bool RcbWifi::stopAcquisition()
{
    seqNum = 0;
   // sendRCBTriggerPost(ipNumStr, "__SL_P_ULD=OFF");
   
    if (isThreadRunning())
    {
        signalThreadShouldExit();
        notify();
        std::cout << "thread should exit" << std::endl;
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
    sourceBuffers[0]->clear();
    stopTimer(); //josh

    firstPacket = 1;
    return true;
}


bool RcbWifi::updateBuffer()
{  
	// int rc = socket->read(recvbuf, num_channels * num_samp * 2, true);
	// int rc = socket->read(recvbuf, (num_channels + 2) * num_samp + 40, true); //1444 1468
	// int rc = socket->read(recvbuf, ((num_channels + 2) * num_samp * 2) + 40, true); //1444 1468
//	int rc = socket->read(recvbuf, 1468, true); //1444 1468
    int rc = socket->read(recvbuf, recvBufSize, true); //1444 1468

    //uint8* bytes = (uint8*)&recvbuf;
    //CoreServices::sendStatusMessage("mNum = " + (String(bytes[1])));

	if (rc == -1) {
		//CoreServices::sendStatusMessage("RC = " + String(rc));
        CoreServices::sendStatusMessage("RCB WiFi : Data shape mismatch"); 
		return false;
	}

    seqNum = ((uint32_t)recvbuf[5] << 16) + recvbuf[4];
    batteryVolts = recvbuf[18];
    //CoreServices::sendStatusMessage("recvbuf[4,5] = " + String(seqNum));

    if (firstPacket == 1)
    {
        nextsn = seqNum;
        hit = 0;
        miss = 0;
        delayed = 0;
        firstPacket = 0;
        magicNum = (uint8_t)(recvbuf[0] & 0x00ff);
      
        std::cout << "mNum = " + (String::toHexString(magicNum)) << std::endl;
    }

    if (nextsn == 0 || seqNum == nextsn) {
        nextsn = seqNum + 1;
        hit++;
    } 
    else if (seqNum < nextsn) {
        delayed++;
       // CoreServices::sendStatusMessage("delayed = " + String(delayed));
       
        std::cout << "delayed = " + String(delayed) << std::endl;
    }
    else {
        miss += seqNum - nextsn;
        nextsn = seqNum + 1;
        hit++;
    }

	// Transpose because the chunkSize arguement in addToBuffer does not seem to do anything

	int k = 0;
	for (int i = 0; i < num_samp; i++) {
		for (int j = 0; j < num_channels; j++) {
			convbuf[k++] = 0.195 * (float)(recvbuf[(j + 22) + (i * (num_channels + 2))] - 32768);
		}
	}

	//sourceBuffers[0]->addToBuffer(convbuf, &timestamps.getReference(0), &ttlEventWords.getReference(0), num_samp, 1);

    sourceBuffers[0]->addToBuffer(convbuf,
        timestamps.getRawDataPointer(),
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
    //CoreServices::sendStatusMessage("Send the POST.");
    //AlertWindow::showMessageBox(AlertWindow::NoIcon,
    //	"RCB-LVDS Module.",
    //	"IP:" + ipNumStr + "  Trigger:" +triggerChStr + "  Msg:" +msgStr ,
    //	"OK", 0);

    this->ipNumStr = ipNumStr;
    //URL urlPost("http://192.168.0.93");
    std::cout << "ipNumStr  " + ipNumStr << std::endl;
    URL urlPost("http://" + ipNumStr);

    int statusCode = 0;
    //urlPost = urlPost.withPOSTData("__SL_P_UDI=C" + triggerChStr);
    urlPost = urlPost.withPOSTData(msgStr);
   // ScopedPointer<InputStream> stream(urlPost.createInputStream(true, nullptr, nullptr, String(), 100, &responseHeaders, &statusCode, 5, "POST"));

    std::unique_ptr<InputStream> postStream(urlPost.createInputStream(true, nullptr, nullptr, String(), 100, &responseHeaders, &statusCode, 5, "POST"));
    if (postStream != nullptr)
    {
        postStream->readEntireStreamAsString();
        //CoreServices::sendStatusMessage("stream = " + stream->readString());
        //std::cout << "Post Stream = " + postStream->readString() << std::endl; 
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

String RcbWifi::getIntanStatusInfo()
{
    isGoodIntan = false;
    isGoodRCB = false;
    // want to do this at init button
    // fix so uses user input IP number

   // URL urlInit("http://192.168.0.93/intan_status.html");
    URL urlInit("http://" + ipNumStr + "/intan_status.html");

    auto result = getResultText(urlInit);
    std::cout << "msg is  " + result << std::endl;
    if (result.length() > 20)
    {
        //check that status code = 200, indicates RCB is available
        auto lines = StringArray::fromLines(result);
        std::cout << "line 0 is  " + lines[0] << std::endl;
        if (lines[0] == "Status code: 200")
        {
            //RCB is available
            isGoodRCB = true;

            // get RCB battery voltage
            auto voltStr = lines[8].substring(11, 15);
            // batteryStatusInfo = ("RCB Battery\n" + voltStr + " Volts");
           // batteryStatusInfo = ("" + voltStr + "V");
            batteryStatusInfo = ("Battery\n" + voltStr + "V\nOK");
            //batteryLabel = new Label("batteryVolts", "RCB Battery\n0.0 volts");
            std::cout << "RCB Battery = " + voltStr + "V" << std::endl;

            // format and display RHD registers 40-44,and 60-63.  See Intan RHD data
            // sheet page 23 for description of register values.

            // used to check that Intan RCH is connected and working ok
            auto intanStr = lines[12].substring(8, 28);
            const char* intanChar = static_cast<const char*>(intanStr.toRawUTF8());
            /// auto intanRaw = intanStr.toUTF8();  //sscanf(intanStr, "%4x", 5);
            std::cout << "str =" + intanStr << std::endl;
            //   std::cout << "char =" + String((intanChar)) << std::endl;
            if (intanStr == "0049004e00540041004e") // INTAN
            {
                isGoodIntan = true;
                std::cout << "isGoodIntan = " + String(int(isGoodIntan)) << std::endl;

                auto dieRev = lines[12].substring(30, 32);
                std::cout << "" + dieRev << std::endl;

                auto uniBi = lines[12].substring(34, 36);
                std::cout << "" + uniBi << std::endl;

                numAmps = (lines[12].substring(38, 40).getHexValue32());
                std::cout << "" + String(numAmps) << std::endl;
                //   auto foo =     numAmps.getHexValue32();
                //   std::cout << "foo = " + String(foo) << std::endl;

                auto chipId = lines[12].substring(42, 44);
                std::cout << "" + chipId << std::endl;

                if (chipId == "01")
                {
                    // is RHD2132
                    chipId = "RHD2132";
                }
                else if (chipId == "02")
                {
                    // is RHD2216
                    chipId = "RHD2216";
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

            if (isGoodIntan == true)
            {
                return batteryStatusInfo;
            }
            else
            {
                AlertWindow::showMessageBox(AlertWindow::NoIcon,
                    "Intan Headstage not found.",
                    "Please check SPI cable connections. \r\n\r\n"
                    "Press Init button to try again.",
                    "OK", 0);
                
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
    //batteryInfo = "none";
    // convert battery voltage from fixed point to float, accounting also for resistor divider
    float b = (0xfff & (batteryVolts >> 2)) * 1.467 / 4096 * 62.0 / 15.0;

   // batteryInfo = ("RCB Battery\n" + String(b, 2) + " Volts");
  //  batteryInfo = ("" + String(b, 2) + "V");
    batteryInfo = ("Battery\n" + String(b, 2) + "V\nOK");
    
    // batteryInfo.append((String(b, 2) + "volts"), 40);
  //   batteryInfo.append(( " volts"), 40);
     //CoreServices::sendStatusMessage("Volts = " + String(b));
    return batteryInfo;

}


// from Juce network demo 
String RcbWifi::getResultText(const URL& url)
{
    StringPairArray responseHeaders;
    int statusCode = 0;

   // ScopedPointer<InputStream> stream = url.createInputStream(false, nullptr, nullptr, String(), 1000, &responseHeaders, &statusCode);

    std::unique_ptr<InputStream> urlStream = url.createInputStream(false, nullptr, nullptr, String(), 200, &responseHeaders, &statusCode);

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