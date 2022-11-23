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

    class RcbWifiEditor : public GenericEditor, public Label::Listener, public ComboBox::Listener, public Button::Listener, public Timer
    {

    public:

        RcbWifiEditor(GenericProcessor* parentNode, RcbWifi *node);
        //~RcbWifiEditor();

        /** Button listener callback, called by button when pressed. */
       // void buttonEvent(Button* button);
        void buttonClicked(Button* button);

        /** Called by processor graph in beginning of the acqusition, disables editor completly. */
        void startAcquisition();

        /** Called by processor graph at the end of the acqusition, reenables editor completly. */
        void stopAcquisition();

        /** Called when configuration is saved. Adds editors config to xml. */
        //void saveEditorParameters(XmlElement* xml);

        /** Called when configuration is loaded. Reads editors config from xml. */
        //void loadEditorParameters(XmlElement* xml);

        //?void showControlButtons(bool);

        //ScopedPointer<RcbDrawerButton> rcbDrawerButton;

        void labelTextChanged(juce::Label* label) override;
        void comboBoxChanged(ComboBox* comboBoxThatHasChanged);
        void setEditEnableState(bool enableState);
        IPAddress getCurrentIpAddress();
       
        String desiredFs;
        

    private:

   //     ScopedPointer<RcbDrawerButton> rcbDrawerButton;

        // Test
       // TextButton testButton{ "Test" };

        //ScopedPointer<UtilityButton> testButton;
        //ScopedPointer<UtilityButton> runRCB;
        //ScopedPointer<UtilityButton> stopRCB;

        //std::unique_ptr<UtilityButton> initButton;
       // std::unique_ptr<ToggleButton> testButton;
       // std::unique_ptr<UtilityButton> runRCB;
       // std::unique_ptr<UtilityButton> stopRCB;
        String ipNumStr = "192.168.0.93";
        

        // Button that tried to connect to client
        //ScopedPointer<UtilityButton> connectButton;
        std::unique_ptr<ColorButton> configButton;

      //  std::unique_ptr<TextButton> testButton;
        ScopedPointer<TextButton>testButton;

        // Chans
        ScopedPointer<Label> chanLabel;
      //  ScopedPointer<TextEditor> chanText;  // replaced with comboBox
        ScopedPointer<ComboBox> chanCbox;

        // Samples
        ScopedPointer<Label> sampLabel;
        ScopedPointer<TextEditor> sampText;

        // Desired Sample Rate
        ScopedPointer<Label> fsLabel;
      //  ScopedPointer<TextEditor> fsText;  // replaced with comboBox
        ScopedPointer<ComboBox> fsCbox;  // Desired RCB Sample Rate Selection

        // Actual Sample Rate
        ScopedPointer<Label> actFsLabel;
        ScopedPointer<TextEditor> actFsText;

        // RHD Up Bw
        ScopedPointer<Label> upBwLabel;
        ScopedPointer<TextEditor> upBwText;   // replaced with comboBox
        ScopedPointer<ComboBox> upBwCbox;

        // RHD Low Bw
        ScopedPointer<Label> lowBwLabel;
        ScopedPointer<TextEditor> lowBwText;  // replaced with comboBox
        ScopedPointer<ComboBox> lowBwCbox;

        // RHD DSP Cutoff
        ScopedPointer<Label> dspCutLabel;
        ScopedPointer<Label> dspCutNumLabel;
        ScopedPointer<UtilityButton> dspoffsetButton;

        // Packet Info - Hit Miss
        ScopedPointer<Label> seqNumLabel;
        
        //Battery
        ScopedPointer<Label> batteryLabel;
  
        // Intan RDH read only regs
        ScopedPointer<Label> rhdRegsLabel;

        // RCB PA Power
        ScopedPointer<Label> paPwrLabel;
        ScopedPointer<ComboBox> paPwrCbox;

        // RCB IP Addr
        ScopedPointer<Label> destIpLabel;
        ScopedPointer<Label> rcbIpNumLabel;

        // Host IP Addr
        ScopedPointer<Label> hostIpLabel;
        ScopedPointer<Label> hostIpNumLabel;

        // Port
        ScopedPointer<Label> portLabel;
        ScopedPointer<Label> portNumLabel;

        // Init Button
        ScopedPointer<Label> initLabel;
        std::unique_ptr<UtilityButton> initButton;
       


        // Parent node
        RcbWifi* node;
        void timerCallback();
        int timeInt = 0;
        //int numTsItems[8];

        // IP stuff
        //String ipNumStr;
        String triggerChStr;
        String myHostStr;
        String hostStr;
        IPAddress myHost;

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