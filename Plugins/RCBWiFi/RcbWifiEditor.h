#ifndef __RCBWIFIEDITORH__
#define __RCBWIFIEDITORH__

#ifdef _WIN32
#include <Windows.h>
#endif

#include <VisualizerEditorHeaders.h>
//#include <EditorHeaders.h>
#include "../Source/Processors/Editors/GenericEditor.h"

namespace RcbWifiNode
{
    class RcbWifi;
  //  class RcbDrawerButton : public DrawerButton
  //  {
  //  public:
  //      RcbDrawerButton(const String& name);
 //       ~RcbDrawerButton();
  //  private:
 //       void paintButton(Graphics& g, bool isMouseOver, bool isButtonDown) override;
  //  };

    class RcbWifiEditor : public GenericEditor, public Label::Listener, public ComboBox::Listener
    {

    public:

        RcbWifiEditor(GenericProcessor* parentNode, RcbWifi *node);
        //~RcbWifiEditor();

        /** Button listener callback, called by button when pressed. */
        void buttonEvent(Button* button);

        /** Called by processor graph in beginning of the acqusition, disables editor completly. */
        void startAcquisition();

        /** Called by processor graph at the end of the acqusition, reenables editor completly. */
        void stopAcquisition();

        /** Called when configuration is saved. Adds editors config to xml. */
        void saveEditorParameters(XmlElement* xml);

        /** Called when configuration is loaded. Reads editors config from xml. */
        void loadEditorParameters(XmlElement* xml);

        void showControlButtons(bool);

        //ScopedPointer<RcbDrawerButton> rcbDrawerButton;

        void labelTextChanged(juce::Label* label) override;
        void comboBoxChanged(ComboBox* comboBoxThatHasChanged);
        void setEditEnableState(bool enableState);
        IPAddress getCurrentIpAddress();
       
       

    private:

   //     ScopedPointer<RcbDrawerButton> rcbDrawerButton;

        // Test
       // TextButton testButton{ "Test" };
        ScopedPointer<UtilityButton> testButton;
        ScopedPointer<UtilityButton> runRCB;
        ScopedPointer<UtilityButton> stopRCB;
        String ipNumStr = "192.168.0.93";
        

        // Button that tried to connect to client
        ScopedPointer<UtilityButton> connectButton;

        

        // Chans
        ScopedPointer<Label> chanLabel;
      //  ScopedPointer<TextEditor> chanText;
        ScopedPointer<ComboBox> chanCbox;

        // Samples
        ScopedPointer<Label> sampLabel;
        ScopedPointer<TextEditor> sampText;

        // Desired Sample Rate
        ScopedPointer<Label> fsLabel;
      //  ScopedPointer<TextEditor> fsText;
        ScopedPointer<ComboBox> fsCbox;  // Desired RCB Sample Rate Selection

        // Actual Sample Rate
        ScopedPointer<Label> actFsLabel;
        ScopedPointer<TextEditor> actFsText;

        // RHD Up Bw
        ScopedPointer<Label> upBwLabel;
        ScopedPointer<TextEditor> upBwText;

        // RHD Low Bw
        ScopedPointer<Label> lowBwLabel;
        ScopedPointer<TextEditor> lowBwText;

        // RHD DSP Cutoff
        ScopedPointer<Label> dspCutLabel;
        ScopedPointer<Label> dspCutNumLabel;

        // Packet Info - Hit Miss
        ScopedPointer<Label> seqNumLabel;
        
        //Battery
        ScopedPointer<Label> batteryLabel;
  
        // Intan RDH read only regs
        ScopedPointer<Label> rhdRegsLabel;

        // RCB PA Power
        ScopedPointer<Label> paPwrLabel;
        //ScopedPointer<Label> paPwrNumLabel;
        ScopedPointer<ComboBox> paPwrCbox;

        // RCB IP Addr
        ScopedPointer<Label> destIpLabel;
        ScopedPointer<Label> ipNumLabel;

        // Host IP Addr
        ScopedPointer<Label> hostIpLabel;
        ScopedPointer<Label> hostIpNumLabel;

        // Port
        ScopedPointer<Label> portLabel;
        ScopedPointer<Label> portNumLabel;

       
  

        // Parent node
        RcbWifi* node;
        void timerCallback();
        int timeInt = 0;

        // IP stuff
        //String ipNumStr;
        String triggerChStr;
        String myHostStr;

        int eventChToRCB;
        int gateChToRCB;
        bool enableState;
        bool ipIsValid;
        bool hostIpIsValid;
        bool portIsValid;

        //bool initPassed;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RcbWifiEditor);
    };
}

#endif