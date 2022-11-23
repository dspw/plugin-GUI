// DSPW

#include "RcbWifiEditor.h"
#include "RcbWifi.h"

#include <string>
#include <iostream>

using namespace RcbWifiNode;

RcbWifiEditor::RcbWifiEditor(GenericProcessor* parentNode, RcbWifi *socket) : GenericEditor(parentNode)  //false)
{
    node = socket;
    desiredWidth = 400;

    //initPassed = false;

    String desiredFs[15] = { "20000", "15000", "12500", "10000", "8000", "6250", "5000", "4000", "3330", "3000", "2500", "2000", "1500", "1250", "1000" };
    
    String lowerBw[25] = { "500 Hz", "300 Hz", "250 Hz", "200 Hz", "150 Hz",
        "100 Hz", "75 Hz", "50 Hz", "30 Hz", "25 Hz", "20 Hz", "15 Hz", "10 Hz",
        "7.5 Hz", "5.0 Hz", "3.0 Hz", "2.5 Hz", "2.0 Hz", "1.5 Hz", "1.0 Hz",
        "0.75 Hz", "0.50 Hz", "0.30 Hz", "0.25 Hz", "0.10 Hz" };
    
    String upperBw[17] = { "20 kHz", "15 kHz", "10 kHz", "7.5 kHz", "5.0 kHz", 
        "3.0 kHz", "2.5 kHz", "2.0 kHz", "1.5 kHz", "1.0 kHz", "750 Hz", "500 Hz", 
        "300 Hz", "250 Hz", "200 Hz", "150 Hz", "100 Hz" };

    //int numTsItems[8] = { 21, 23, 27, 32, 39, 51, 71, 119 }; // numCh[32,28, 24,20, 16, 12, 8, 4];

    

    configButton = std::make_unique<ColorButton>("Config", Font("default", 13, Font::bold));
    configButton->setBounds(250, 50, 50, 18);
    
    configButton->setColors(Colours::red, Colours::white);
    configButton->addListener(this);
    //addAndMakeVisible(configButton.get());
 

    // Add connect button
  //  connectButton = new UtilityButton("Init", Font("default", 13, Font::bold));
 //   connectButton->setBounds(5, 30, 50, 18);
 //   connectButton->setColour(UtilityButton::Button::, Colours::black)
 //   connectButton->addListener(this);
 //   addAndMakeVisible(connectButton);

  
    // Num chans
    chanLabel = new Label("NumCh", "Channels");
    chanLabel->setFont(Font(Font::getDefaultSerifFontName(), 13, Font::plain));
    chanLabel->setBounds(186, 62, 75, 12);
    chanLabel->setColour(Label::textColourId, Colours::black);
    addAndMakeVisible(chanLabel);

   // chanText = new TextEditor("Num. Ch.");
   // chanText->setFont(Font(Font::getDefaultSerifFontName(), 12, Font::plain));
   // chanText->setText(std::to_string(node->num_channels));
   // chanText->setBounds(219, 74, 45, 15);
   // addAndMakeVisible(chanText);

    chanCbox = new ComboBox();
   /// chanCbox->setBounds(174, 74, 53, 17);
    chanCbox->setBounds(188, 74, 53, 17);
    chanCbox->addListener(this);
    for (int i = 8; i > 0; i--)
        chanCbox->addItem(String(i * 4), i ); // start numbering at one for

    //for (int i = 1; i < 9; i++)
    //    chanCbox->addItem(String(i * 4), i); // start numbering at one for
    chanCbox->setSelectedId(8, dontSendNotification);
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
    fsLabel = new Label("Fs(Hz)", "Fs (Hz)");
    fsLabel->setFont(Font(Font::getDefaultSerifFontName(), 13, Font::plain));
    fsLabel->setBounds(186, 28, 65, 12);
    fsLabel->setColour(Label::textColourId, Colours::black);
    addAndMakeVisible(fsLabel);

    fsCbox = new ComboBox();
  //  fsCbox->setBounds(174, 40, 53, 17);
    fsCbox->setBounds(188, 40, 53, 17);

    fsCbox->addListener(this);
    for (int i = 0; i < 15; i++)
        fsCbox->addItem(desiredFs[i], i + 1); // start numbering at one for
    //fsCbox->addItem(String(i * 1000), i + 1); // start numbering at one for
    fsCbox->setSelectedId(1, dontSendNotification);
    addAndMakeVisible(fsCbox);

    // Actual Sample Rate Fs
   // actFsLabel = new Label("Fs (Hz)", "Actual Fs (Hz)\n20345.678");
    actFsLabel = new Label("ActFs(Hz)", "Actual Fs\n" + String(node->sample_rate) + " Hz");
    actFsLabel->setFont(Font(Font::getDefaultSerifFontName(), 13, Font::plain));
    actFsLabel->setBounds(250, 28, 120, 25);
    actFsLabel->setColour(Label::textColourId, Colours::black);
   // addAndMakeVisible(actFsLabel);

    // RHD Upper BW
    upBwLabel = new Label("upBW", "Upper");
    upBwLabel->setFont(Font(Font::getDefaultSerifFontName(), 13, Font::plain));
    upBwLabel->setBounds(120, 62, 85, 10);
    upBwLabel->setColour(Label::textColourId, Colours::black);
    addAndMakeVisible(upBwLabel);

    upBwText = new TextEditor("upBW");
    upBwText->setFont(Font(Font::getDefaultSerifFontName(), 13, Font::plain));
    upBwText->setText(std::to_string(node->upperBw));
    upBwText->setBounds(122, 74, 40, 15);
   // addAndMakeVisible(upBwText); 

    upBwCbox = new ComboBox();
    upBwCbox->setBounds(122, 74, 55, 17);
    upBwCbox->addListener(this);
    for (int i = 0; i < 17; i++)
        upBwCbox->addItem(upperBw[i], i + 1); // start numbering at one for
    upBwCbox->setSelectedId(4, dontSendNotification);
    addAndMakeVisible(upBwCbox);

    // RHD Lower BW
    lowBwLabel = new Label("lowBW", "Lower");
    lowBwLabel->setFont(Font(Font::getDefaultSerifFontName(), 13, Font::plain));
    lowBwLabel->setBounds(120, 97, 85, 12);
    lowBwLabel->setColour(Label::textColourId, Colours::black);
    addAndMakeVisible(lowBwLabel);

    lowBwText = new TextEditor("lowBW");
    lowBwText->setFont(Font(Font::getDefaultSerifFontName(), 13, Font::plain));
    lowBwText->setText(std::to_string(node->lowerBw));
    lowBwText->setBounds(122, 109, 40, 15);
    //addAndMakeVisible(lowBwText);

    lowBwCbox = new ComboBox();
    lowBwCbox->setBounds(122, 109, 55, 17);
    lowBwCbox->addListener(this);
    for (int i = 0; i < 25; i++)
        lowBwCbox->addItem(lowerBw[i], i + 1); // start numbering at one for
    lowBwCbox->setSelectedId(20, dontSendNotification);
    addAndMakeVisible(lowBwCbox);


    // RHD DSP Cutoff
    dspCutLabel = new Label("dspCutLabel", "DSP HPF");
    dspCutLabel->setFont(Font(Font::getDefaultSerifFontName(), 13, Font::plain));
    dspCutLabel->setBounds(120, 28, 85, 12);
    dspCutLabel->setColour(Label::textColourId, Colours::black);
    addAndMakeVisible(dspCutLabel);

    dspCutNumLabel = new Label("dspCutNumLabel", "10"); 
    dspCutNumLabel->setBounds(150, 41, 30, 15);
    //dspCutNumLabel->setBounds(122, 41, 55, 15);
    dspCutNumLabel->setFont(Font(Font::getDefaultSerifFontName(), 13, Font::plain));
    dspCutNumLabel->setColour(Label::textColourId, Colours::black);
    //dspCutNumLabel->setColour(Label::backgroundColourId, Colours::white);
    dspCutNumLabel->setEditable(true);
    dspCutNumLabel->addListener(this);
    addAndMakeVisible(dspCutNumLabel);

    // add DSP Offset Button
    dspoffsetButton = new UtilityButton("DSP:", Font("Small Text", 13, Font::plain));
    dspoffsetButton->setRadius(3.0f); // sets the radius of the button's corners
    dspoffsetButton->setBounds(122, 39, 29, 18); // sets the x position, y position, width, and height of the button
    dspoffsetButton->addListener(this);
    dspoffsetButton->setClickingTogglesState(true); // makes the button toggle its state when clicked
    dspoffsetButton->setTooltip("Toggle DSP offset removal");
    addAndMakeVisible(dspoffsetButton); // makes the button a child component of the editor and makes it visible
    dspoffsetButton->setToggleState(true, dontSendNotification);


    // Hit Miss
    seqNumLabel = new Label("seqNum", "Packet Info:\nSQ N: 0\nGood: 0\nMiss: 0");
    seqNumLabel->setFont(Font(Font::getDefaultSerifFontName(), 13, Font::plain));
    seqNumLabel->setBounds(254, 29, 115, 50);
  //  fsLabel->setBounds(185, 28, 65, 12);
    seqNumLabel->setColour(Label::textColourId, Colours::black);
    addAndMakeVisible(seqNumLabel);

    //batteryLabel = new Label("batteryVolts", "RCB Battery\n0.0 Volts");
    batteryLabel = new Label("batteryVolts", "Battery\n0.00V\n--");
    batteryLabel->setFont(Font(Font::getDefaultSerifFontName(), 13, Font::plain));
   // batteryLabel->setBounds(250, 57, 145, 25);
  //  batteryLabel->setBounds(318, 110, 215, 15);
    batteryLabel->setBounds(318, 80, 215, 50);
   // batteryLabel->setBounds(250, 5, 215, 15);
    batteryLabel->setColour(Label::textColourId, Colours::black);
    addAndMakeVisible(batteryLabel);

   // rhdRegsLabel = new Label("rhdRegs", "Headstage Info:\nChannels: xx\nType: Uni/Bi\nChip ID: RHD2xxx ");
    rhdRegsLabel = new Label("rhdRegs", "RHD2xxx\nxx Ch\nUni/Bi ");
    rhdRegsLabel->setFont(Font(Font::getDefaultSerifFontName(), 13, Font::plain));
    rhdRegsLabel->setBounds(254, 80, 215, 50);
 //   rhdRegsLabel->setBounds(244, 110, 215, 15);
  //  rhdRegsLabel->setBounds(130, 5, 215, 15);
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


    destIpLabel = new Label("RCBIP", "RCB IP Addr:");
    destIpLabel->setBounds(8, 28, 130, 12);
    destIpLabel->setFont(Font(Font::getDefaultSerifFontName(), 13, Font::plain));
    addAndMakeVisible(destIpLabel);

    rcbIpNumLabel = new Label("ipNumLabel", "192.168.0.93"); //default RCB-LVDS IP number is 192.168.0.93
    rcbIpNumLabel->setTooltip("Default RCB-LVDS IP number is 192.168.0.93");
    rcbIpNumLabel->setBounds(10, 41, 100, 15);
    rcbIpNumLabel->setFont(Font(Font::getDefaultSerifFontName(), 13, Font::plain));
    rcbIpNumLabel->setColour(Label::textColourId, Colours::black);
    rcbIpNumLabel->setColour(Label::backgroundColourId, Colours::white);
    rcbIpNumLabel->setEditable(true);
    rcbIpNumLabel->addListener(this);
    addAndMakeVisible(rcbIpNumLabel);
    ipIsValid = true;  // since we have set a valid default ipAddr = 192.168.0.93

    hostIpLabel = new Label("DestIP", "Host IP Addr:");
    hostIpLabel->setBounds(8, 61, 130, 12);
    hostIpLabel->setFont(Font(Font::getDefaultSerifFontName(), 13, Font::plain));
    addAndMakeVisible(hostIpLabel);

   // hostIpNumLabel = new Label("HostIP", "192.168.0.203"); 
    hostIpNumLabel = new Label("HostIP", "Press Init!");
    hostIpNumLabel->setBounds(10, 74, 100, 15);
    hostIpNumLabel->setFont(Font(Font::getDefaultSerifFontName(), 13, Font::plain));
    hostIpNumLabel->setColour(Label::textColourId, Colours::black);
    hostIpNumLabel->setColour(Label::backgroundColourId, Colours::white);
    hostIpNumLabel->setEditable(true);
    hostIpNumLabel->addListener(this);
    addAndMakeVisible(hostIpNumLabel);
    hostIpIsValid = false;  // since we have set a valid default ipAddr = 192.168.0.93

     // Port
    portLabel = new Label("Port", "Port"); //Host Port
    portLabel->setFont(Font(Font::getDefaultSerifFontName(), 13, Font::plain));
    portLabel->setBounds(8, 96, 65, 12);
    portLabel->setColour(Label::textColourId, Colours::black);
    addAndMakeVisible(portLabel);

   // portNumLabel = new Label("Port Num", "Host Port");
    portNumLabel = new Label("PortNum", String(node->port));
    portNumLabel->setFont(Font(Font::getDefaultSerifFontName(), 13, Font::plain));
 //   portNumLabel->setText(std::to_string(node->port), dontSendNotification);
    portNumLabel->setBounds(10, 109, 38, 15); //42
    portNumLabel->setColour(Label::textColourId, Colours::black);
    portNumLabel->setColour(Label::backgroundColourId, Colours::white);
    portNumLabel->setEditable(true);
    portNumLabel->addListener(this);
    addAndMakeVisible(portNumLabel);

    // Init
    initLabel = new Label("Init", "Connect"); 
    initLabel->setFont(Font(Font::getDefaultSerifFontName(), 13, Font::plain));
    initLabel->setBounds(52, 96, 60, 12);
    initLabel->setColour(Label::textColourId, Colours::black);
    addAndMakeVisible(initLabel);

    // Add Init button
 //  testButton = new UtilityButton("INIT", Font("Small Text", 13, Font::bold));
  // testButton = std::make_unique<UtilityButton>("INIT", Font("Small Text", 13, Font::bold));
    initButton = std::make_unique<UtilityButton>("Init", Font("Small Text", 13, Font::bold)); //13
  // testButton->setLabel("Pass");
   // testButton->setColour(Label::textColourId, Colours::red);
    initButton->setBounds(57, 108, 50, 16); //67 107 38 18 //(71, 99, 35, 26)
    initButton->addListener(this);
    addAndMakeVisible(initButton.get());

    testButton = new TextButton("PRESS ME");//, Font("default", 13, Font::bold));
    testButton->setBounds(58, 108, 55, 16); //250, 20, 70, 18
    testButton->setColour(TextButton::textColourOffId, Colours::black);
    testButton->setColour(TextButton::textColourOnId, Colours::blue);
    testButton->setColour(TextButton::buttonColourId, Colours::lightgreen);
    testButton->setColour(TextButton::buttonOnColourId, Colours::lightyellow);
    testButton->addListener(this);
  //  addAndMakeVisible(testButton);
 
    // RCB PA Power
    paPwrLabel = new Label("PAPWR", "PA Attn");
    paPwrLabel->setFont(Font(Font::getDefaultSerifFontName(), 13, Font::plain));
    paPwrLabel->setBounds(186, 96, 65, 12);
    paPwrLabel->setColour(Label::textColourId, Colours::black);
    addAndMakeVisible(paPwrLabel);

    paPwrCbox = new ComboBox();
    paPwrCbox->setBounds(188, 108, 53, 17);
    paPwrCbox->addListener(this);
    for (int i = 0; i < 16; i++)
        paPwrCbox->addItem(String(i ), i + 1); // start numbering at one for
    paPwrCbox->setSelectedId(5, dontSendNotification);
    paPwrCbox->setTooltip("RCB WiFi PA Attenuation. Best when set at 4.");
    addAndMakeVisible(paPwrCbox);
    
    myHost = getCurrentIpAddress();
    hostStr = myHost.toString();
    //CoreServices::sendStatusMessage(" host ip = " + myHost.toString());
    std::cout << "[dspw]" << "Host IP = " + myHost.toString() << std::endl;
    hostIpNumLabel->setText(myHost.toString(), dontSendNotification);
}


void RcbWifiEditor::startAcquisition()
{
    // Check if initPassed is true.  If not the must press Init Button.
   // if (initPassed == true)

    if( node->initPassed == true)  //init passed must expand
    {
     std::cout << "[dspw]" << "editor startAcq Init Passed = true" << std::endl;

    // Disable the whole gui
    portNumLabel->setEnabled(false);
    chanCbox->setEnabled(false);
    fsCbox->setEnabled(false);
    upBwCbox->setEnabled(false);
    lowBwCbox->setEnabled(false);
    paPwrCbox->setEnabled(false);
    dspCutNumLabel->setEnabled(false);
    //sampText->setEnabled(false);
    //upBwText->setEnabled(false);
    //lowBwText->setEnabled(false);
    //connectButton->setEnabled(false);
    

    // Set the channels etc
 //   node->num_channels = chanText->getText().getIntValue();
    ////node->num_samp = sampText->getText().getIntValue();
 //   node->sample_rate = fsText->getText().getFloatValue();
  //  node->data_scale = scaleText->getText().getFloatValue();
  //  node->data_offset = offsetText->getText().getIntValue();
    //node->transpose = transposeButton.getToggleState();


     // where shoud this be called?????
     // at init button?
    //node->resizeChanSamp();


     //rhdRegsLabel->setText(node->rhdStatusInfo, dontSendNotification);

    timeInt = 0;
    startTimer(2000);
    }
    else {
        std::cout << "[dspw]" << " editor startAcq Init Passed = false " << std::endl;
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
   // portNumLabel->setEnabled(true);
   // chanCbox->setEnabled(true);
    //sampText->setEnabled(true);
    //fsCbox->setEnabled(true);
    //upBwText->setEnabled(true);
    //lowBwText->setEnabled(true);
    //connectButton->setEnabled(true);
    

    portNumLabel->setEnabled(true);
    chanCbox->setEnabled(true);
    fsCbox->setEnabled(true);
    upBwCbox->setEnabled(true);
    lowBwCbox->setEnabled(true);
    paPwrCbox->setEnabled(true);
    dspCutNumLabel->setEnabled(true);
    
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

void RcbWifiEditor::buttonClicked(Button* button)
{
    // Only one button
   // if (button == connectButton.get())
  //  {
  //      node->port = portNumLabel->getText().getIntValue();
  //      node->tryToConnect();

  //  }
   // else 
    if (button == initButton.get()) //now called Init button
    {
        std::cout << "[dspw]" << "Init Button Pressed" << std::endl;  // hey, this worked

        node->port = portNumLabel->getText().getIntValue();
        node->ipNumStr = rcbIpNumLabel->getText();
        
       // IPAddress myHost = getCurrentIpAddress();
       // String hostStr = myHost.toString();
       // //CoreServices::sendStatusMessage(" host ip = " + myHost.toString());
       // std::cout << "[dspw]" << "Host IP = " + myHost.toString() << std::endl;
      //  hostIpNumLabel->setText(myHost.toString(), dontSendNotification);

        //check if host and RCB are on same network
        if (node->ipNumStr.substring(0, 8) == hostStr.substring(0, 8))
        {
            std::cout << "[dspw]" << "IP compare OK " << std::endl;
            // set up host string with ip and port
            node->myHostStr = myHost.toString() + ":" + portNumLabel->getText();


            // would be better to check if above values were valid
            //initPassed = true;
            //node->initPassed = true;
         
            //first check that RCB init happened ok
            String status = node->getIntanStatusInfo();
            if (node->isGoodRCB == true)
            {
                batteryLabel->setText(node->batteryStatusInfo, dontSendNotification);
                //node->setRCBTokens();

                //node->isGoodIntan = true;  // for test
                //then check that rhd is ok
                rhdRegsLabel->setText(node->rhdStatusInfo, dontSendNotification);
               // if (node->isGoodIntan == true)
                {
                    // set up pa attn
                    node->rcbPaStr = paPwrCbox->getText();

                    // get number of channels from dropdown box
                    node->rhdNumChStr = chanCbox->getText();
                    int num_channels = chanCbox->getText().getIntValue();
                    node->num_channels = num_channels;
                    //CoreServices::updateSignalChain(this);
                    // get number of samples in each packet
                   // int rhdNumTsItems = chanCbox->getSelectedId() ;
                    node->rhdNumTsItems = chanCbox->getSelectedItemIndex();
           
                    // get nominal sample rate from combo box
                    node->rhdFsStr = fsCbox->getText();
                    int myFs = fsCbox->getText().getIntValue();
                   // node->sample_rate = myFs;
                    myFs = node->updateSampleRate();
                    node->sample_rate = myFs;
                    CoreServices::updateSignalChain(this);


                    // get upper BW filter from combo box
                  //  node->rhdUpBwStr = upBwCbox->getText();

                    // get lower BW filter from combo box
                  //  node->rhdLowBwStr = lowBwCbox->getText();

                    node->setRCBTokens();
                    // if tokens are sent ok then RCB WiFi should be initialized!


                    node->initPassed = true;
                }
               // else {
               //     std::cout << "[dspw]" << "isGoodIntan = false" << std::endl;
               //     return;
               // }

            }
            else {
                std::cout << "[dspw]" << "isGoodRCB = false" << std::endl;
                return;
            }

            //see above for maybe better
            //then check that rhd is ok
            //rhdRegsLabel->setText(node->rhdStatusInfo, dontSendNotification);
/*
            // set up pa attn
            node->rcbPaStr = paPwrCbox->getText();

            // get number of channels from dropdown box
            node->rhdNumChStr = chanCbox->getText();
          
            // get nominal sample rate from combo box
            node->rhdFsStr = fsCbox->getText();

            node->setRCBTokens();
            */

            //hitLabel->setText(node->getPacketInfo(),sendNotification
            //startTimer(1000);
        }
        else
        {
            std::cout << "[dspw]" << "IP compare failed " << std::endl;

            AlertWindow::showMessageBox(AlertWindow::NoIcon,
                "RCB and Host IP network mismatch.",
                "Check your Wifi network connection. \r\n\r\n"
                "Press Init button to try again.",
                "OK", 0);
        }
        

    }
    else if (button == testButton)
    {
   //     node->sendRCBTriggerPost(ipNumStr, "__SL_P_ULD=ON");
       // testButton->setColour(TextButton::buttonColourId, Colours::lightsalmon);
    }
  //  else if (button == stopRCB)
    {
   //     node->sendRCBTriggerPost(ipNumStr,  "__SL_P_ULD=OFF");
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
    if (label == rcbIpNumLabel)
    {
        //set enable false
        enableState = false;

        String ipStr = "192.168.0.93";
        IPAddress ip = IPAddress(rcbIpNumLabel->getText());
        if (ip.toString() == rcbIpNumLabel->getText())
        {
            ipIsValid = true;
            //CoreServices::sendStatusMessage("ip valid = " + String(int(ipIsValid)));
        }
        else
        {
            ipIsValid = false;
            //CoreServices::sendStatusMessage("ip valid = " + String(int(ipIsValid)));
            //setEditEnableState(false);
            AlertWindow::showMessageBox(AlertWindow::NoIcon,
                "RCB-LVDS Module IP address " + rcbIpNumLabel->getText() + " not valid.",
                "Please check your IP address setting. \r\n"
                "",
                "OK", 0);
            rcbIpNumLabel->setText(ipStr, sendNotification);
        }
    }
    else if (label == hostIpNumLabel)
    {
        String hostStr = "Press Init Button!";
        IPAddress host = IPAddress(hostIpNumLabel->getText());
        if (host.toString() == hostIpNumLabel->getText())
        {
            hostIpIsValid = true;
            //CoreServices::sendStatusMessage("host ip valid = " + String(int(ipIsValid)));
        }
        else
        {
            hostIpIsValid = false;
            // CoreServices::sendStatusMessage("host ip valid = " + String(int(ipIsValid)));
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
    if (comboBoxThatHasChanged == chanCbox)
    {
        String rcbNumCh = chanCbox->getText();
    }
    else if (comboBoxThatHasChanged == fsCbox)
    {
        String rcbDesiredFs = fsCbox->getText();
    }
    else if (comboBoxThatHasChanged == upBwCbox)
    {

    }
    else if (comboBoxThatHasChanged == lowBwCbox)
    {

    }
    else if (comboBoxThatHasChanged == paPwrCbox)
    {

    }

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




/*
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
}*/

