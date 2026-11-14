#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// =============================================================================
//  Direction artistique
//  Épurée mais texturée : fond blanc cassé chaud (ou gris-noir en mode sombre),
//  halo de lumière diffuse, fines fissures et poussières en arrière-plan,
//  grain de film léger, cartes arrondies aux ombres douces.
//  Une seule couleur d'accent : orange braise.
// =============================================================================
struct Theme
{
    bool dark = false;

    juce::Colour background, backgroundLight, card, cardEdge;
    juce::Colour text, secondary, separator, control, controlHover;
    juce::Colour accent;                                   // la couleur d'accent unique
    juce::Colour green, blue, orange, grey, red;           // couleurs d'état
    juce::Colour crack, dust;                              // texture d'arrière-plan

    float shadowAlpha = 0.07f;
    float grainAlpha  = 0.04f;

    static const Theme& light();
    static const Theme& darkTheme();
};

/** Texte UTF-8 (accents) -> juce::String */
inline juce::String fr (const char* utf8)   { return juce::String::fromUTF8 (utf8); }

enum class FontWeight { Light, Regular, Medium };

/** Police système sans-serif (Helvetica Neue sur Mac, Segoe UI sur Windows). */
juce::Font uiFont (float height, FontWeight weight = FontWeight::Regular);

class TimeLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    TimeLookAndFeel();

    void setDarkMode (bool shouldBeDark);
    const Theme& getTheme() const noexcept    { return *theme; }

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

private:
    const Theme* theme = &Theme::light();
};

/** Thème actif de la fenêtre à laquelle appartient ce composant. */
const Theme& themeFor (const juce::Component& c);

//==============================================================================
/** Logo : chronomètre stylisé (fixe), dessiné en vectoriel dans 'area' (carré). */
void drawStopwatchLogo (juce::Graphics&, juce::Rectangle<float> area, const Theme&);

//==============================================================================
//  Textures (générées une fois, puis mises en cache par l'éditeur)
namespace Texture
{
    /** Fond : couleur, halo de lumière, fissures fines, poussières. */
    void paintBackdrop (juce::Graphics&, juce::Rectangle<float> area, float scale, const Theme&);

    /** Grain de film (bruit) à superposer sur tout. */
    juce::Image makeGrain (int width, int height, const Theme&);
}
