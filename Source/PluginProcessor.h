#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <atomic>
#include <vector>

#include "LiveClock.h"
#include "ActivityProbe.h"

// Une session passée (ouverture du projet -> fermeture)
struct SessionRecord
{
    juce::String id;
    juce::int64  startMs = 0;      // heure murale (ms depuis 1970)
    juce::int64  endMs   = 0;
    double       activeSeconds = 0.0;
};

// Une correction manuelle (+/- minutes)
struct AdjustmentRecord
{
    juce::String id;
    juce::int64  timeMs  = 0;
    double       seconds = 0.0;    // positif = ajout, négatif = retrait
};

// Une ligne de l'historique telle qu'affichée par l'interface
struct HistoryRow
{
    enum class Kind { CurrentSession, Session, Adjustment, Prior };

    Kind        kind    = Kind::Session;
    juce::int64 startMs = 0;
    juce::int64 endMs   = 0;
    double      seconds = 0.0;
};

// Tout ce dont l'interface a besoin pour s'afficher, copié d'un coup.
struct TimeSnapshot
{
    double           totalSeconds         = 0.0;
    double           pendingSeconds       = 0.0;
    double           secondsBeforeDiscard = 0.0;
    double           priorSeconds         = 0.0;
    LiveClock::State state                = LiveClock::State::Stopped;
    bool             paused               = false;
    std::vector<HistoryRow> rows;          // la plus récente en premier
};

//==============================================================================
class TimeAudioProcessor final : public juce::AudioProcessor,
                                 private juce::Timer
{
public:
    TimeAudioProcessor();
    ~TimeAudioProcessor() override;

    //==========================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlock (juce::AudioBuffer<double>&, juce::MidiBuffer&) override;
    bool supportsDoublePrecisionProcessing() const override { return true; }

    //==========================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override                         { return true; }

    const juce::String getName() const override             { return JucePlugin_Name; }
    bool acceptsMidi() const override                       { return false; }
    bool producesMidi() const override                      { return false; }
    bool isMidiEffect() const override                      { return false; }
    double getTailLengthSeconds() const override            { return 0.0; }

    int getNumPrograms() override                           { return 1; }
    int getCurrentProgram() override                        { return 0; }
    void setCurrentProgram (int) override                   {}
    const juce::String getProgramName (int) override        { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==========================================================================
    //  API utilisée par l'interface (thread de l'interface)
    TimeSnapshot getSnapshot() const;
    void togglePause();
    void addAdjustment (double seconds);
    void setPriorSeconds (double seconds);
    void resetAll();

    juce::Point<int> getSavedEditorSize() const;
    void setSavedEditorSize (int width, int height);

    /** true (une seule fois) si le DAW vient de recharger une taille de fenêtre. */
    bool consumeSizeRestoreRequest()    { return sizeRestoreRequested.exchange (false); }

    static constexpr int defaultWidth  = 440;
    static constexpr int defaultHeight = 280;

private:
    static constexpr int stateFormatVersion = 1;

    void timerCallback() override;
    template <typename SampleType> void trackPlayback (juce::AudioBuffer<SampleType>&);

    static double monotonicNow()  { return juce::Time::getMillisecondCounterHiRes() * 0.001; }
    static juce::int64 wallNow()  { return juce::Time::currentTimeMillis(); }

    void startNewSession();
    double currentSessionSecondsForSave (double now) const;

    // --- Données partagées avec le thread audio (sans verrou) ---------------
    std::atomic<bool>   hostPlaying      { false };
    std::atomic<double> playStart        { -1.0 };
    std::atomic<double> lastPlayingBlock { -1.0 };
    bool audioWasPlaying = false;   // utilisé uniquement par le thread audio

    // --- Données protégées par 'lock' --------------------------------------
    mutable juce::CriticalSection lock;
    LiveClock      clock;
    ActivityProbe  probe;

    juce::String                  currentSessionId;
    juce::int64                   currentSessionStartMs = 0;
    juce::int64                   lastCountedWallMs     = 0;
    double                        lastCommitted         = 0.0;

    std::vector<SessionRecord>    pastSessions;
    std::vector<AdjustmentRecord> adjustments;
    double                        priorSeconds = 0.0;

    std::atomic<int> editorWidth  { defaultWidth };
    std::atomic<int> editorHeight { defaultHeight };
    std::atomic<bool> sizeRestoreRequested { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TimeAudioProcessor)
};
