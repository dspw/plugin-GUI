/*
    ------------------------------------------------------------------

    This file is part of the Open Ephys GUI
    Copyright (C) 2013 Open Ephys

    ------------------------------------------------------------------

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/


#ifndef __FILEREADEREDITOR_H_D6EC8B48__
#define __FILEREADEREDITOR_H_D6EC8B48__

#include "../../../JuceLibraryCode/JuceHeader.h"
#include "../Editors/GenericEditor.h"
#include "../../Utils/Utils.h"

class FileReader;
class FileReaderEditor;
class DualTimeComponent;
class FileSource;

/**

  User interface for the "FileReader" source node.

  @see SourceNode, FileReaderThread

*/

class PlaybackButton : public Button
{
public:
    PlaybackButton(FileReader*);

    ~PlaybackButton();
private:

    FileReader* fileReader;
    
    void paintButton(Graphics &g, bool isMouseOver, bool isButtonDown);
};

class FullTimeline : public Component
{
public:
    FullTimeline(FileReader*);
    ~FullTimeline();

    void setIntervalPosition(int start, int width);

    int getStartInterval();
    int getIntervalWidth();

private:

    FileReader* fileReader;

    int intervalStartPosition;
    int intervalWidth;
    bool intervalIsSelected;

    void paint(Graphics& g);
    void mouseDown(const MouseEvent& event);
    void mouseDrag(const MouseEvent& event);
    void mouseUp(const MouseEvent& event);

    bool leftSliderIsSelected;
};

class ZoomTimeline : public Component, MouseListener
{
public:
    ZoomTimeline(FileReader*);
    ~ZoomTimeline();

    void updatePlaybackRegion(int min, int max);
    int getStartInterval();
    int getIntervalDurationInSeconds();

private:

    FileReader* fileReader;

    int sliderWidth;
    int widthInSeconds;
    float leftSliderPosition;
    float rightSliderPosition;
    float lastDragXPosition;

    void paint(Graphics& g);
    void mouseDown(const MouseEvent& event);
    void mouseDrag(const MouseEvent& event);
    void mouseUp(const MouseEvent& event);

    bool leftSliderIsSelected;
    bool rightSliderIsSelected;
    bool playbackRegionIsSelected;

};

class ScrubDrawerButton : public DrawerButton
{
public:
	ScrubDrawerButton(const String& name);
	~ScrubDrawerButton();
private:
	void paintButton(Graphics& g, bool isMouseOver, bool isButtonDown) override;
};

class FileReaderEditor  : public GenericEditor
                        , public FileDragAndDropTarget
                        , public ComboBox::Listener
                        , public Button::Listener
{
public:
    FileReaderEditor (GenericProcessor* parentNode);
    virtual ~FileReaderEditor();

    void paintOverChildren (Graphics& g) override;

    void buttonClicked (Button* button) override;

    void saveCustomParametersToXml (XmlElement*) override;
    void loadCustomParametersFromXml (XmlElement*) override;

    // FileDragAndDropTarget methods
    // ============================================
    bool isInterestedInFileDrag (const StringArray& files)  override;
    void fileDragExit           (const StringArray& files)  override;
    void filesDropped           (const StringArray& files, int x, int y)  override;
    void fileDragEnter          (const StringArray& files, int x, int y)  override;

    bool setPlaybackStartTime (unsigned int ms);
    bool setPlaybackStopTime  (unsigned int ms);
    void setTotalTime   (unsigned int ms);
    void setCurrentTime (unsigned int ms);

	void startAcquisition() override;
	void stopAcquisition()  override;

    void setFile (String file);

    void comboBoxChanged (ComboBox* combo);
    void populateRecordings (FileSource* source);

    void showScrubInterface(bool show);
    void updateScrubInterface(bool reset);

    void updateZoomTimeLabels();
    int getFullTimelineStartPosition();
    int getZoomTimelineStartPosition();
    void updatePlaybackTimes();

    void togglePlayback();

    Array<Colour> channelColours;

    ScopedPointer<PlaybackButton>       playbackButton;

private:
    void clearEditor();

    ScopedPointer<ScrubDrawerButton>    scrubDrawerButton;

    ScopedPointer<UtilityButton>        fileButton;
    ScopedPointer<Label>                fileNameLabel;
    ScopedPointer<ComboBox>             recordSelector;
    ScopedPointer<DualTimeComponent>    currentTime;
    ScopedPointer<DualTimeComponent>    timeLimits;

    //ScrubbingInterface controls
    ScopedPointer<Label>                zoomStartTimeLabel;
    ScopedPointer<Label>                zoomMiddleTimeLabel;
    ScopedPointer<Label>                zoomEndTimeLabel;
    ScopedPointer<Label>                fullStartTimeLabel;
    ScopedPointer<Label>                fullEndTimeLabel;
    ScopedPointer<FullTimeline>         fullTimeline;
    ScopedPointer<ZoomTimeline>         zoomTimeline;

    FileReader* fileReader;
    unsigned int recTotalTime;

    bool m_isFileDragAndDropActive;
    bool scrubInterfaceVisible;
    int scrubInterfaceWidth;

    File lastFilePath;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FileReaderEditor);
};


class DualTimeComponent : public Component
                        , public Label::Listener
                        , public AsyncUpdater
{
public:
    DualTimeComponent (FileReaderEditor* e, bool isEditable);
    ~DualTimeComponent();

    void paint (Graphics& g) override;

    void labelTextChanged (Label* label) override;

    void handleAsyncUpdate() override;

    void setEnable(bool enable);

    void setTimeMilliseconds (unsigned int index, unsigned int time);
    unsigned int getTimeMilliseconds (unsigned int index) const;


private:
    ScopedPointer<Label> timeLabel[2];
    String labelText[2];
    unsigned int msTime[2];

    FileReaderEditor* editor;
    bool isEditable;
};



#endif  // __FILEREADEREDITOR_H_D6EC8B48__
