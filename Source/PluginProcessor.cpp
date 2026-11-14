#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <algorithm>
#include <cmath>

//==============================================================================
TimeAudioProcessor::TimeAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      clock (monotonicNow())
{
    startNewSession();

    // Sonde d'activité toutes les 250 ms. Le comptage vit ici, dans le
    // processeur : il continue même quand la fenêtre du plugin est fermée.
    startTimer (250);
}

TimeAudioProcessor::~TimeAudioProcessor()
{
    stopTimer();
}

void TimeAudioProcessor::startNewSession()
{
    currentSessionId      = juce::Uuid().toString();
    currentSessionStartMs = wallNow();
    lastCountedWallMs     = 0;
    lastCommitted         = 0.0;
    clock.reset (monotonicNow());
}

//==============================================================================
//  Audio : on ne touche pas au son. On note seulement si le transport joue.
//==============================================================================
void TimeAudioProcessor::prepareToPlay (double, int)
{
    setLatencySamples (0);
}

bool TimeAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();

    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;

    return layouts.getMainInputChannelSet() == out;   // mono->mono ou stéréo->stéréo
}

template <typename SampleType>
void TimeAudioProcessor::trackPlayback (juce::AudioBuffer<SampleType>& buffer)
{
    // L'audio traverse le plugin sans aucune modification (traitement "en place").
    // On ne fait que vider d'éventuels canaux de sortie sans entrée correspondante.
    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    bool playing = isNonRealtime();   // un export offline compte comme de la lecture

    if (auto* ph = getPlayHead())
        if (auto position = ph->getPosition())
            playing = playing || position->getIsPlaying();

    if (playing)
    {
        const double now = monotonicNow();

        if (! audioWasPlaying || now - lastPlayingBlock.load() > 1.0)
            playStart.store (now);

        lastPlayingBlock.store (now);
    }

    audioWasPlaying = playing;
    hostPlaying.store (playing);
}

void TimeAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    trackPlayback (buffer);
}

void TimeAudioProcessor::processBlock (juce::AudioBuffer<double>& buffer, juce::MidiBuffer&)
{
    trackPlayback (buffer);
}

//==============================================================================
//  Sonde (thread de l'interface, toutes les 250 ms)
//==============================================================================
void TimeAudioProcessor::timerCallback()
{
    const bool activity = probe.poll();
    const double now = monotonicNow();

    PlaybackInfo playback;
    playback.lastPlayingBlock = lastPlayingBlock.load();
    playback.playStart        = playStart.load();
    // Si processBlock n'est plus appelé depuis plus d'1 s : lecture arrêtée.
    playback.playingNow       = hostPlaying.load()
                                && playback.lastPlayingBlock >= 0.0
                                && now - playback.lastPlayingBlock < 1.0;

    const juce::ScopedLock sl (lock);
    clock.tick (now, activity, playback);

    if (clock.getCommittedSeconds() > lastCommitted)
    {
        lastCommitted     = clock.getCommittedSeconds();
        lastCountedWallMs = wallNow();
    }
}

//==============================================================================
//  API pour l'interface
//==============================================================================
TimeSnapshot TimeAudioProcessor::getSnapshot() const
{
    const double now = monotonicNow();
    const juce::ScopedLock sl (lock);

    TimeSnapshot snap;
    snap.paused               = clock.isPaused();
    snap.state                = clock.getState (now);
    snap.pendingSeconds       = clock.getPendingSeconds (now);
    snap.secondsBeforeDiscard = clock.getSecondsBeforeDiscard (now);
    snap.priorSeconds         = priorSeconds;

    const double current = clock.getCommittedSeconds() + snap.pendingSeconds;
    double total = current + priorSeconds;

    snap.rows.reserve (pastSessions.size() + adjustments.size() + 2);

    HistoryRow cur;
    cur.kind    = HistoryRow::Kind::CurrentSession;
    cur.startMs = currentSessionStartMs;
    cur.endMs   = wallNow();
    cur.seconds = current;
    snap.rows.push_back (cur);

    for (auto& s : pastSessions)
    {
        total += s.activeSeconds;
        snap.rows.push_back ({ HistoryRow::Kind::Session, s.startMs, s.endMs, s.activeSeconds });
    }

    for (auto& a : adjustments)
    {
        total += a.seconds;
        snap.rows.push_back ({ HistoryRow::Kind::Adjustment, a.timeMs, a.timeMs, a.seconds });
    }

    // Tri : la plus récente en haut (la session en cours reste toujours en tête)
    std::stable_sort (snap.rows.begin() + 1, snap.rows.end(),
                      [] (const HistoryRow& a, const HistoryRow& b) { return a.endMs > b.endMs; });

    if (priorSeconds > 0.0)
        snap.rows.push_back ({ HistoryRow::Kind::Prior, 0, 0, priorSeconds });

    snap.totalSeconds = std::max (0.0, total);
    return snap;
}

void TimeAudioProcessor::togglePause()
{
    const juce::ScopedLock sl (lock);
    clock.setPaused (! clock.isPaused(), monotonicNow());

    if (clock.getCommittedSeconds() > lastCommitted)
    {
        lastCommitted     = clock.getCommittedSeconds();
        lastCountedWallMs = wallNow();
    }
}

void TimeAudioProcessor::addAdjustment (double seconds)
{
    if (std::abs (seconds) < 0.5)
        return;

    const juce::ScopedLock sl (lock);
    adjustments.push_back ({ juce::Uuid().toString(), wallNow(), seconds });
}

void TimeAudioProcessor::setPriorSeconds (double seconds)
{
    const juce::ScopedLock sl (lock);
    priorSeconds = std::max (0.0, seconds);
}

void TimeAudioProcessor::resetAll()
{
    const juce::ScopedLock sl (lock);
    pastSessions.clear();
    adjustments.clear();
    priorSeconds = 0.0;
    startNewSession();
}

juce::Point<int> TimeAudioProcessor::getSavedEditorSize() const
{
    return { editorWidth.load(), editorHeight.load() };
}

void TimeAudioProcessor::setSavedEditorSize (int width, int height)
{
    editorWidth.store (width);
    editorHeight.store (height);
}

//==============================================================================
//  Sauvegarde dans le projet du DAW
//==============================================================================
double TimeAudioProcessor::currentSessionSecondsForSave (double now) const
{
    // Le temps en attente est inclus : enregistrer le projet est une action.
    // S'il est retiré plus tard, la prochaine sauvegarde corrigera la valeur.
    return clock.getCommittedSeconds() + clock.getPendingSeconds (now);
}

void TimeAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    const double now = monotonicNow();
    const juce::ScopedLock sl (lock);

    juce::XmlElement root ("TIME");
    root.setAttribute ("formatVersion", stateFormatVersion);
    root.setAttribute ("paused",        clock.isPaused());
    root.setAttribute ("priorSeconds",  priorSeconds);
    root.setAttribute ("editorWidth",   editorWidth.load());
    root.setAttribute ("editorHeight",  editorHeight.load());

    auto* sessionsXml = root.createNewChildElement ("SESSIONS");

    auto writeSession = [sessionsXml] (const SessionRecord& s)
    {
        auto* e = sessionsXml->createNewChildElement ("SESSION");
        e->setAttribute ("id",      s.id);
        e->setAttribute ("start",   juce::String (s.startMs));
        e->setAttribute ("end",     juce::String (s.endMs));
        e->setAttribute ("seconds", s.activeSeconds);
    };

    for (auto& s : pastSessions)
        writeSession (s);

    // La session en cours est enregistrée seulement si elle contient du temps actif.
    const double current = currentSessionSecondsForSave (now);

    if (current >= 1.0)
    {
        SessionRecord s;
        s.id            = currentSessionId;
        s.startMs       = currentSessionStartMs;
        s.endMs         = clock.getPendingSeconds (now) > 0.0 ? wallNow()
                                                              : std::max (lastCountedWallMs, currentSessionStartMs);
        s.activeSeconds = current;
        writeSession (s);
    }

    auto* adjXml = root.createNewChildElement ("ADJUSTMENTS");

    for (auto& a : adjustments)
    {
        auto* e = adjXml->createNewChildElement ("ADJUSTMENT");
        e->setAttribute ("id",      a.id);
        e->setAttribute ("time",    juce::String (a.timeMs));
        e->setAttribute ("seconds", a.seconds);
    }

    copyXmlToBinary (root, destData);
}

void TimeAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);

    if (xml == nullptr || ! xml->hasTagName ("TIME"))
        return;

    // Un état créé par une future version plus récente reste lisible tant que
    // les champs connus sont présents ; les champs inconnus sont ignorés.
    const int version = xml->getIntAttribute ("formatVersion", 1);
    juce::ignoreUnused (version);

    const juce::ScopedLock sl (lock);

    pastSessions.clear();
    adjustments.clear();

    if (auto* sessionsXml = xml->getChildByName ("SESSIONS"))
    {
        for (auto* e : sessionsXml->getChildWithTagNameIterator ("SESSION"))
        {
            SessionRecord s;
            s.id            = e->getStringAttribute ("id");
            s.startMs       = e->getStringAttribute ("start").getLargeIntValue();
            s.endMs         = e->getStringAttribute ("end").getLargeIntValue();
            s.activeSeconds = e->getDoubleAttribute ("seconds");

            // La session en cours est déjà en mémoire (cas où le DAW recharge
            // l'état pendant que le projet est ouvert) : pas de doublon.
            if (s.id == currentSessionId || s.activeSeconds <= 0.0)
                continue;

            pastSessions.push_back (s);
        }
    }

    if (auto* adjXml = xml->getChildByName ("ADJUSTMENTS"))
    {
        for (auto* e : adjXml->getChildWithTagNameIterator ("ADJUSTMENT"))
        {
            AdjustmentRecord a;
            a.id      = e->getStringAttribute ("id");
            a.timeMs  = e->getStringAttribute ("time").getLargeIntValue();
            a.seconds = e->getDoubleAttribute ("seconds");
            adjustments.push_back (a);
        }
    }

    priorSeconds = std::max (0.0, xml->getDoubleAttribute ("priorSeconds", 0.0));

    clock.setPaused (xml->getBoolAttribute ("paused", false), monotonicNow());

    editorWidth.store  (juce::jlimit (200, 4000, xml->getIntAttribute ("editorWidth",  defaultWidth)));
    editorHeight.store (juce::jlimit (150, 4000, xml->getIntAttribute ("editorHeight", defaultHeight)));

    // L'interface (si ouverte) reprendra cette taille à son prochain rafraîchissement.
    sizeRestoreRequested.store (true);
}

//==============================================================================
juce::AudioProcessorEditor* TimeAudioProcessor::createEditor()
{
    return new TimeAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TimeAudioProcessor();
}
