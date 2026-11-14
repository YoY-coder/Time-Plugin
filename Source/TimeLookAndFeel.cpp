#include "TimeLookAndFeel.h"

juce::Font uiFont (float height, FontWeight weight)
{
   #if JUCE_MAC
    const juce::String name ("Helvetica Neue");
    const juce::String style = weight == FontWeight::Light  ? "Light"
                             : weight == FontWeight::Medium ? "Medium" : "Regular";
   #elif JUCE_WINDOWS
    const juce::String name ("Segoe UI");
    const juce::String style = weight == FontWeight::Light  ? "Light"
                             : weight == FontWeight::Medium ? "Semibold" : "Regular";
   #else
    const juce::String name (juce::Font::getDefaultSansSerifFontName());
    const juce::String style = weight == FontWeight::Medium ? "Bold" : "Regular";
   #endif

    return juce::Font (juce::FontOptions (name, style, juce::jmax (1.0f, height)));
}

TimeLookAndFeel::TimeLookAndFeel()
{
    setColour (juce::TextButton::textColourOffId,           Palette::accent);
    setColour (juce::TextButton::textColourOnId,            juce::Colours::white);
    setColour (juce::TextEditor::textColourId,              Palette::text);
    setColour (juce::TextEditor::highlightColourId,         Palette::accent.withAlpha (0.25f));
    setColour (juce::TextEditor::highlightedTextColourId,   Palette::text);
    setColour (juce::CaretComponent::caretColourId,         Palette::accent);
    setColour (juce::ListBox::backgroundColourId,           juce::Colours::transparentBlack);
    setColour (juce::ListBox::outlineColourId,              juce::Colours::transparentBlack);
    setColour (juce::ScrollBar::thumbColourId,              Palette::grey.withAlpha (0.45f));
}

void TimeLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour&,
                                            bool isMouseOver, bool isButtonDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);
    const float radius = bounds.getHeight() * 0.5f;     // forme "pilule"
    const auto  style  = button.getProperties()[styleProperty].toString();

    juce::Colour fill = Palette::control;

    if (style == "primary" || button.getToggleState())
        fill = Palette::accent;
    else if (style == "destructive")
        fill = Palette::red;
    else if (isMouseOver)
        fill = Palette::controlHover;

    if (style == "primary" || style == "destructive" || button.getToggleState())
    {
        if (isMouseOver)  fill = fill.brighter (0.08f);
    }

    if (isButtonDown)
        fill = fill.darker (0.08f);

    if (! button.isEnabled())
        fill = fill.withMultipliedAlpha (0.5f);

    g.setColour (fill);
    g.fillRoundedRectangle (bounds, radius);
}

juce::Font TimeLookAndFeel::getTextButtonFont (juce::TextButton&, int buttonHeight)
{
    return uiFont ((float) buttonHeight * 0.42f, FontWeight::Medium);
}

void TimeLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button, bool, bool)
{
    const auto style = button.getProperties()[styleProperty].toString();
    const bool filled = style == "primary" || style == "destructive" || button.getToggleState();

    g.setColour (filled ? juce::Colours::white : Palette::accent);
    g.setFont (getTextButtonFont (button, button.getHeight()));

    auto area = button.getLocalBounds().reduced (juce::jmax (2, button.getHeight() / 4), 0);
    g.drawFittedText (button.getButtonText(), area, juce::Justification::centred, 1, 0.8f);
}

void TimeLookAndFeel::fillTextEditorBackground (juce::Graphics& g, int width, int height, juce::TextEditor&)
{
    g.setColour (Palette::control);
    g.fillRoundedRectangle (juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height),
                            (float) height * 0.25f);
}

void TimeLookAndFeel::drawTextEditorOutline (juce::Graphics& g, int width, int height, juce::TextEditor& editor)
{
    if (editor.hasKeyboardFocus (true))
    {
        g.setColour (Palette::accent.withAlpha (0.6f));
        g.drawRoundedRectangle (juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height).reduced (1.0f),
                                (float) height * 0.25f, 1.5f);
    }
}

void TimeLookAndFeel::drawScrollbar (juce::Graphics& g, juce::ScrollBar&, int x, int y, int width, int height,
                                     bool isScrollbarVertical, int thumbStartPosition, int thumbSize,
                                     bool isMouseOver, bool isMouseDown)
{
    juce::Rectangle<float> thumb;

    if (isScrollbarVertical)
        thumb = { (float) x + (float) width * 0.3f, (float) thumbStartPosition, (float) width * 0.4f, (float) thumbSize };
    else
        thumb = { (float) thumbStartPosition, (float) y + (float) height * 0.3f, (float) thumbSize, (float) height * 0.4f };

    thumb = thumb.reduced (0.0f, 2.0f);
    const float alpha = isMouseDown ? 0.7f : (isMouseOver ? 0.55f : 0.35f);
    g.setColour (Palette::grey.withAlpha (alpha));
    g.fillRoundedRectangle (thumb, juce::jmin (thumb.getWidth(), thumb.getHeight()) * 0.5f);
}

void TimeLookAndFeel::drawCornerResizer (juce::Graphics& g, int w, int h, bool isMouseOver, bool isMouseDragging)
{
    // Coin de redimensionnement discret : trois points en diagonale
    const float size = (float) juce::jmin (w, h);
    const float dot  = juce::jmax (2.0f, size * 0.14f);
    g.setColour (Palette::grey.withAlpha (isMouseDragging ? 0.8f : (isMouseOver ? 0.6f : 0.35f)));

    for (int i = 0; i < 3; ++i)
        for (int j = 0; j <= i; ++j)
            g.fillEllipse ((float) w - (float) (j + 1) * dot * 1.9f,
                           (float) h - (float) (i - j + 1) * dot * 1.9f, dot, dot);
}
