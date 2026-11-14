#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// =============================================================================
//  Direction artistique : épurée, inspirée de l'environnement Apple.
//  Fond clair légèrement cassé, cartes blanches arrondies, ombres très douces,
//  police système, une seule couleur d'accent.
// =============================================================================
namespace Palette
{
    const juce::Colour background   { 0xfff5f5f7 };
    const juce::Colour card         { 0xffffffff };
    const juce::Colour text         { 0xff1d1d1f };
    const juce::Colour secondary    { 0xff86868b };
    const juce::Colour separator    { 0xffe5e5ea };
    const juce::Colour control      { 0xfff2f2f7 };
    const juce::Colour controlHover { 0xffe8e8ed };
    const juce::Colour accent       { 0xff007aff };   // la couleur d'accent unique

    // Couleurs d'état
    const juce::Colour green        { 0xff34c759 };
    const juce::Colour blue         { 0xff007aff };
    const juce::Colour orange       { 0xffff9500 };
    const juce::Colour grey         { 0xff8e8e93 };
    const juce::Colour red          { 0xffff3b30 };
}

/** Texte UTF-8 (accents) -> juce::String */
inline juce::String fr (const char* utf8)   { return juce::String::fromUTF8 (utf8); }

enum class FontWeight { Light, Regular, Medium };

/** Police système sans-serif (Helvetica Neue sur Mac, Segoe UI sur Windows). */
juce::Font uiFont (float height, FontWeight weight = FontWeight::Regular);

class TimeLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    TimeLookAndFeel();

    // Propriété de bouton "style" : "primary", "destructive", "plain" (défaut)
    static constexpr const char* styleProperty = "timeStyle";

    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour& backgroundColour,
                               bool isMouseOver, bool isButtonDown) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool isMouseOver, bool isButtonDown) override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;

    void fillTextEditorBackground (juce::Graphics&, int width, int height, juce::TextEditor&) override;
    void drawTextEditorOutline (juce::Graphics&, int width, int height, juce::TextEditor&) override;

    void drawScrollbar (juce::Graphics&, juce::ScrollBar&, int x, int y, int width, int height,
                        bool isScrollbarVertical, int thumbStartPosition, int thumbSize,
                        bool isMouseOver, bool isMouseDown) override;
    int getDefaultScrollbarWidth() override   { return 10; }

    void drawCornerResizer (juce::Graphics&, int w, int h, bool isMouseOver, bool isMouseDragging) override;
};
