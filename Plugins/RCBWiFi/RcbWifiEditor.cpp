// DSPW

#include "RcbWifiEditor.h"
#include "RcbWifi.h"

#include <string>
#include <iostream>

using namespace RcbWifiNode;

RcbWifiEditor::RcbWifiEditor(GenericProcessor* parentNode, RcbWifi *socket) : GenericEditor(parentNode, false)  //false)
{
    node = socket;
    desiredWidth = 460;

    //initPassed = false;

  //  rcbDrawerButton = new RcbDrawerButton("RcbDrawer");
  //  rcbDrawerButton->setBounds(4, 40, 10, 78);
  //  rcbDrawerButton->addListener(this);
  //  addAndMakeVisible(rcbDrawerButton);

    runRCB = new UtilityButton("Run", Font(Font::getDefaultSerifFontName(), 15, Font::plain));
    runRCB->setBounds(70, 30, 50, 18);
    runRCB->addListener(this);
    //addAndMakeVisible(runRCB);

    stopRCB = new UtilityButton("Stop", Font("Default", 15, Font::plain));
    stopRCB->setBounds(135, 30, 50, 18);
    stopRCB->addListener(this);
    //addAndMakeVisible(stopRCB);

    // Add test button
    testButton = new UtilityButton("INIT", Font("Small Text", 13, Font::bold));
    testButton->setBounds(67, 107, 38, 18);
    testButton->addListener(this);
    addAndMakeVisible(testButton);
    

    // Add connect button
  //  connectButton = new UtilityButton("Init", Font("default", 13, Font::bold));
 //   connectButton->setBounds(5, 30, 50, 18);
 //   connectButton->setColour(UtilityButton::Button::, Colours::black)
 //   connectButton->addListener(this);
 //   addAndMakeVisible(connectButton);

   

    //---

    // Num chans
    chanLabel = new Label("Num. Ch.", "Channels");
    chanLabel->setFont(Font(Font::getDefaultSerifFontName(), 12, Font::plain));
    chanLabel->setBounds(185, 62, 75, 12);
    chanLabel->setColour(Label::textColourId, Colours::black);
    addAndMakeVisible(chanLabel);

   // chanText = new TextEditor("Num. Ch.");
   // chanText->setFont(Font(Font::getDefaultSerifFontName(), 12, Font::plain));
   // chanText->setText(std::to_string(node->num_channels));
   // chanText->setBounds(219, 74, 45, 15);
   // addAndMakeVisible(chanText);

    chanCbox = new ComboBox();
    chanCbox->setBounds(189, 74, 50, 17);
    chanCbox->addListener(this);
    for (int i = 1; i < 9; i++)
        chanCbox->addItem(String(i * 4), i ); // start numbering at one for
    chanCbox->setSelectedId(1, dontSendNotification);
    addAndMakeVisible(chanCbox);

    // Num samples
 //   sampLabel = new Label("Num. Samp.", "Num. Samp.");
 //   sampLabel->setFont(Font("Small Text", 10, Font::plain));
 //   sampLabel->setBounds(80, 60, 65, 8);
 //   sampLabel->setColour(Label::textColourId, Colours::darkgrey);
   // addAndMakeVisible(sampLabel);

 //   sampText = new TextEditor("Num. Samp.");
 //   sampText->setFont(Font("Small Text", 10, Font::plain));
 //   sampText->setText(std::to_string(node->num_samp));
 //   sampText->setBounds(80, 70, 55, 15);
   // addAndMakeVisible(sampText);
   
    //  Desired Sample Rate  Fs
    fsLabel = new Label("Fs (Hz)", "Fs (Hz)");
    fsLabel->setFont(Font(Font::getDefaultSerifFontName(), 12, Font::plain));
    fsLabel->setBounds(185, 28, 65, 12);
    fsLabel->setColour(Label::textColourId, Colours::black);
    addAndMakeVisible(fsLabel);

   // fsText = new TextEditor("Fs (Hz)");
  //  fsText->setFont(Font(Font::getDefaultSerifFontName(), 12, Font::plain));
  //  fsText->setText(std::to_string(node->sample_rate));
   // fsText->setBounds(235, 71, 70, 15);
  //  fsText->setBounds(119, 42, 70, 15);
   // addAndMakeVisible(fsText);

    fsCbox = new ComboBox();
    fsCbox->setBounds(189, 40, 53, 17);
    fsCbox->addListener(this);
    for (int i = 0; i < 16; i++)
        fsCbox->addItem(String(i * 1000), i + 1); // start numbering at one for
    fsCbox->setSelectedId(1, dontSendNotification);
    addAndMakeVisible(fsCbox);

    // Actual Sample Rate Fs
    actFsLabel = new Label("Fs (Hz)", "Actual Fs (Hz)\n20345.678");
    actFsLabel->setFont(Font(Font::getDefaultSerifFontName(), 12, Font::plain));
    actFsLabel->setBounds(250, 29, 120, 25);
    actFsLabel->setColour(Label::textColourId, Colours::black);
    addAndMakeVisible(actFsLabel);

  //  actFsText = new TextEditor("Fs (Hz)");
  //  actFsText->setFont(Font(Font::getDefaultSerifFontName(), 12, Font::plain));
  //  actFsText->setText(std::to_string(node->sample_rate));
  //  actFsText->setBounds(235, 71, 70, 15);
  //  addAndMakeVisible(actFsText);

    //---

    // RHD Upper BW
    upBwLabel = new Label("upBW", "Up BW(Hz)");
    upBwLabel->setFont(Font(Font::getDefaultSerifFontName(), 12, Font::plain));
    upBwLabel->setBounds(115, 62, 85, 10);
    upBwLabel->setColour(Label::textColourId, Colours::black);
    addAndMakeVisible(upBwLabel);

    upBwText = new TextEditor("upBW");
    upBwText->setFont(Font(Font::getDefaultSerifFontName(), 12, Font::plain));
    upBwText->setText(std::to_string(node->upperBw));
    upBwText->setBounds(119, 74, 55, 15);
    addAndMakeVisible(upBwText); 

    // RHD Lower BW
    lowBwLabel = new Label("lowBW", "Low BW (Hz)");
    lowBwLabel->setFont(Font(Font::getDefaultSerifFontName(), 12, Font::plain));
    lowBwLabel->setBounds(115, 97, 85, 10);
    lowBwLabel->setColour(Label::textColourId, Colours::black);
    addAndMakeVisible(lowBwLabel);

    lowBwText = new TextEditor("lowBW");
    lowBwText->setFont(Font(Font::getDefaultSerifFontName(), 12, Font::plain));
    lowBwText->setText(std::to_string(node->lowerBw));
    lowBwText->setBounds(119, 109, 55, 15);
    addAndMakeVisible(lowBwText);

    // RHD DSP Cutoff
    dspCutLabel = new Label("dspCutLabel", "DSP Offset");
    dspCutLabel->setFont(Font(Font::getDefaultSerifFontName(), 12, Font::plain));
    dspCutLabel->setBounds(115, 29, 85, 10);
    dspCutLabel->setColour(Label::textColourId, Colours::black);
    addAndMakeVisible(dspCutLabel);

    dspCutNumLabel = new Label("dspCutNumLabel", "10"); //default RCB-LVDS IP number is 192.168.0.93
    dspCutNumLabel->setBounds(119, 42, 55, 15);
    dspCutNumLabel->setFont(Font(Font::getDefaultSerifFontName(), 12, Font::plain));
    dspCutNumLabel->setColour(Label::textColourId, Colours::black);
    dspCutNumLabel->setColour(Label::backgroundColourId, Colours::white);
    dspCutNumLabel->setEditable(true);
    dspCutNumLabel->addListener(this);
    addAndMakeVisible(dspCutNumLabel);


    // Hit Miss
    seqNumLabel = new Label("seqNum", "Packet Info:\nSQ N: 0\nGood: 0\nMiss: 0");
    seqNumLabel->setFont(Font(Font::getDefaultSerifFontName(), 12, Font::plain));
    seqNumLabel->setBounds(330, 29, 115, 45);
    seqNumLabel->setColour(Label::textColourId, Colours::black);
    addAndMakeVisible(seqNumLabel);

    batteryLabel = new Label("batteryVolts", "RCB Battery\n0.0 volts");
    batteryLabel->setFont(Font(Font::getDefaultSerifFontName(), 12, Font::plain));
    batteryLabel->setBounds(250, 57, 145, 25);
    batteryLabel->setColour(Label::textColourId, Colours::black);
    addAndMakeVisible(batteryLabel);

    rhdRegsLabel = new Label("rhdRegs", "Headstage Info:\nChannels: xx\nType: Uni/Bi\nChip ID: RHD2xxx ");
    rhdRegsLabel->setFont(Font(Font::getDefaultSerifFontName(), 12, Font::plain));
    rhdRegsLabel->setBounds(330, 79, 115, 45);
    rhdRegsLabel->setColour(Label::textColourId, Colours::black);
    addAndMakeVisible(rhdRegsLabel);

   // missLabel = new Label("missCnt", "Miss: 0");
   // missLabel->setFont(Font("Small Text", 10, Font::plain));
  //  missLabel->setBounds(325, 100, 85, 8);
  //  missLabel->setColour(Label::textColourId, Colours::darkgrey);
  //  addAndMakeVisible(missLabel);

   // hitCntText = new TextEditor("hitCnt");
  //  hitCntText->setFont(Font("Small Text", 10, Font::plain));
   // hitCntText->setText(std::to_string(node->data_offset));
  //  hitCntText->setBounds(295, 70, 65, 15);
  //  addAndMakeVisible(hitCntText);


    destIpLabel = new Label("DestIP", "RCB IP Addr:");
    destIpLabel->setBounds(15, 29, 130, 10);
    destIpLabel->setFont(Font(Font::getDefaultSerifFontName(), 12, Font::plain));
    addAndMakeVisible(destIpLabel);

    ipNumLabel = new Label("ipNumLabel", "192.168.0.93"); //default RCB-LVDS IP number is 192.168.0.93
    ipNumLabel->setBounds(19, 42, 85, 15);
    ipNumLabel->setFont(Font(Font::getDefaultSerifFontName(), 12, Font::plain));
    ipNumLabel->setColour(Label::textColourId, Colours::black);
    ipNumLabel->setColour(Label::backgroundColourId, Colours::white);
    ipNumLabel->setEditable(true);
    ipNumLabel->addListener(this);
    addAndMakeVisible(ipNumLabel);
    ipIsValid = true;  // since we have set a valid default ipAddr = 192.168.0.93

    hostIpLabel = new Label("DestIP", "Host IP Addr:");
    hostIpLabel->setBounds(15, 62, 130, 12);
    hostIpLabel->setFont(Font(Font::getDefaultSerifFontName(), 12, Font::plain));
    addAndMakeVisible(hostIpLabel);

   // hostIpNumLabel = new Label("HostIP", "192.168.0.203"); 
    hostIpNumLabel = new Label("HostIP", "Press Init!");
    hostIpNumLabel->setBounds(19, 77, 85, 15);
    hostIpNumLabel->setFont(Font(Font::getDefaultSerifFontName(), 12, Font::plain));
    hostIpNumLabel->setColour(Label::textColourId, Colours::black);
    hostIpNumLabel->setColour(Label::backgroundColourId, Colours::white);
    hostIpNumLabel->setEditable(true);
    hostIpNumLabel->addListener(this);
    addAndMakeVisible(hostIpNumLabel);
    hostIpIsValid = false;  // since we have set a valid default ipAddr = 192.168.0.93

     // Port
    portLabel = new Label("Port", "Host Port");
    portLabel->setFont(Font(Font::getDefaultSerifFontName(), 12, Font::plain));
    portLabel->setBounds(15, 97, 65, 12);
    portLabel->setColour(Label::textColourId, Colours::black);
    addAndMakeVisible(portLabel);

   // portNumLabel = new Label("Port Num", "Host Port");
    portNumLabel = new Label("Port Num", String(node->port));
    portNumLabel->setFont(Font(Font::getDefaultSerifFontName(), 12, Font::plain));
 //   portNumLabel->setText(std::to_string(node->port), dontSendNotification);
    portNumLabel->setBounds(19, 109, 42, 15);
    portNumLabel->setColour(Label::textColourId, Colours::black);
    portNumLabel->setColour(Label::backgroundColourId, Colours::white);
    portNumLabel->setEditable(true);
    portNumLabel->addListener(this);
    addAndMakeVisible(portNumLabel);
 
    // RCB PA Power
    paPwrLabel = new Label("PA PWR", "PA Attn");
    paPwrLabel->setFont(Font(Font::getDefaultSerifFontName(), 12, Font::plain));
    paPwrLabel->setBounds(185, 96, 65, 12);
    paPwrLabel->setColour(Label::textColourId, Colours::black);
    addAndMakeVisible(paPwrLabel);

    paPwrCbox = new ComboBox();
    paPwrCbox->setBounds(189, 108, 50, 17);
    paPwrCbox->addListener(this);
    for (int i = 0; i < 16; i++)
        paPwrCbox->addItem(String(i ), i + 1); // start numbering at one for
    paPwrCbox->setSelectedId(5, dontSendNotification);
    paPwrCbox->setTooltip("RCB WiFi PA Attenuation. Best when set at 4.");
    addAndMakeVisible(paPwrCbox);
    
}


void RcbWifiEditor::startAcquisition()
{
    // Check if initPassed is true.  If not the must press Init Button.
   // if (initPassed == true)

    if( node->initPassed == true)
    {
     std::cout << "editor startAcq Init Passed = " + String(node->initPassed) << std::endl;

    // Disable the whole gui
    portNumLabel->setEnabled(false);
    chanCbox->setEnabled(false);
    //sampText->setEnabled(false);
    fsCbox->setEnabled(false);
    upBwText->setEnabled(false);
    lowBwText->setEnabled(false);
    //connectButton->setEnabled(false);

    // Set the channels etc
 //   node->num_channels = chanText->getText().getIntValue();
    ////node->num_samp = sampText->getText().getIntValue();
 //   node->sample_rate = fsText->getText().getFloatValue();
  //  node->data_scale = scaleText->getText().getFloatValue();
  //  node->data_offset = offsetText->getText().getIntValue();
    //node->transpose = transposeButton.getToggleState();

    node->resizeChanSamp();

    timeInt = 0;
    startTimer(2000);
    }
    else {
        std::cout << " editor startAcq Init Passed = " + String(node->initPassed) << std::endl;  
    }


    
}

void RcbWifiEditor::timerCallback()
{
    //timeInt++;
    //stopTimer();
    //node->getPacketInfo();
    seqNumLabel->setText(node->getPacketInfo(), dontSendNotification);

    if (timeInt == 0)
    {
        // either of these approaches witll work, not sure if addvantage to either
        //this
        node->getBatteryInfo();
        batteryLabel->setText(node->batteryInfo, dontSendNotification);

        // or this
        //batteryLabel->setText(node->getBatteryInfo(), dontSendNotification);
       
        timeInt = 5;
    }
    timeInt--;
}

void RcbWifiEditor::stopAcquisition()
{
    stopTimer();
   
    // Reenable the whole gui
    portNumLabel->setEnabled(true);
    chanCbox->setEnabled(true);
    //sampText->setEnabled(true);
    fsCbox->setEnabled(true);
    upBwText->setEnabled(true);
    lowBwText->setEnabled(true);
    //connectButton->setEnabled(true);
    //transposeButton.setEnabled(true);
}

IPAddress RcbWifiEditor::getCurrentIpAddress()
{
    Array<IPAddress> ipAddresses;
    IPAddress::findAllAddresses(ipAddresses);
    // return fisrts non local addr
    for (int i = 0; i < ipAddresses.size(); ++i)
    {
        if (ipAddresses[i] != IPAddress::local())
            return ipAddresses[i];
    }

    return IPAddress();
}

void RcbWifiEditor::buttonEvent(Button* button)
{
    // Only one button
    if (button == connectButton)
    {
        node->port = portNumLabel->getText().getIntValue();
        node->tryToConnect();
    }
    else if (button == testButton) //now called Init button
    {
        std::cout << "Test Button Pressed" << std::endl;  // hey, this worked

        node->port = portNumLabel->getText().getIntValue();
        node->ipNumStr = ipNumLabel->getText();
        
        IPAddress myHost = getCurrentIpAddress();
        //CoreServices::sendStatusMessage(" host ip = " + myHost.toString());
        std::cout << "Host IP = " + myHost.toString() << std::endl;  // hey, this worked
        hostIpNumLabel->setText(myHost.toString(), dontSendNotification);
        node->myHostStr = myHost.toString() + ":" + portNumLabel->getText();

        // would be better to check if above values were valid
        //initPassed = true;
        node->initPassed = true;

        //node->getIntanStatusInfo();
        batteryLabel->setText(node->getIntanStatusInfo(), dontSendNotification);

        //hitLabel->setText(node->getPacketInfo(),sendNotification
        //startTimer(1000);

    }
    else if (button == runRCB)
    {
        node->sendRCBTriggerPost(ipNumStr, "__SL_P_ULD=ON");
    }
    else if (button == stopRCB)
    {
        node->sendRCBTriggerPost(ipNumStr,  "__SL_P_ULD=OFF");
    }
  //  else if (button == rcbDrawerButton)
  //  {
        //updateSubprocessorFifos();
   //     if (button->getToggleState())
   //         showControlButtons(true);
   //     else
   //         showControlButtons(false);
   // }
}


void RcbWifiEditor::labelTextChanged(juce::Label* label)
{
    if (label == ipNumLabel)
    {
        //set enable false
        enableState = false;

        String ipStr = "192.168.0.93";
        IPAddress ip = IPAddress(ipNumLabel->getText());
        if (ip.toString() == ipNumLabel->getText())
        {
            ipIsValid = true;
            CoreServices::sendStatusMessage("ip valid = " + String(ipIsValid));
        }
        else
        {
            ipIsValid = false;
            CoreServices::sendStatusMessage("ip valid = " + String(ipIsValid));
            //setEditEnableState(false);
            AlertWindow::showMessageBox(AlertWindow::NoIcon,
                "RCB-LVDS Module IP address " + ipNumLabel->getText() + " not valid.",
                "Please check your IP address setting. \r\n"
                "",
                "OK", 0);
            ipNumLabel->setText(ipStr, sendNotification);
        }
    }
    else if (label == hostIpNumLabel)
    {
        String hostStr = "Press Init Button!";
        IPAddress host = IPAddress(hostIpNumLabel->getText());
        if (host.toString() == hostIpNumLabel->getText())
        {
            hostIpIsValid = true;
            CoreServices::sendStatusMessage("host ip valid = " + String(ipIsValid));
        }
        else
        {
            hostIpIsValid = false;
            CoreServices::sendStatusMessage("host ip valid = " + String(ipIsValid));
            //setEditEnableState(false);
            AlertWindow::showMessageBox(AlertWindow::NoIcon,
                "OE GUI Host IP address " + hostIpNumLabel->getText() + " not valid.",
                "Please check your Host IP address setting. \r\n"
                "",
                "OK", 0);
            hostIpNumLabel->setText(hostStr, sendNotification);
        }
    }
    else if (label == portNumLabel)
    {
        Value val = label->getTextValue();
        int requestedValue = int(val.getValue());

        if (requestedValue < 1024 || requestedValue > 49151)
        {
            portIsValid = false;
            CoreServices::sendStatusMessage("Port value out of range.");
            AlertWindow::showMessageBox(AlertWindow::NoIcon,
                "OE GUI Host Port " + portNumLabel->getText() + " not valid.",
                "Please check your Host Port setting. \r\n"
                "",
                "OK", 0);

        }           
        else
        {
            portIsValid = true;
            CoreServices::sendStatusMessage("Port value ok.");
        }
    }

}



void RcbWifiEditor::setEditEnableState(bool enableState)
{
    this->enableState = enableState;
    if (enableState == true)
    {
        //CoreServices::sendStatusMessage("enable= true");
       // enableRcbTrigger->setToggleState(true, dontSendNotification);
    }
    else
    {
        //CoreServices::sendStatusMessage("enable= false");
       // enableRcbTrigger->setToggleState(false, dontSendNotification);
    }

}

void RcbWifiEditor::comboBoxChanged(ComboBox* comboBoxThatHasChanged)
{
   /* RCBTrigger* p = (RCBTrigger*)getProcessor();
    if (comboBoxThatHasChanged == triggerChannelSelector)
    {
        String rcbTrigCh = triggerChannelSelector->getText();
        testLabel->setText(triggerChannelSelector->getText(), dontSendNotification);
        p->setTriggerCh(rcbTrigCh);
    }
    else if (comboBoxThatHasChanged == gateChannelSelector)
    {
        gateChToRCB = gateChannelSelector->getSelectedId();
        testLabel->setText(eventChannelSelector->getText(), dontSendNotification);
        p->setGateChRCB(gateChToRCB);
    }
    else if (comboBoxThatHasChanged == eventChannelSelector)
    {
        eventChToRCB = eventChannelSelector->getSelectedId();
        testLabel->setText(eventChannelSelector->getText(), dontSendNotification);
        p->setEventChRCB(eventChToRCB);
    } */
}

/*void RcbWifiEditor::showControlButtons(bool show)
{

    //subprocessorsVisible = show;

  //  if (show)
        // numSubprocessors = recordNode->getNumSubProcessors();

    int dX = 60; //* (numSubprocessors + 1);
    dX = show ? dX : -dX;

    rcbDrawerButton->setBounds(
        rcbDrawerButton->getX() + dX, rcbDrawerButton->getY(),
        rcbDrawerButton->getWidth(), rcbDrawerButton->getHeight());

    
    // Add connect button
    connectButton = new UtilityButton("Init", Font("default", 13, Font::bold));
    connectButton->setBounds(5, 30, 50, 18);
    //   connectButton->setColour(UtilityButton::Button::, Colours::black)
    //connectButton->addListener(this);
    addAndMakeVisible(connectButton);
    connectButton->setVisible(show);
   

  //  dataPathLabel->setBounds(
  //      dataPathLabel->getX() + dX, dataPathLabel->getY(),
  //      dataPathLabel->getWidth(), dataPathLabel->getHeight());

   // connectButton->setBounds(
   //     connectButton->getX() + dX, connectButton->getY(),
   //     connectButton->getWidth(), connectButton->getHeight());

   
  
    desiredWidth +=  dX;

    CoreServices::highlightEditor(this);
    deselect();

}*/

//RcbDrawerButton::RcbDrawerButton(const String& name) : DrawerButton(name)
//{
//}

//RcbDrawerButton::~RcbDrawerButton()
//{
//}

/*void RcbDrawerButton::paintButton(Graphics& g, bool isMouseOver, bool isButtonDown)
{
    g.setColour(Colour(110, 110, 110));
    if (isMouseOver)
        g.setColour(Colour(210, 210, 210));

    g.drawVerticalLine(3, 0.0f, getHeight());
    g.drawVerticalLine(5, 0.0f, getHeight());
    g.drawVerticalLine(7, 0.0f, getHeight());
}*/


void RcbWifiEditor::saveEditorParameters(XmlElement* xmlNode)
{
    XmlElement* parameters = xmlNode->createNewChildElement("PARAMETERS");

    parameters->setAttribute("port", portNumLabel->getText());
    parameters->setAttribute("numchan", chanCbox->getText());
    parameters->setAttribute("numsamp", sampText->getText());
    parameters->setAttribute("fs", fsCbox->getText());
    parameters->setAttribute("upbw", upBwText->getText());
    parameters->setAttribute("lowbw", lowBwText->getText());
}

void RcbWifiEditor::loadEditorParameters(XmlElement* xmlNode)
{
    forEachXmlChildElement(*xmlNode, subNode)
    {
        if (subNode->hasTagName("PARAMETERS"))
        {
            portNumLabel->setText(subNode->getStringAttribute("port", ""), dontSendNotification);
            chanCbox->setText(subNode->getStringAttribute("numchan", ""));
            sampText->setText(subNode->getStringAttribute("numsamp", ""));
            fsCbox->setText(subNode->getStringAttribute("fs", ""));
            upBwText->setText(subNode->getStringAttribute("upbw", ""));
            lowBwText->setText(subNode->getStringAttribute("lowbw", ""));
        }
    }
}

