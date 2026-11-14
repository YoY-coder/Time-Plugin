#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "TimeLookAndFeel.h"

class TimeAudioProcessorEditor;

//==============================================================================
/** Champ numérique : zone de saisie + boutons − / + (utile si le DAW
    intercepte le clavier). */
class NumberField final : public juce::Component
{
public:
    NumberField (int step, int minValue, int maxValue);

    int  getValue() const;
    void setValue (int v);
    void setFontHeight (float h);
    std::function<void()> onReturn;

    void resized() override;

private:
    juce::TextEditor editor;
    juce::TextButton minus, plus;
    int step, minValue, maxValue;
};

//==============================================================================
/** Panneau superposé (correction, temps antérieur, confirmation de reset). */
class Sheet final : public juce::Component
{
public:
    enum class Mode { Correct, Prior, Reset };

    Sheet (TimeAudioProcessorEditor& owner, TimeAudioProcessor& processor, Mode mode);

    void setScale (float newScale);
    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void focusFirstField();

private:
    juce::Rectangle<float> getCardBounds() const;
    void close();

    TimeAudioProcessorEditor& owner;
    TimeAudioProcessor& processor;
    Mode mode;
    float scale = 1.0f;

    juce::String title, message;
    std::unique_ptr<NumberField> fieldA, fieldB;     // minutes / heures + minutes
    juce::String labelA, labelB;
    juce::TextButton cancelButton, actionButton1, actionButton2;
};

//==============================================================================
/** Bouton "Sessions ▸" qui déplie / replie l'historique. */
class DisclosureButton final : public juce::Button
{
public:
    DisclosureButton() : juce::Button ("Sessions") {}
    void setExpanded (bool e)        { expanded = e; repaint(); }
    void setCountText (const juce::String& t) { if (t != countText) { countText = t; repaint(); } }
    void paintButton (juce::Graphics&, bool isMouseOver, bool isButtonDown) override;

private:
    bool expanded = false;
    juce::String countText;
};

//==============================================================================
/** Petit bouton rond soleil / lune pour passer du mode clair au mode sombre. */
class ThemeToggle final : public juce::Button
{
public:
    ThemeToggle() : juce::Button ("Theme") {}
    void paintButton (juce::Graphics&, bool isMouseOver, bool isButtonDown) override;
};

//==============================================================================
class TimeAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                       private juce::Timer,
                                       private juce::ListBoxModel
{
public:
    explicit TimeAudioProcessorEditor (TimeAudioProcessor&);
    ~TimeAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    void resized() override;

    void openSheet (Sheet::Mode mode);
    void closeSheet();
    float getScale() const noexcept { return scale; }

private:
    // Taille de référence (le tout est mis à l'échelle proportionnellement)
    static constexpr float baseWidth          = 440.0f;
    static constexpr float baseHeightCollapsed = 280.0f;
    static constexpr float baseHeightExpanded  = 510.0f;

    void timerCallback() override;
    void setExpanded (bool shouldExpand);
    void restoreSavedSize();
    void applyTheme();
    void rebuildTextures();
    void updateResizeLimits();
    juce::Rectangle<float> unit (float x, float y, float w, float h) const;

    void paintChrono (juce::Graphics&, juce::Rectangle<float> area) const;
    void paintStatus (juce::Graphics&, juce::Rectangle<float> area) const;

    // ListBoxModel (historique)
    int  getNumRows() override;
    void paintListBoxItem (int row, juce::Graphics&, int width, int height, bool selected) override;
    void listBoxItemClicked (int row, const juce::MouseEvent&) override;

    TimeAudioProcessor& processor;
    TimeLookAndFeel lookAndFeel;
    TimeSnapshot snapshot;

    juce::TextButton pauseButton, correctButton, priorButton, resetButton;
    DisclosureButton sessionsButton;
    ThemeToggle themeButton;
    juce::ListBox historyList;
    std::unique_ptr<Sheet> sheet;

    bool expanded = false;
    bool applyingSize = false;
    float scale = 1.0f;
    juce::Point<float> origin;
    bool darkMode = false;
    juce::Image backdropCache, grainCache;   // textures pré-calculées
    float textureScale = 1.0f;               // densité de pixels de l'écran

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TimeAudioProcessorEditor)
};
