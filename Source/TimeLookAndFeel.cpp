#include "TimeLookAndFeel.h"

#include <cmath>
#include <vector>

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

const Theme& Theme::light()
{
    static const Theme t = []
    {
        Theme x;
        x.dark            = false;
        x.background      = juce::Colour (0xffefedea);   // blanc cassé chaud
        x.backgroundLight = juce::Colour (0xfffbfaf8);   // halo de lumière
        x.card            = juce::Colour (0xfffaf9f7);
        x.cardEdge        = juce::Colour (0x0f000000);
        x.text            = juce::Colour (0xff1f1c1a);
        x.secondary       = juce::Colour (0xff8c8681);
        x.separator       = juce::Colour (0xffe7e3de);
        x.control         = juce::Colour (0xffefece8);
        x.controlHover    = juce::Colour (0xffe6e2dd);
        x.accent          = juce::Colour (0xffe4572e);   // orange braise
        x.green           = juce::Colour (0xff34c759);
        x.blue            = juce::Colour (0xff2f80ed);
        x.orange          = juce::Colour (0xffeea12b);   // ambre (attente)
        x.grey            = juce::Colour (0xff8e8a86);
        x.red             = juce::Colour (0xffd93a2b);
        x.crack           = juce::Colour (0xff2a2420);
        x.dust            = juce::Colour (0xff3a322c);
        x.shadowAlpha     = 0.08f;
        x.grainAlpha      = 0.022f;
        return x;
    }();
    return t;
}

const Theme& Theme::darkTheme()
{
    static const Theme t = []
    {
        Theme x;
        x.dark            = true;
        x.background      = juce::Colour (0xff17161a);   // gris-noir
        x.backgroundLight = juce::Colour (0xff2b2827);   // halo chaud discret
        x.card            = juce::Colour (0xff232225);
        x.cardEdge        = juce::Colour (0x14ffffff);
        x.text            = juce::Colour (0xfff2efeb);
        x.secondary       = juce::Colour (0xff8f8a86);
        x.separator       = juce::Colour (0xff333136);
        x.control         = juce::Colour (0xff2f2d31);
        x.controlHover    = juce::Colour (0xff3a3740);
        x.accent          = juce::Colour (0xffff6a3d);   // orange braise
        x.green           = juce::Colour (0xff3ad46b);
        x.blue            = juce::Colour (0xff4c95ff);
        x.orange          = juce::Colour (0xfff2ae3e);
        x.grey            = juce::Colour (0xff8a8690);
        x.red             = juce::Colour (0xffff5a4a);
        x.crack           = juce::Colour (0xff000000);
        x.dust            = juce::Colour (0xffd8cfc7);
        x.shadowAlpha     = 0.45f;
        x.grainAlpha      = 0.028f;
        return x;
    }();
    return t;
}

const Theme& themeFor (const juce::Component& c)
{
    if (auto* laf = dynamic_cast<TimeLookAndFeel*> (&c.getLookAndFeel()))
        return laf->getTheme();

    return Theme::light();
}

TimeLookAndFeel::TimeLookAndFeel()
{
    setDarkMode (false);
}

void TimeLookAndFeel::setDarkMode (bool shouldBeDark)
{
    theme = shouldBeDark ? &Theme::darkTheme() : &Theme::light();
    const auto& T = *theme;

    setColour (juce::TextButton::textColourOffId,           T.accent);
    setColour (juce::TextButton::textColourOnId,            juce::Colours::white);
    setColour (juce::TextEditor::textColourId,              T.text);
    setColour (juce::TextEditor::highlightColourId,         T.accent.withAlpha (0.25f));
    setColour (juce::TextEditor::highlightedTextColourId,   T.text);
    setColour (juce::CaretComponent::caretColourId,         T.accent);
    setColour (juce::ListBox::backgroundColourId,           juce::Colours::transparentBlack);
    setColour (juce::ListBox::outlineColourId,              juce::Colours::transparentBlack);
    setColour (juce::ScrollBar::thumbColourId,              T.grey.withAlpha (0.45f));
}

void TimeLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour&,
                                            bool isMouseOver, bool isButtonDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);
    const float radius = bounds.getHeight() * 0.5f;     // forme "pilule"
    const auto  style  = button.getProperties()[styleProperty].toString();

    juce::Colour fill = theme->control;

    if (style == "primary" || button.getToggleState())
        fill = theme->accent;
    else if (style == "destructive")
        fill = theme->red;
    else if (isMouseOver)
        fill = theme->controlHover;

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

    g.setColour (filled ? juce::Colours::white : theme->accent);
    g.setFont (getTextButtonFont (button, button.getHeight()));

    auto area = button.getLocalBounds().reduced (juce::jmax (2, button.getHeight() / 4), 0);
    g.drawFittedText (button.getButtonText(), area, juce::Justification::centred, 1, 0.8f);
}

void TimeLookAndFeel::fillTextEditorBackground (juce::Graphics& g, int width, int height, juce::TextEditor&)
{
    g.setColour (theme->control);
    g.fillRoundedRectangle (juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height),
                            (float) height * 0.25f);
}

void TimeLookAndFeel::drawTextEditorOutline (juce::Graphics& g, int width, int height, juce::TextEditor& editor)
{
    if (editor.hasKeyboardFocus (true))
    {
        g.setColour (theme->accent.withAlpha (0.6f));
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
    g.setColour (theme->grey.withAlpha (alpha));
    g.fillRoundedRectangle (thumb, juce::jmin (thumb.getWidth(), thumb.getHeight()) * 0.5f);
}

void TimeLookAndFeel::drawCornerResizer (juce::Graphics& g, int w, int h, bool isMouseOver, bool isMouseDragging)
{
    // Coin de redimensionnement discret : trois points en diagonale
    const float size = (float) juce::jmin (w, h);
    const float dot  = juce::jmax (2.0f, size * 0.14f);
    g.setColour (theme->grey.withAlpha (isMouseDragging ? 0.8f : (isMouseOver ? 0.6f : 0.35f)));

    for (int i = 0; i < 3; ++i)
        for (int j = 0; j <= i; ++j)
            g.fillEllipse ((float) w - (float) (j + 1) * dot * 1.9f,
                           (float) h - (float) (i - j + 1) * dot * 1.9f, dot, dot);
}

//==============================================================================
//  Textures
//==============================================================================
namespace
{
    // Une fissure : ligne brisée qui part d'un point, avec quelques ramifications
    void addCrack (juce::Path& path, juce::Random& rng, juce::Point<float> p, float angle,
                   float segment, int segments, int depth, std::vector<juce::Point<float>>& joints)
    {
        path.startNewSubPath (p);

        for (int i = 0; i < segments; ++i)
        {
            angle += (rng.nextFloat() - 0.5f) * 0.9f;
            const float len = segment * (0.6f + rng.nextFloat() * 0.8f);
            p += { std::cos (angle) * len, std::sin (angle) * len };
            path.lineTo (p);

            if (depth > 0 && rng.nextFloat() < 0.35f)
            {
                joints.push_back (p);
                juce::Path branch;
                addCrack (branch, rng, p, angle + (rng.nextBool() ? 1.0f : -1.0f) * (0.7f + rng.nextFloat() * 0.6f),
                          segment * 0.7f, 2 + rng.nextInt (3), depth - 1, joints);
                path.addPath (branch);
                path.startNewSubPath (p);
            }
        }
    }
}

void Texture::paintBackdrop (juce::Graphics& g, juce::Rectangle<float> area, float scale, const Theme& T)
{
    // 1. Couleur de fond + halo de lumière diffuse (en haut à droite)
    g.setColour (T.background);
    g.fillRect (area);

    juce::ColourGradient glow (T.backgroundLight, area.getRight() - area.getWidth() * 0.15f, area.getY() + area.getHeight() * 0.1f,
                               T.backgroundLight.withAlpha (0.0f), area.getX() + area.getWidth() * 0.25f, area.getBottom(), true);
    g.setGradientFill (glow);
    g.fillRect (area);

    // Ombre douce opposée (en bas à gauche), comme une lumière rasante
    juce::ColourGradient shade (T.crack.withAlpha (T.dark ? 0.35f : 0.06f), area.getX(), area.getBottom(),
                                T.crack.withAlpha (0.0f), area.getCentreX(), area.getCentreY(), true);
    g.setGradientFill (shade);
    g.fillRect (area);

    juce::Random rng (20241114);   // graine fixe : la texture est toujours la même

    // 2. Fissures fines dans les coins, avec une lueur braise discrète aux jonctions
    const float seg = 26.0f * scale;
    std::vector<juce::Point<float>> joints;
    juce::Path cracks;

    const float W = area.getWidth(), H = area.getHeight();
    addCrack (cracks, rng, { area.getX() - 4.0f, area.getY() + H * 0.70f }, -0.30f, seg, 9, 2, joints);
    addCrack (cracks, rng, { area.getRight() + 4.0f, area.getY() + H * 0.22f }, 3.14159f + 0.35f, seg, 9, 2, joints);
    addCrack (cracks, rng, { area.getX() + W * 0.62f, area.getBottom() + 4.0f }, -1.9f, seg, 6, 1, joints);

    // Lueur braise le long des fissures (comme de la lumière qui passe à travers)
    for (float width : { 9.0f, 4.0f })
    {
        g.setColour (T.accent.withAlpha ((T.dark ? 0.10f : 0.05f) * (width < 5.0f ? 2.0f : 1.0f)));
        g.strokePath (cracks, juce::PathStrokeType (width * scale, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    g.setColour (T.dark ? T.accent.withAlpha (0.55f) : T.crack.withAlpha (0.22f));
    g.strokePath (cracks, juce::PathStrokeType (juce::jmax (0.7f, 1.0f * scale)));

    // Liseré clair à côté de la fissure (effet de relief)
    g.setColour (juce::Colours::white.withAlpha (T.dark ? 0.05f : 0.8f));
    g.strokePath (cracks, juce::PathStrokeType (juce::jmax (0.5f, 0.6f * scale)),
                  juce::AffineTransform::translation (0.0f, juce::jmax (0.6f, 1.0f * scale)));

    for (auto j : joints)
    {
        const float r = (4.0f + rng.nextFloat() * 6.0f) * scale;
        juce::ColourGradient ember (T.accent.withAlpha (T.dark ? 0.35f : 0.18f), j.x, j.y,
                                    T.accent.withAlpha (0.0f), j.x + r, j.y, true);
        g.setGradientFill (ember);
        g.fillEllipse (juce::Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (j));
    }

    // 3. Poussières / particules, plus denses près des fissures
    const int count = juce::roundToInt (area.getWidth() * area.getHeight() / (900.0f * juce::jmax (0.5f, scale)));

    for (int i = 0; i < count; ++i)
    {
        juce::Point<float> p;

        if (! joints.empty() && rng.nextFloat() < 0.55f)
        {
            const auto j = joints[(size_t) rng.nextInt ((int) joints.size())];
            const float a = rng.nextFloat() * 6.2832f;
            const float d = std::pow (rng.nextFloat(), 1.6f) * 70.0f * scale;
            p = j + juce::Point<float> (std::cos (a) * d, std::sin (a) * d);
        }
        else
        {
            p = { area.getX() + rng.nextFloat() * area.getWidth(), area.getY() + rng.nextFloat() * area.getHeight() };
        }

        const bool bokeh = rng.nextFloat() < 0.06f;
        const float size = (bokeh ? 3.0f + rng.nextFloat() * 5.0f : 0.5f + rng.nextFloat() * 1.4f) * scale;
        const bool ember = rng.nextFloat() < 0.12f;
        auto c = ember ? T.accent : T.dust;
        float alpha = bokeh ? 0.05f + rng.nextFloat() * 0.06f
                            : (T.dark ? 0.10f : 0.08f) + rng.nextFloat() * (ember ? 0.35f : 0.18f);

        g.setColour (c.withAlpha (alpha));
        g.fillEllipse (juce::Rectangle<float> (size, size).withCentre (p));
    }
}

juce::Image Texture::makeGrain (int width, int height, const Theme& T)
{
    juce::Image img (juce::Image::ARGB, juce::jmax (1, width), juce::jmax (1, height), true);
    juce::Random rng (777);
    juce::Image::BitmapData data (img, juce::Image::BitmapData::writeOnly);

    const auto base = T.dark ? juce::Colours::white : juce::Colours::black;

    for (int y = 0; y < data.height; ++y)
        for (int x = 0; x < data.width; ++x)
        {
            const float n = rng.nextFloat();
            data.setPixelColour (x, y, base.withAlpha (n * T.grainAlpha));
        }

    return img;
}

//==============================================================================
//  Logo chronomètre
//==============================================================================
void drawStopwatchLogo (juce::Graphics& g, juce::Rectangle<float> area, const Theme& T)
{
    const float u = juce::jmin (area.getWidth(), area.getHeight()) / 24.0f;   // grille 24 x 24
    const auto  o = area.withSizeKeepingCentre (24.0f * u, 24.0f * u).getTopLeft();
    auto P = [&] (float x, float y) { return juce::Point<float> (o.x + x * u, o.y + y * u); };

    const auto centre = P (12.0f, 13.5f);
    const float radius = 8.2f * u;
    const float stroke = 1.7f * u;
    const auto  body   = T.text.withAlpha (0.88f);

    // Halo braise discret derrière le cadran
    juce::ColourGradient halo (T.accent.withAlpha (T.dark ? 0.30f : 0.16f), centre.x, centre.y,
                               T.accent.withAlpha (0.0f), centre.x + radius * 1.6f, centre.y, true);
    g.setGradientFill (halo);
    g.fillEllipse (juce::Rectangle<float> (radius * 3.2f, radius * 3.2f).withCentre (centre));

    // Boîtier
    g.setColour (body);
    g.drawEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (centre), stroke);

    // Couronne (bouton du haut) + tige
    g.fillRoundedRectangle (juce::Rectangle<float> (P (9.6f, 1.6f), P (14.4f, 3.4f)), 0.8f * u);
    g.fillRect (juce::Rectangle<float> (P (11.2f, 3.0f), P (12.8f, 5.6f)));

    // Bouton latéral (en haut à droite)
    {
        juce::Path side;
        side.addRoundedRectangle (-1.3f * u, -0.8f * u, 2.6f * u, 1.6f * u, 0.5f * u);
        const float a = juce::degreesToRadians (45.0f);
        const auto  p = centre + juce::Point<float> (std::sin (a), -std::cos (a)) * (radius + 1.6f * u);
        g.fillPath (side, juce::AffineTransform::rotation (a).translated (p));
    }

    // Graduations : 12, 3, 6, 9 h
    for (int i = 0; i < 4; ++i)
    {
        const float a = (float) i * juce::MathConstants<float>::halfPi;
        const juce::Point<float> d (std::sin (a), -std::cos (a));
        juce::Path tick;
        tick.startNewSubPath (centre + d * (radius - 1.6f * u));
        tick.lineTo (centre + d * (radius - 3.0f * u));
        g.setColour (body.withMultipliedAlpha (0.6f));
        g.strokePath (tick, juce::PathStrokeType (1.1f * u, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // Aiguille (couleur d'accent), pointée vers ~ 2 h
    {
        const float a = juce::degreesToRadians (60.0f);
        const juce::Point<float> d (std::sin (a), -std::cos (a));
        juce::Path hand;
        hand.startNewSubPath (centre);
        hand.lineTo (centre + d * (radius - 2.4f * u));
        g.setColour (T.accent);
        g.strokePath (hand, juce::PathStrokeType (1.7f * u, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.fillEllipse (juce::Rectangle<float> (2.8f * u, 2.8f * u).withCentre (centre));
    }
}
