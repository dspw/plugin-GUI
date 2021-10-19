/*
    ------------------------------------------------------------------

    This file is part of the Open Ephys GUI
    Copyright (C) 2016 Open Ephys

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

#ifndef __PARAMETEREDITOR_H_44537DA9__
#define __PARAMETEREDITOR_H_44537DA9__

#include "../../../JuceLibraryCode/JuceHeader.h"

#include "Parameter.h"
#include "../Editors/PopupChannelSelector.h"

class UtilityButton;

/** 
    Base class for ParameterEditors

    All custom ParameterEditors must inherit from this class.
*/
class ParameterEditor : public Component
{
public:

    ParameterEditor(Parameter* param_) : param(param_) { }

    virtual ~ParameterEditor() { }

    virtual void updateView() = 0;

    void setParameter(Parameter* param_)
    {
        param = param_;
        updateView();
    }

    bool shouldDeactivateDuringAcquisition()
    {
        return param->shouldDeactivateDuringAcquisition();
    }

    const String getParameterName() { return param->getName(); }

protected:
    Parameter* param;
};

/** 
    Allows parameters to be changed via text box.

    Only valid for IntParameter and FloatParameter

*/
class PLUGIN_API TextBoxParameterEditor : public ParameterEditor,
    public Label::Listener
{
public:
    TextBoxParameterEditor(Parameter* param);
    virtual ~TextBoxParameterEditor() { }

    void labelTextChanged(Label* label);

    virtual void updateView() override;

    virtual void resized();

private:
    ScopedPointer<Label> parameterNameLabel;
    ScopedPointer<Label> valueTextBox;
};


/**
    Allows parameters to be changed via a check box.

    Only valid for BooleanParameter

*/
class PLUGIN_API CheckBoxParameterEditor : public ParameterEditor,
    public Button::Listener
{
public:
    CheckBoxParameterEditor(Parameter* param);
    virtual ~CheckBoxParameterEditor() { }

    void buttonClicked(Button* label);

    virtual void updateView() override;

    virtual void resized();

private:
    ScopedPointer<Label> parameterNameLabel;
    ScopedPointer<ToggleButton> valueCheckBox;
};


/**
    Allows parameters to be changed via combo box.

    Only valid for BooleanParameter, IntParameter, and CategoricalParameter

*/
class PLUGIN_API ComboBoxParameterEditor : public ParameterEditor,
    public ComboBox::Listener
{
public:
    ComboBoxParameterEditor(Parameter* param);
    virtual ~ComboBoxParameterEditor() { }

    void comboBoxChanged(ComboBox* comboBox);

    virtual void updateView() override;

    virtual void resized();

private:
    ScopedPointer<Label> parameterNameLabel;
    ScopedPointer<ComboBox> valueComboBox;

    int offset;
};



/**
    Allows parameters to be changed via a slider

    Only valid for IntParameter and FloatParameter

*/
class PLUGIN_API SliderParameterEditor : public ParameterEditor,
    public Slider::Listener
{
public:
    SliderParameterEditor(Parameter* param);
    virtual ~SliderParameterEditor() { }

    void sliderValueChanged(Slider* slider);

    virtual void updateView() override;

    virtual void resized();

private:
    ScopedPointer<Label> parameterNameLabel;
    ScopedPointer<Slider> valueSlider;
};

/**
    Creates a special editor for a SelectedChannelsParameter

    Displays all of the channels in the currently active DataStream,
    and makes it possible to select them by clicking.

*/
class PLUGIN_API SelectedChannelsParameterEditor : public ParameterEditor,
    public Button::Listener,
    public PopupChannelSelector::Listener
{
public:
    SelectedChannelsParameterEditor(Parameter* param);
    virtual ~SelectedChannelsParameterEditor() { }

    void buttonClicked(Button* label);

    virtual void updateView() override;

    void channelStateChanged(Array<int> selectedChannels);

    virtual void resized();

private:
    std::unique_ptr<UtilityButton> button;
};


#endif  // __PARAMETEREDITOR_H_44537DA9__
