#ifdef _WIN32
#include <Windows.h>
#endif

#include "RcbWifi.h"
#include "RcbWifiEditor.h"

//#include "../../Source/Utils/Utils.h"

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
    CoreServices::sendStatusMessage("Connected = " + String(connected));
  //  sourceBuffers.add(new DataBuffer(num_channels, num_channels * num_samp * 4 * 1)); // start with 2 channels and automatically resize
    sourceBuffers.add(new DataBuffer(num_channels,10000)); // start with 2 channels and automatically resize
  //  recvbuf = (uint16_t *)malloc(num_channels * num_samp * 2);
 //   convbuf = (float *)malloc(num_channels * num_samp * 4);
  

  //  recvbuf = (uint16_t*)malloc(1444);
  //  convbuf = (float*)malloc(2496);

    //32 20k
   // recvbuf = (uint16_t*)malloc(1468);
    recvbuf = (uint16_t*)malloc(recvBufSize);

  //  convbuf = (float*)malloc(2688);
    convbuf = (float*)malloc(convBufSize);
  
}

GenericEditor* RcbWifi::createEditor(SourceNode* sn)
{
    return new RcbWifiEditor(sn, this);
}

RcbWifi::~RcbWifi()
{
    free(recvbuf);
    free(convbuf);
    socket->shutdown();
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
        //   CoreServices::sendStatusMessage("Connected = " + String(connected));
        return connected;
    }
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

bool RcbWifi::startAcquisition()
{
    std::cout << "startAcquisition initPassed = " + String(initPassed) << std::endl;
    if (initPassed == true)
    {

        //sourceBuffers[0]->clear();
        firstPacket = 1;
        hit = 0;
        miss = 0;
        delayed = 0;

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

String RcbWifi::getIntanStatusInfo()
{
    isGoodIntan = false;
    // want to do this at init button
    URL urlInit("http://192.168.0.93/intan_status.html");
    auto result = getResultText(urlInit);
    std::cout << "msg is  " + result << std::endl;

    auto lines = StringArray::fromLines(result);
    //std::cout << "line 4 is  " + lines[4] << std::endl;
    //std::cout << "line 8 is  " + lines[8] << std::endl;
    auto voltStr = lines[8].substring(11, 15);
    batteryStatusInfo = ("Battery(v)\n" + voltStr + " V");
    //batteryLabel = new Label("batteryVolts", "RCB Battery\n0.0 volts");
    std::cout << "" + voltStr + " V" << std::endl;

// format and display RHD registers 40-44,and 60-63.  See Intan RHD data
       // sheet page 23 for description of register values.
    
    
    auto intanStr = lines[12].substring(8, 28);
    const char* intanChar = static_cast<const char*>(intanStr.toRawUTF8());
   /// auto intanRaw = intanStr.toUTF8();  //sscanf(intanStr, "%4x", 5);
    std::cout << "str =" + intanStr << std::endl;
 //   std::cout << "char =" + String((intanChar)) << std::endl;
    if (intanStr == "0049004e00540041004e") // INTAN
    {
        isGoodIntan = true;
        std::cout << "isGoodIntan = " + String(isGoodIntan) << std::endl;

        auto dieRev = lines[12].substring(30, 32);
        std::cout << "" + dieRev << std::endl;

        auto uniBi = lines[12].substring(34,36);
        std::cout << "" + uniBi << std::endl;

        auto numAmps = (lines[12].substring(38, 40).getHexValue32());
        std::cout << "" + String(numAmps) << std::endl;
     //   auto foo =     numAmps.getHexValue32();
     //   std::cout << "foo = " + String(foo) << std::endl;

        auto chipId = lines[12].substring(42, 44);
        std::cout << "" + chipId << std::endl;

        //auto myIntanIs = sprintf("%s - %s - %s - %s ", dieRev, uniBi, numAmps, chipId);
        //std::cout << "" + myIntanIs << std::endl;
    }
    else
    {
        isGoodIntan = false;
        std::cout << "isGoodIntan = " + String(isGoodIntan) << std::endl;
    }

    




    /*rcbStatStr = splitlines(status);
        fprintf('rcbStatStr9 - %s\n',rcbStatStr{9});
        myStat9 = rcbStatStr{9};
        
        isIntan = dec2hex(sscanf(myStat9(9:28),'%4x',5));
        intanStr = sprintf('%s',char(hex2dec(isIntan)));
        
        dieRev = sscanf(myStat9(29:32),'%x4x',1);
        myDieRev = sprintf('%d',dieRev);
        
        uniBi = sscanf(myStat9(33:36),'%4x',1);
        myUniBi = sprintf('%d',uniBi);
        
        numAmps = sscanf(myStat9(37:40),'%4x',1);
        myNumAmps = sprintf('%d',numAmps);
        
        chipId = sscanf(myStat9(41:44),'%4x',1);
        myChipId = sprintf('%d',chipId);
        
        myIntan = sprintf('%s - %s - %s - %s - %s ',...
            intanStr,myDieRev,myUniBi,myNumAmps,myChipId);
        set(hs.statusText, 'String', myIntan);
        fprintf('%s\n',myIntan);

*/



    return batteryStatusInfo;
}

String RcbWifi::getPacketInfo()
{
    packetInfo = ("Packet Info\nSQN - " + String(seqNum));
    packetInfo.append(("\nGood - " + String(hit)), 100);
    packetInfo.append(("\nMiss - " + String(miss)), 100);
    //return ("SN - " + String(seqNum));
    return packetInfo;
}

String RcbWifi::getBatteryInfo()
{
    //batteryInfo = "none";
    // convert battery voltage from fixed point to float, accounting also for resistor divider
    float b = (0xfff & (batteryVolts >> 2)) * 1.467 / 4096 * 62.0 / 15.0;

    batteryInfo = ("Battery(v)\n" + String(b,2) + " V");
   // batteryInfo.append((String(b, 2) + "volts"), 40);
 //   batteryInfo.append(( " volts"), 40);
    //CoreServices::sendStatusMessage("Volts = " + String(b));
    return batteryInfo;

}

void  RcbWifi::tryToConnect()
{
    socket->shutdown();
    socket = new DatagramSocket();
    socket->setEnablePortReuse(true);
    bool bindOK = socket->bindToPort(port);
    connected = bindOK;
    //connected = (socket->waitUntilReady(true, 100) == 1);
    CoreServices::sendStatusMessage("BindOk = " + String(connected));
    //sendRCBTriggerPost(ipNumStr, "__SL_P_ULD=ON");
    //isReady();
}

bool RcbWifi::stopAcquisition()
{
    seqNum = 0;
    sendRCBTriggerPost(ipNumStr, "__SL_P_ULD=OFF");
    socket->shutdown();
    if (isThreadRunning())
    {
        signalThreadShouldExit();
    }

    bool wThread = waitForThreadToExit(500);
    //CoreServices::sendStatusMessage("Thread = " + String(wThread));
    
  //  Time::waitForMillisecondCounter(1000);
    sourceBuffers[0]->clear();
   
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
		CoreServices::sendStatusMessage("RC = " + String(rc));
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
      //  CoreServices::sendStatusMessage("mNum = " + (String::toHexString(magicNum)));
    }

    if (nextsn == 0 || seqNum == nextsn) {
        nextsn = seqNum + 1;
        hit++;
    } 
    else if (seqNum < nextsn) {
        delayed++;
        CoreServices::sendStatusMessage("delayed = " + String(delayed));
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

	sourceBuffers[0]->addToBuffer(convbuf, &timestamps.getReference(0), &ttlEventWords.getReference(0), num_samp, 1);

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
    ScopedPointer<InputStream> stream(urlPost.createInputStream(true, nullptr, nullptr, String(), 100, &responseHeaders, &statusCode, 5, "POST"));
    if (stream != nullptr)
    {
        stream->readEntireStreamAsString();
        //CoreServices::sendStatusMessage("stream = " + stream->readString());
        std::cout << "Stream = " + stream->readString() << std::endl; 
    }
    else
    {
        //enableState = false;
       // changeEnable(false);
        AlertWindow::showMessageBox(AlertWindow::NoIcon,
            "RCB-LVDS Module not found at IP address " + ipNumStr,
            "Please check your IP address setting. \r\n"
            "Press Enable button to restart.",
            "OK", 0);
    }
}

// from Juce network demo 
String RcbWifi::getResultText(const URL& url)
{
    StringPairArray responseHeaders;
    int statusCode = 0;

    ScopedPointer<InputStream> stream = url.createInputStream(false, nullptr, nullptr, String(), 1000, &responseHeaders, &statusCode);

    //if (auto stream = url.createInputStream(false, nullptr, nullptr, String(), 1000, &responseHeaders, &statusCode))
    if (stream != nullptr)
    {
        return (statusCode != 0 ? "Status code: " + String(statusCode) + newLine : String())
            + "Response headers: " + newLine
            + responseHeaders.getDescription() + newLine
            + "----------------------------------------------------" + newLine
            + stream->readEntireStreamAsString();
    }

    if (statusCode != 0)
        return "Failed to connect, status code = " + String(statusCode);

    return "Failed to connect!";
}