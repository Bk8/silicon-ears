#ifndef MAINCOMPONENT_H_INCLUDED
#define MAINCOMPONENT_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"
#include "rubberband/RubberBandStretcher.h"
#include "ReferenceCountedBuffer.h"

class MainContentComponent   : public AudioAppComponent,
                               public ChangeListener,
                               public Button::Listener,
                               public Slider::Listener,
                               public Timer,
                               private Thread
{
public:

        MainContentComponent()
                :
                Thread("background-thread"),
                stretcher(44100, 1,
                          RubberBand::RubberBandStretcher::OptionProcessRealTime,
                          1.0,
                          1.0),
                state (Stopped)
        {
                setLookAndFeel (&lookAndFeel);
                addAndMakeVisible (&openButton);
                openButton.setButtonText ("Open...");
                openButton.addListener (this);
                addAndMakeVisible (&playButton);
                playButton.setButtonText ("Play");
                playButton.addListener (this);
                playButton.setColour (TextButton::buttonColourId, Colours::green);
                playButton.setEnabled (false);

                addAndMakeVisible (&stopButton);
                stopButton.setButtonText ("Stop");
                stopButton.addListener (this);
                stopButton.setColour (TextButton::buttonColourId, Colours::red);
                stopButton.setEnabled (false);

                addAndMakeVisible (&loopingToggle);
                loopingToggle.setButtonText ("Loop");
                loopingToggle.addListener (this);

                addAndMakeVisible (&currentPositionLabel);
                currentPositionLabel.setText ("Stopped", dontSendNotification);

                addAndMakeVisible(pitchLabel);
                pitchLabel.setText("Pitch", dontSendNotification);
                pitchLabel.attachToComponent(&pitchSlider, true);

                addAndMakeVisible(pitchSlider);
                pitchSlider.setRange(0.25, 4);
                pitchSlider.setValue(1.0);
                pitchSlider.addListener(this);

                addAndMakeVisible(durationLabel);
                durationLabel.setText("Duration", dontSendNotification);
                durationLabel.attachToComponent(&durationSlider, true);

                addAndMakeVisible(durationSlider);
                durationSlider.setRange(0.25, 4);
                durationSlider.setValue(1.0);
                durationSlider.addListener(this);

                setSize (300, 200);

                formatManager.registerBasicFormats();
                transportSource.addChangeListener (this);

                setAudioChannels (2, 2);
                startTimer (20);

                startThread();
        }

        ~MainContentComponent()
        {
                shutdownAudio();
        }

        //------------------------------------------------------------------------------
        // Thread Code
        //------------------------------------------------------------------------------
        void checkForBuffersToFree()
        {
                for(int i = buffers.size(); --i >= 0;){
                        ReferenceCountedBuffer::Ptr buffer (buffers.getUnchecked(i));

                        if(buffer->getReferenceCount() == 2)
                                buffers.remove(i);
                }
        }
        void run() override
        {
                while(! threadShouldExit()) {
                        checkForBuffersToFree();
                        wait(500);
                }
        }
        //------------------------------------------------------------------------------


        void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override
        {
                transportSource.prepareToPlay (samplesPerBlockExpected, sampleRate);
        }

        void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override
        {
                if (readerSource == nullptr)
                        {
                                bufferToFill.clearActiveBufferRegion();
                                return;
                        }

                transportSource.getNextAudioBlock (bufferToFill);


                const float **bufferToRead = bufferToFill.buffer->getArrayOfReadPointers();

                stretcher.setPitchScale(pitchSlider.getValue());
                stretcher.setTimeRatio(durationSlider.getValue());

                stretcher.process(bufferToRead, bufferToFill.numSamples ,false);

                float **bufferToWrite = bufferToFill.buffer->getArrayOfWritePointers();

                stretcher.retrieve(bufferToWrite, bufferToFill.numSamples);

        }

        void releaseResources() override
        {
                transportSource.releaseResources();
        }

        void resized() override
        {
                openButton.setBounds (10, 10, getWidth() - 20, 20);
                playButton.setBounds (10, 40, getWidth() - 20, 20);
                stopButton.setBounds (10, 70, getWidth() - 20, 20);
                loopingToggle.setBounds(10, 100, getWidth() - 20, 20);
                currentPositionLabel.setBounds (10, 130, getWidth() - 20, 20);

                const int sliderLeft = 120;
                pitchSlider.setBounds(sliderLeft, 150, getWidth() - sliderLeft - 10, 20);
                durationSlider.setBounds(sliderLeft, 170, getWidth() - sliderLeft - 10, 20);
        }

        void changeListenerCallback (ChangeBroadcaster* source) override
        {
                if (source == &transportSource)
                        {
                                if (transportSource.isPlaying())
                                        changeState (Playing);
                                else
                                        changeState (Stopped);
                        }
        }

        void buttonClicked (Button* button) override
        {
                if (button == &openButton)      openButtonClicked();
                if (button == &playButton)      playButtonClicked();
                if (button == &stopButton)      stopButtonClicked();
                if (button == &loopingToggle)   loopButtonChanged();
        }

        void timerCallback() override
        {
                if (transportSource.isPlaying())
                        {
                                const RelativeTime position (transportSource.getCurrentPosition());

                                const int minutes = ((int) position.inMinutes()) % 60;
                                const int seconds = ((int) position.inSeconds()) % 60;
                                const int millis  = ((int) position.inMilliseconds()) % 1000;

                                const String positionString (String::formatted ("%02d:%02d:%03d", minutes, seconds, millis));

                                currentPositionLabel.setText (positionString, dontSendNotification);
                        }
                else
                        {
                                currentPositionLabel.setText ("Stopped", dontSendNotification);
                        }
        }

        void updateLoopState (bool shouldLoop)
        {
                if (readerSource != nullptr)
                        readerSource->setLooping (shouldLoop);
        }


private:
        ReferenceCountedArray<ReferenceCountedBuffer> buffers;
        ReferenceCountedBuffer::Ptr currentBuffer;

        Slider pitchSlider;
        Slider durationSlider;
        Label pitchLabel;
        Label durationLabel;
        RubberBand::RubberBandStretcher stretcher;
        enum TransportState
                {
                        Stopped,
                        Starting,
                        Playing,
                        Stopping
                };

        void changeState (TransportState newState)
        {
                if (state != newState) {
                        state = newState;

                        switch (state) {
                        case Stopped:
                                stopButton.setEnabled (false);
                                playButton.setEnabled (true);
                                transportSource.setPosition (0.0);
                                break;

                        case Starting:
                                playButton.setEnabled (false);
                                transportSource.start();
                                break;

                        case Playing:
                                stopButton.setEnabled (true);
                                break;

                        case Stopping:
                                transportSource.stop();
                                break;
                        }
                }
        }

        void sliderValueChanged(Slider* slider) override
        {
                if (slider == &pitchSlider)
                        pitchSlider.setValue(pitchSlider.getValue());
                else if (slider == &durationSlider)
                        durationSlider.setValue(durationSlider.getValue());
        }

        void openButtonClicked()
        {
                FileChooser chooser ("Select a Wave file to play...",
                                     File::nonexistent,
                                     "*.wav");

                if (chooser.browseForFileToOpen()){
                        const File file (chooser.getResult());
                        AudioFormatReader* reader = formatManager.createReaderFor(file);
                        if (reader != nullptr){
                                ScopedPointer<AudioFormatReaderSource> newSource =
                                        new AudioFormatReaderSource (reader, true);
                                transportSource.setSource (newSource, 0, nullptr, reader->sampleRate);
                                playButton.setEnabled (true);
                                readerSource = newSource.release();
                        }
                }
        }

        void playButtonClicked()
        {
                updateLoopState (loopingToggle.getToggleState());
                changeState (Starting);
        }

        void stopButtonClicked()
        {
                changeState (Stopping);
        }

        void loopButtonChanged()
        {
                updateLoopState (loopingToggle.getToggleState());
        }

        //==========================================================================
        TextButton openButton;
        TextButton playButton;
        TextButton stopButton;
        ToggleButton loopingToggle;
        Label currentPositionLabel;

        AudioFormatManager formatManager;
        ScopedPointer<AudioFormatReaderSource> readerSource;
        AudioTransportSource transportSource;
        TransportState state;

        LookAndFeel_V3 lookAndFeel;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};


Component* createMainContentComponent()     { return new MainContentComponent(); }


#endif  // MAINCOMPONENT_H_INCLUDED
