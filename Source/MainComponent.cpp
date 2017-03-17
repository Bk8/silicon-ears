#ifndef MAINCOMPONENT_H_INCLUDED
#define MAINCOMPONENT_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"
#include "RubberBandStretcher.h"
#include "ReferenceCountedBuffer.h"
#include <sstream>

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
                state (Stopped),
                fileLocSlider(Slider::LinearHorizontal, Slider::NoTextBox),
                thumbnailCache(5),
                thumbnail(512, formatManager, thumbnailCache)
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

                addAndMakeVisible(pitchSlider);
                pitchSlider.setRange(0.25, 4);
                pitchSlider.setValue(1.0);
                pitchSlider.addListener(this);

                addAndMakeVisible(fileLocSlider);
                fileLocSlider.setRange(0, 1);
                fileLocSlider.setValue(0);
                fileLocSlider.addListener(this);


                addAndMakeVisible(durationLabel);
                durationLabel.setText("Duration", dontSendNotification);

                addAndMakeVisible(durationSlider);
                durationSlider.setRange(0.25, 4);
                durationSlider.setValue(1.0);
                durationSlider.addListener(this);

                thumbnail.addChangeListener(this);

                setSize (300, 200);

                formatManager.registerBasicFormats();
                transportSource.addChangeListener (this);

                setAudioChannels (2, 2);
                startTimer (40);

                startThread();
        }

        ~MainContentComponent()
        {
                stopThread(10);
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
                ReferenceCountedBuffer::Ptr retainedCurrentBuffer(currentBuffer);
                if (retainedCurrentBuffer == nullptr)
                        {
                                bufferToFill.clearActiveBufferRegion();
                                return;
                        }

                AudioSampleBuffer *currentAudioSampleBuffer(retainedCurrentBuffer->getAudioSampleBuffer());

                int position = retainedCurrentBuffer->position;


                const float **bufferToRead = bufferToFill.buffer->getArrayOfReadPointers();

                // stretcher.setPitchScale(pitchSlider.getValue());
                // stretcher.setTimeRatio(durationSlider.getValue());

                // stretcher.process(bufferToRead, bufferToFill.numSamples ,false);

                // float **bufferToWrite = bufferToFill.buffer->getArrayOfWritePointers();

                //stretcher.retrieve(bufferToWrite, bufferToFill.numSamples);

        }

        void releaseResources() override
        {
                transportSource.releaseResources();
        }

        void resized() override
        {
                const int border = 10;

                openButton.setBounds (border, 10, getWidth() - 20, 20);
                playButton.setBounds (border, 40, getWidth() - 20, 20);
                stopButton.setBounds (border, 70, getWidth() - 20, 20);
                loopingToggle.setBounds(border, 100, getWidth() - 20, 20);
                currentPositionLabel.setBounds (border, 130, getWidth() - 20, 20);

                const int buttonsBottom = 150;

                const int waveFormbottom = 0;
                const int waveFormTop = 0;
                const int waveFormLeft = border;
                const int waveFormRight = 0;

                const int sliderLeft = border;
                const int sliderSpacing = 20;
                const int slidersRight = getWidth() - border - sliderSpacing;
                const int slidersBottom  = getBottom() - 30;

                fileLocSlider.setBounds(sliderLeft, getHeight() - 100, getWidth() - sliderLeft - border, 20);

                const int labelWidth = 70;
                const int labelSlidersKludge = slidersRight - 55;
                durationLabel.setBounds(labelSlidersKludge + sliderLeft, slidersBottom - sliderSpacing, labelWidth, sliderSpacing);
                pitchLabel.setBounds(labelSlidersKludge + sliderLeft, slidersBottom, labelWidth, sliderSpacing);

                const int vertSliderHeight = slidersBottom - buttonsBottom - 2 * border - sliderSpacing;
                durationSlider.setBounds(sliderLeft, slidersBottom - sliderSpacing, labelSlidersKludge, sliderSpacing);
                pitchSlider.setBounds(sliderLeft, slidersBottom, labelSlidersKludge, sliderSpacing);
        }

        void transportSourceChanged()
        {
                if (transportSource.isPlaying())
                        changeState (Playing);
                else
                        changeState (Stopped);
        }

        void thumbNailChanged()
        {
                repaint();
        }

        void changeListenerCallback (ChangeBroadcaster* source) override
        {
                if(source == &transportSource) transportSourceChanged();
                if(source == &thumbnail) thumbNailChanged();
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

                repaint();
        }

        void updateLoopState (bool shouldLoop)
        {
                if (readerSource != nullptr)
                        readerSource->setLooping (shouldLoop);
        }


private:

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

        void paintIfFileLoaded(Graphics&g, const Rectangle<int> & thumbnailBounds)
        {
                g.setColour (Colours::darkgrey);
                g.fillRect (thumbnailBounds);

                g.setColour (Colour((uint8) 89, (uint8)121, (uint8)165, .8f));

                thumbnail.drawChannel (g,
                                       thumbnailBounds,
                                       0.0,                                    // start time
                                       thumbnail.getTotalLength(),             // end time
                                       0,
                                       1.0f);
        }

        void paintIfNoFileLoaded(Graphics&g, const Rectangle<int>& thumbnailBounds)
        {
                g.setColour (Colours::darkgrey);
                g.fillRect (thumbnailBounds);
                g.setColour (Colours::white);
                g.drawFittedText ("No File Loaded", thumbnailBounds, Justification::centred, 1.0f);
        }

        void paint(Graphics &g) override
        {
                const Rectangle<int> thumbnailBounds(10, 160, getWidth() - 20, getHeight() - 120 - 160);

                if(thumbnail.getNumChannels() == 0)
                        paintIfNoFileLoaded(g, thumbnailBounds);
                else
                        paintIfFileLoaded(g, thumbnailBounds);
        }
    
        void openButtonClicked()
        {
                FileChooser chooser ("Select a Wave file to play...",
                                     File::nonexistent,
                                     "*.wav");

                std::stringstream ss;
                std::stringstream newpath;

                if (chooser.browseForFileToOpen()){
                        const File file (chooser.getResult());
                        String path = file.getFullPathName();

                        // newpath << path << ".out.wav";

                        // ss << "/usr/local/bin/rubberband -t "
                        //    << durationSlider.getValue()
                        //    << " -f "
                        //    << pitchSlider.getValue()
                        //    << " " << path << " "
                        //    << newpath.str();

                        // std::cerr << "here:" << ss.str() << std::endl;
                        // std::system(ss.str().c_str());
                        // std::cerr << "there" << std::endl;

                        // const File fuck = File(newpath.str());



                        ScopedPointer<AudioFormatReader> reader = formatManager.createReaderFor(file);
                        if (reader != nullptr){

                                thumbnail.setSource(new FileInputSource(file));

                                const double duration = reader->lengthInSamples / reader->sampleRate;

                                ReferenceCountedBuffer::Ptr newBuffer = new ReferenceCountedBuffer(file.getFileName(),
                                                                                                   reader->numChannels,
                                                                                                   reader->lengthInSamples);

                                reader->read(newBuffer->getAudioSampleBuffer(), 0, reader->lengthInSamples, 0, true, true);
                                currentBuffer = newBuffer;
                                buffers.add(newBuffer);
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
        ReferenceCountedArray<ReferenceCountedBuffer> buffers;
        ReferenceCountedBuffer::Ptr currentBuffer;

        Slider pitchSlider;
        Slider durationSlider;
        Slider fileLocSlider;
        Label pitchLabel;
        Label durationLabel;

        TextButton openButton;
        TextButton playButton;
        TextButton stopButton;
        ToggleButton loopingToggle;
        Label currentPositionLabel;

        AudioFormatManager formatManager;
        AudioThumbnailCache thumbnailCache;
        AudioThumbnail thumbnail;

        ScopedPointer<AudioFormatReaderSource> readerSource;
        AudioTransportSource transportSource;
        TransportState state;

        LookAndFeel_V3 lookAndFeel;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};


Component* createMainContentComponent()     { return new MainContentComponent(); }


#endif  // MAINCOMPONENT_H_INCLUDED
