#include "PluginEditor.h"

#include <cmath>

//==============================================================================
//  Formatage
//==============================================================================
namespace
{
    juce::String formatDuration (double seconds, bool padHours)
    {
        const auto total = (juce::int64) std::floor (std::abs (seconds));
        const auto h = total / 3600;
        const auto m = (total / 60) % 60;
        const auto s = total % 60;

        return (padHours ? juce::String (h).paddedLeft ('0', 2) : juce::String (h))
             + ":" + juce::String (m).paddedLeft ('0', 2)
             + ":" + juce::String (s).paddedLeft ('0', 2);
    }

    juce::String formatMinSec (double seconds)
    {
        const auto total = (int) std::floor (juce::jmax (0.0, seconds));
        return juce::String (total / 60) + ":" + juce::String (total % 60).paddedLeft ('0', 2);
    }

    juce::String formatDate (juce::int64 ms)
    {
        static const char* months[] = { "janv.", "févr.", "mars", "avr.", "mai", "juin",
                                        "juil.", "août", "sept.", "oct.", "nov.", "déc." };
        const juce::Time t (ms);
        return juce::String (t.getDayOfMonth()) + " " + fr (months[t.getMonth()]) + " " + juce::String (t.getYear());
    }

    juce::String formatClock (juce::int64 ms)
    {
        const juce::Time t (ms);
        return juce::String (t.getHours()).paddedLeft ('0', 2) + ":" + juce::String (t.getMinutes()).paddedLeft ('0', 2);
    }

    void setStyle (juce::Button& b, const char* style)
    {
        b.getProperties().set (TimeLookAndFeel::styleProperty, juce::String (style));
    }
}

//==============================================================================
//  NumberField
//==============================================================================
NumberField::NumberField (int stepIn, int minIn, int maxIn)
    : minus (fr ("−")), plus ("+"), step (stepIn), minValue (minIn), maxValue (maxIn)
{
    editor.setInputRestrictions (6, "0123456789");
    editor.setJustification (juce::Justification::centred);
    editor.setSelectAllWhenFocused (true);
    editor.onReturnKey = [this] { if (onReturn) onReturn(); };
    editor.onFocusLost = [this] { setValue (getValue()); };

    minus.onClick = [this] { setValue (getValue() - step); };
    plus .onClick = [this] { setValue (getValue() + step); };

    addAndMakeVisible (editor);
    addAndMakeVisible (minus);
    addAndMakeVisible (plus);
}

int NumberField::getValue() const
{
    return juce::jlimit (minValue, maxValue, editor.getText().getIntValue());
}

void NumberField::setValue (int v)
{
    editor.setText (juce::String (juce::jlimit (minValue, maxValue, v)), false);
}

void NumberField::setFontHeight (float h)
{
    editor.setFont (uiFont (h, FontWeight::Regular));
    editor.applyFontToAllText (uiFont (h, FontWeight::Regular));
}

void NumberField::resized()
{
    auto r = getLocalBounds();
    const int b = r.getHeight();
    const int gap = juce::jmax (3, b / 6);
    minus.setBounds (r.removeFromLeft (b));
    plus .setBounds (r.removeFromRight (b));
    editor.setBounds (r.reduced (gap, 0));
    editor.setIndents (4, juce::jmax (0, juce::roundToInt (((float) editor.getHeight() - editor.getFont().getHeight()) * 0.5f) - 2));
}

//==============================================================================
//  Sheet (panneau superposé)
//==============================================================================
Sheet::Sheet (TimeAudioProcessorEditor& o, TimeAudioProcessor& p, Mode m)
    : owner (o), processor (p), mode (m)
{
    setWantsKeyboardFocus (true);

    cancelButton.setButtonText ("Annuler");
    cancelButton.onClick = [this] { close(); };
    addAndMakeVisible (cancelButton);

    if (mode == Mode::Correct)
    {
        title   = "Corriger le temps";
        message = fr ("Minutes à ajouter ou à retirer du total. La correction apparaît dans l'historique.");
        labelA  = "minutes";
        fieldA  = std::make_unique<NumberField> (5, 0, 99999);
        fieldA->setValue (15);

        actionButton1.setButtonText (fr ("− Retirer"));
        actionButton2.setButtonText ("+ Ajouter");
        setStyle (actionButton2, "primary");

        actionButton1.onClick = [this] { processor.addAdjustment (-60.0 * fieldA->getValue()); close(); };
        actionButton2.onClick = [this] { processor.addAdjustment ( 60.0 * fieldA->getValue()); close(); };
        fieldA->onReturn = [this] { actionButton2.triggerClick(); };

        addAndMakeVisible (*fieldA);
        addAndMakeVisible (actionButton1);
        addAndMakeVisible (actionButton2);
    }
    else if (mode == Mode::Prior)
    {
        const auto prior = (int) std::round (processor.getSnapshot().priorSeconds / 60.0);

        title   = fr ("Temps antérieur");
        message = fr ("Temps déjà passé sur ce projet avant d'utiliser Time. Mettez 0 pour le supprimer.");
        labelA  = "heures";
        labelB  = "minutes";
        fieldA  = std::make_unique<NumberField> (1, 0, 99999);
        fieldB  = std::make_unique<NumberField> (5, 0, 59);
        fieldA->setValue (prior / 60);
        fieldB->setValue (prior % 60);

        actionButton2.setButtonText ("Enregistrer");
        setStyle (actionButton2, "primary");
        actionButton2.onClick = [this]
        {
            processor.setPriorSeconds (3600.0 * fieldA->getValue() + 60.0 * fieldB->getValue());
            close();
        };
        fieldA->onReturn = [this] { actionButton2.triggerClick(); };
        fieldB->onReturn = [this] { actionButton2.triggerClick(); };

        addAndMakeVisible (*fieldA);
        addAndMakeVisible (*fieldB);
        addAndMakeVisible (actionButton2);
    }
    else
    {
        title   = fr ("Tout remettre à zéro ?");
        message = fr ("Toutes les sessions, corrections et le temps antérieur seront effacés. Cette action est irréversible.");

        actionButton2.setButtonText ("Tout effacer");
        setStyle (actionButton2, "destructive");
        actionButton2.onClick = [this] { processor.resetAll(); close(); };
        addAndMakeVisible (actionButton2);
    }
}

void Sheet::close()
{
    owner.closeSheet();
}

void Sheet::focusFirstField()
{
    if (fieldA != nullptr)
        if (auto* ed = dynamic_cast<juce::TextEditor*> (fieldA->getChildComponent (0)))
            if (ed->isShowing())
                ed->grabKeyboardFocus();
}

void Sheet::setScale (float newScale)
{
    scale = newScale;

    if (fieldA != nullptr) fieldA->setFontHeight (14.0f * scale);
    if (fieldB != nullptr) fieldB->setFontHeight (14.0f * scale);

    resized();
    repaint();
}

juce::Rectangle<float> Sheet::getCardBounds() const
{
    const float w = juce::jmin (340.0f * scale, (float) getWidth()  - 16.0f);
    const float h = juce::jmin (196.0f * scale, (float) getHeight() - 16.0f);
    return getLocalBounds().toFloat().withSizeKeepingCentre (w, h);
}

void Sheet::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black.withAlpha (themeFor (*this).dark ? 0.45f : 0.16f));

    const auto card = getCardBounds();
    juce::Path p;
    p.addRoundedRectangle (card, 14.0f * scale);
    juce::DropShadow (juce::Colours::black.withAlpha (0.18f), juce::roundToInt (24 * scale), { 0, juce::roundToInt (6 * scale) })
        .drawForPath (g, p);
    g.setColour (themeFor (*this).card);
    g.fillPath (p);

    const float s = scale;
    auto inner = card.reduced (20.0f * s, 16.0f * s);

    g.setColour (themeFor (*this).text);
    g.setFont (uiFont (16.0f * s, FontWeight::Medium));
    g.drawText (title, inner.removeFromTop (24.0f * s), juce::Justification::centredLeft, true);

    g.setColour (themeFor (*this).secondary);
    g.setFont (uiFont (12.5f * s));
    g.drawFittedText (message, inner.removeFromTop (38.0f * s).toNearestInt(),
                      juce::Justification::topLeft, 3, 0.9f);

    // Légendes sous les champs
    g.setFont (uiFont (11.0f * s));
    if (fieldA != nullptr)
        g.drawText (labelA, fieldA->getBounds().toFloat().translated (0, (float) fieldA->getHeight() + 2.0f * s)
                                                .withHeight (14.0f * s), juce::Justification::centred);
    if (fieldB != nullptr)
        g.drawText (labelB, fieldB->getBounds().toFloat().translated (0, (float) fieldB->getHeight() + 2.0f * s)
                                                .withHeight (14.0f * s), juce::Justification::centred);
}

void Sheet::resized()
{
    const float s = scale;
    const auto card = getCardBounds();
    auto inner = card.reduced (20.0f * s, 16.0f * s);

    inner.removeFromTop (24.0f * s + 38.0f * s);

    // Rangée des boutons, en bas
    auto buttons = inner.removeFromBottom (30.0f * s);
    const float gap = 8.0f * s;

    if (mode == Mode::Correct)
    {
        const float w = (buttons.getWidth() - 2 * gap) / 3.0f;
        cancelButton .setBounds (buttons.removeFromLeft (w).toNearestInt()); buttons.removeFromLeft (gap);
        actionButton1.setBounds (buttons.removeFromLeft (w).toNearestInt()); buttons.removeFromLeft (gap);
        actionButton2.setBounds (buttons.toNearestInt());
    }
    else
    {
        const float w = (buttons.getWidth() - gap) / 2.0f;
        cancelButton .setBounds (buttons.removeFromLeft (w).toNearestInt()); buttons.removeFromLeft (gap);
        actionButton2.setBounds (buttons.toNearestInt());
    }

    // Champs de saisie, centrés
    auto fields = inner.withSizeKeepingCentre (inner.getWidth(), 30.0f * s).translated (0, -8.0f * s);
    const float fieldW = 136.0f * s;

    if (fieldA != nullptr && fieldB != nullptr)
    {
        auto both = fields.withSizeKeepingCentre (2 * fieldW + 20.0f * s, fields.getHeight());
        fieldA->setBounds (both.removeFromLeft (fieldW).toNearestInt());
        fieldB->setBounds (both.removeFromRight (fieldW).toNearestInt());
    }
    else if (fieldA != nullptr)
    {
        fieldA->setBounds (fields.withSizeKeepingCentre (fieldW, fields.getHeight()).toNearestInt());
    }
}

bool Sheet::keyPressed (const juce::KeyPress& key)
{
    if (key == juce::KeyPress::escapeKey)
    {
        close();
        return true;
    }

    return false;   // (Entrée ne déclenche jamais le reset : il faut cliquer)
}

void Sheet::mouseDown (const juce::MouseEvent& e)
{
    // Un clic en dehors de la carte ferme le panneau
    if (! getCardBounds().contains (e.position))
        close();
}

//==============================================================================
//  DisclosureButton
//==============================================================================
void DisclosureButton::paintButton (juce::Graphics& g, bool isMouseOver, bool)
{
    auto r = getLocalBounds().toFloat();
    const float h = r.getHeight();

    // Chevron dessiné (▸ / ▾)
    auto chevronArea = r.removeFromLeft (h * 0.7f).withSizeKeepingCentre (h * 0.28f, h * 0.28f);
    juce::Path chevron;

    if (expanded)
    {
        chevron.startNewSubPath (chevronArea.getX(), chevronArea.getY() + chevronArea.getHeight() * 0.25f);
        chevron.lineTo (chevronArea.getCentreX(), chevronArea.getBottom() - chevronArea.getHeight() * 0.15f);
        chevron.lineTo (chevronArea.getRight(), chevronArea.getY() + chevronArea.getHeight() * 0.25f);
    }
    else
    {
        chevron.startNewSubPath (chevronArea.getX() + chevronArea.getWidth() * 0.25f, chevronArea.getY());
        chevron.lineTo (chevronArea.getRight() - chevronArea.getWidth() * 0.15f, chevronArea.getCentreY());
        chevron.lineTo (chevronArea.getX() + chevronArea.getWidth() * 0.25f, chevronArea.getBottom());
    }

    const auto colour = isMouseOver ? themeFor (*this).accent.darker (0.15f) : themeFor (*this).accent;
    g.setColour (colour);
    g.strokePath (chevron, juce::PathStrokeType (juce::jmax (1.2f, h * 0.06f), juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));

    g.setFont (uiFont (h * 0.40f, FontWeight::Medium));
    g.drawText ("Sessions", r, juce::Justification::centredLeft);

    g.setColour (themeFor (*this).secondary);
    g.setFont (uiFont (h * 0.36f));
    g.drawText (countText, r, juce::Justification::centredRight);
}

//==============================================================================
//  ThemeToggle
//==============================================================================
void ThemeToggle::paintButton (juce::Graphics& g, bool isMouseOver, bool isButtonDown)
{
    const auto& T = themeFor (*this);
    auto r = getLocalBounds().toFloat().reduced (1.0f);

    g.setColour (isButtonDown ? T.controlHover.darker (0.05f) : (isMouseOver ? T.controlHover : T.control));
    g.fillEllipse (r);

    const auto c = r.getCentre();
    const float rad = r.getWidth() * 0.2f;
    g.setColour (isMouseOver ? T.accent : T.secondary);

    if (T.dark)
    {
        // Lune : disque "mordu" par un second disque de la couleur du fond
        g.fillEllipse (juce::Rectangle<float> (rad * 2.3f, rad * 2.3f).withCentre (c));
        g.setColour (isMouseOver ? T.controlHover : T.control);
        g.fillEllipse (juce::Rectangle<float> (rad * 2.1f, rad * 2.1f).withCentre (c + juce::Point<float> (rad * 0.75f, -rad * 0.6f)));
    }
    else
    {
        // Soleil : disque + 8 rayons
        g.fillEllipse (juce::Rectangle<float> (rad * 1.5f, rad * 1.5f).withCentre (c));
        juce::Path rays;
        for (int i = 0; i < 8; ++i)
        {
            const float a = (float) i * juce::MathConstants<float>::pi / 4.0f;
            const juce::Point<float> d (std::cos (a), std::sin (a));
            rays.startNewSubPath (c + d * rad * 1.25f);
            rays.lineTo (c + d * rad * 1.75f);
        }
        g.strokePath (rays, juce::PathStrokeType (juce::jmax (1.0f, rad * 0.28f), juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));
    }
}

//==============================================================================
//  Éditeur principal
//==============================================================================
TimeAudioProcessorEditor::TimeAudioProcessorEditor (TimeAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    darkMode = processor.isDarkMode();
    lookAndFeel.setDarkMode (darkMode);
    setLookAndFeel (&lookAndFeel);
    setOpaque (true);

    pauseButton  .setButtonText ("Pause");
    correctButton.setButtonText (fr ("Corriger ±"));
    priorButton  .setButtonText (fr ("Temps antérieur"));
    resetButton  .setButtonText ("Reset");

    pauseButton  .setTooltip (fr ("Geler le chrono, même en cas d'activité ou de lecture"));
    pauseButton  .onClick = [this] { processor.togglePause(); timerCallback(); };
    correctButton.onClick = [this] { openSheet (Sheet::Mode::Correct); };
    priorButton  .onClick = [this] { openSheet (Sheet::Mode::Prior); };
    resetButton  .onClick = [this] { openSheet (Sheet::Mode::Reset); };
    sessionsButton.onClick = [this] { setExpanded (! expanded); };

    for (auto* b : { &pauseButton, &correctButton, &priorButton, &resetButton })
        addAndMakeVisible (b);

    addAndMakeVisible (sessionsButton);

    themeButton.setTooltip ("Mode clair / sombre");
    themeButton.onClick = [this] { processor.setDarkMode (! processor.isDarkMode()); applyTheme(); };
    addAndMakeVisible (themeButton);

    historyList.setModel (this);
    historyList.setRowSelectedOnMouseDown (false);
    historyList.setMultipleSelectionEnabled (false);
    addChildComponent (historyList);

    snapshot = processor.getSnapshot();

    // Taille mémorisée dans le projet, redimensionnable par le coin
    setResizable (true, true);
    updateResizeLimits();
    const auto size = processor.getSavedEditorSize();
    setSize (size.x, size.y);

    startTimerHz (10);   // rafraîchissement ~10 fois par seconde
}

TimeAudioProcessorEditor::~TimeAudioProcessorEditor()
{
    stopTimer();
    sheet.reset();
    historyList.setModel (nullptr);
    setLookAndFeel (nullptr);
}

void TimeAudioProcessorEditor::updateResizeLimits()
{
    if (expanded)
        setResizeLimits (330, (int) std::ceil (210.0f * baseHeightExpanded / baseHeightCollapsed), 2400, 2400);
    else
        setResizeLimits (330, 210, 2400, 1600);
}

void TimeAudioProcessorEditor::applyTheme()
{
    darkMode = processor.isDarkMode();
    lookAndFeel.setDarkMode (darkMode);
    sendLookAndFeelChange();
    rebuildTextures();
    repaint();
}

void TimeAudioProcessorEditor::rebuildTextures()
{
    const auto& T = lookAndFeel.getTheme();
    const int w = juce::jmax (1, getWidth());
    const int h = juce::jmax (1, getHeight());

    // Rendu à la densité réelle de l'écran (Retina, Windows 150 %…) pour rester net
    textureScale = juce::jlimit (2.0f, 3.0f, juce::Component::getApproximateScaleFactorForComponent (this));
    const int pw = juce::roundToInt ((float) w * textureScale);
    const int ph = juce::roundToInt ((float) h * textureScale);

    backdropCache = juce::Image (juce::Image::RGB, pw, ph, true);
    {
        juce::Graphics g (backdropCache);
        g.addTransform (juce::AffineTransform::scale (textureScale));

        Texture::paintBackdrop (g, getLocalBounds().toFloat(), scale, T);

        // Ombres douces des cartes
        const juce::DropShadow shadow (juce::Colours::black.withAlpha (T.shadowAlpha),
                                       juce::roundToInt (22.0f * scale), { 0, juce::roundToInt (6.0f * scale) });
        juce::Path cards;
        cards.addRoundedRectangle (unit (16, 16, 408, 248), 14.0f * scale);
        if (expanded)
            cards.addRoundedRectangle (unit (16, 274, 408, 220), 14.0f * scale);
        shadow.drawForPath (g, cards);
    }

    grainCache = Texture::makeGrain (pw, ph, T);
}

void TimeAudioProcessorEditor::restoreSavedSize()
{
    const auto size = processor.getSavedEditorSize();
    const int h = expanded ? juce::roundToInt ((float) size.y * baseHeightExpanded / baseHeightCollapsed) : size.y;

    if (getWidth() != size.x || getHeight() != h)
        setSize (size.x, h);
}

void TimeAudioProcessorEditor::setExpanded (bool shouldExpand)
{
    if (shouldExpand == expanded)
        return;

    // La fenêtre s'agrandit vers le bas pour l'historique, puis reprend sa taille.
    const float ratio = shouldExpand ? baseHeightExpanded / baseHeightCollapsed
                                     : baseHeightCollapsed / baseHeightExpanded;
    const int newHeight = juce::roundToInt ((float) getHeight() * ratio);

    expanded = shouldExpand;
    sessionsButton.setExpanded (expanded);
    historyList.setVisible (expanded);

    applyingSize = true;
    updateResizeLimits();
    setSize (getWidth(), newHeight);
    applyingSize = false;

    resized();
    repaint();
}

juce::Rectangle<float> TimeAudioProcessorEditor::unit (float x, float y, float w, float h) const
{
    return { origin.x + x * scale, origin.y + y * scale, w * scale, h * scale };
}

void TimeAudioProcessorEditor::resized()
{
    const float w = (float) getWidth();
    const float h = (float) getHeight();
    const float baseH = expanded ? baseHeightExpanded : baseHeightCollapsed;

    scale  = juce::jmax (0.3f, juce::jmin (w / baseWidth, h / baseH));
    origin = { (w - baseWidth * scale) * 0.5f, (h - baseH * scale) * 0.5f };

    // Mémorise la taille (repliée) dans le projet
    if (! applyingSize)
    {
        const int collapsedH = expanded ? juce::roundToInt (h * baseHeightCollapsed / baseHeightExpanded) : (int) h;
        processor.setSavedEditorSize ((int) w, collapsedH);
    }

    // Rangée de boutons
    const float widths[] = { 78.0f, 84.0f, 120.0f, 70.0f };
    const float gap = 8.0f;
    float total = gap * 3;
    for (auto bw : widths) total += bw;

    float x = (baseWidth - total) * 0.5f;
    juce::TextButton* buttons[] = { &pauseButton, &correctButton, &priorButton, &resetButton };

    for (int i = 0; i < 4; ++i)
    {
        buttons[i]->setBounds (unit (x, 164.0f, widths[i], 30.0f).toNearestInt());
        x += widths[i] + gap;
    }

    sessionsButton.setBounds (unit (32.0f, 218.0f, 376.0f, 36.0f).toNearestInt());

    historyList.setBounds (unit (24.0f, 282.0f, 392.0f, 206.0f).toNearestInt());
    historyList.setRowHeight (juce::jmax (16, juce::roundToInt (44.0f * scale)));

    if (sheet != nullptr)
    {
        sheet->setBounds (getLocalBounds());
        sheet->setScale (scale);
    }

    themeButton.setBounds (unit (390.0f, 26.0f, 24.0f, 24.0f).toNearestInt());

    rebuildTextures();
}

void TimeAudioProcessorEditor::paint (juce::Graphics& g)
{
    const auto& T = themeFor (*this);
    const float s = scale;

    // Fond texturé (fissures, poussières, halo) + ombres
    g.drawImageTransformed (backdropCache, juce::AffineTransform::scale (1.0f / textureScale));

    auto drawCard = [&] (juce::Rectangle<float> r)
    {
        // Carte légèrement translucide : on devine les fissures du fond au travers
        g.setColour (T.card.withAlpha (T.dark ? 0.84f : 0.80f));
        g.fillRoundedRectangle (r, 14.0f * s);

        // Lumière venant du haut : léger dégradé + liseré
        juce::ColourGradient sheen (juce::Colours::white.withAlpha (T.dark ? 0.035f : 0.5f), r.getX(), r.getY(),
                                    juce::Colours::white.withAlpha (0.0f), r.getX(), r.getY() + r.getHeight() * 0.5f, false);
        g.setGradientFill (sheen);
        g.fillRoundedRectangle (r, 14.0f * s);

        g.setColour (T.cardEdge);
        g.drawRoundedRectangle (r.reduced (0.5f), 14.0f * s, 1.0f);
    };

    // Carte principale
    drawCard (unit (16, 16, 408, 248));

    // Légende
    g.setColour (themeFor (*this).secondary);
    g.setFont (uiFont (10.5f * s, FontWeight::Medium));
    g.drawText ("TEMPS DE TRAVAIL", unit (16, 30, 408, 16), juce::Justification::centred);

    paintChrono (g, unit (16, 48, 408, 72));
    paintStatus (g, unit (16, 124, 408, 24));

    // Séparateur au-dessus de "Sessions"
    g.setColour (themeFor (*this).separator);
    g.fillRect (unit (32, 212, 376, 1).withHeight (juce::jmax (1.0f, s * 0.75f)));

    // Carte de l'historique
    if (expanded)
        drawCard (unit (16, 274, 408, 220));
}

void TimeAudioProcessorEditor::paintOverChildren (juce::Graphics& g)
{
    // Grain de film très léger sur l'ensemble : texture sans bruit visuel
    g.drawImageTransformed (grainCache, juce::AffineTransform::scale (1.0f / textureScale));
}

void TimeAudioProcessorEditor::paintChrono (juce::Graphics& g, juce::Rectangle<float> area) const
{
    const auto text = formatDuration (snapshot.totalSeconds, true);
    const auto font = uiFont (area.getHeight() * 0.86f, FontWeight::Light);

    // Chiffres à chasse fixe : chaque caractère est centré dans une case de
    // largeur constante, quelle que soit la police -> rien ne bouge en défilant.
    float digitW = 0.0f;
    for (char c = '0'; c <= '9'; ++c)
        digitW = juce::jmax (digitW, juce::GlyphArrangement::getStringWidth (font, juce::String::charToString (c)));
    const float colonW = juce::GlyphArrangement::getStringWidth (font, ":") * 1.15f;

    float totalW = 0.0f;
    for (auto c : text)
        totalW += (c == ':') ? colonW : digitW;

    // Réduit si les heures deviennent très grandes
    float k = 1.0f;
    if (totalW > area.getWidth() * 0.94f)
        k = area.getWidth() * 0.94f / totalW;

    const auto drawFont = k < 1.0f ? uiFont (area.getHeight() * 0.86f * k, FontWeight::Light) : font;
    digitW *= k;
    const float cw = colonW * k;
    totalW *= k;

    g.setFont (drawFont);
    g.setColour (snapshot.paused ? themeFor (*this).secondary : themeFor (*this).text);

    float x = area.getCentreX() - totalW * 0.5f;
    for (auto c : text)
    {
        const float cellW = (c == ':') ? cw : digitW;
        const auto cell = juce::Rectangle<float> (x, area.getY(), cellW, area.getHeight());
        g.drawText (juce::String::charToString (c), cell,
                    juce::Justification::centred, false);
        x += cellW;
    }
}

void TimeAudioProcessorEditor::paintStatus (juce::Graphics& g, juce::Rectangle<float> area) const
{
    juce::Colour colour;
    juce::String label;

    switch (snapshot.state)
    {
        case LiveClock::State::Active:   colour = themeFor (*this).green;  label = "Actif"; break;
        case LiveClock::State::Playing:  colour = themeFor (*this).blue;   label = "Lecture"; break;
        case LiveClock::State::Waiting:  colour = themeFor (*this).orange;
            label = fr ("Inactif · ") + formatMinSec (snapshot.pendingSeconds) + fr (" en attente · retrait dans ")
                  + formatMinSec (snapshot.secondsBeforeDiscard);
            break;
        case LiveClock::State::Stopped:  colour = themeFor (*this).grey;   label = fr ("Arrêté · en attente d'activité"); break;
        case LiveClock::State::Paused:   colour = themeFor (*this).grey;   label = "En pause"; break;
    }

    const float s = scale;
    const auto font = uiFont (12.0f * s, FontWeight::Medium);
    const float textW = juce::GlyphArrangement::getStringWidth (font, label);
    const float dot = 7.0f * s;
    const float padX = 11.0f * s;
    const float pillW = juce::jmin (area.getWidth(), textW + dot + 7.0f * s + padX * 2.0f);
    const auto pill = area.withSizeKeepingCentre (pillW, area.getHeight());

    const auto& T = themeFor (*this);
    g.setColour (colour.withAlpha (T.dark ? 0.16f : 0.12f));
    g.fillRoundedRectangle (pill, pill.getHeight() * 0.5f);

    auto inner = pill.reduced (padX, 0.0f);
    const auto dotArea = inner.removeFromLeft (dot).withSizeKeepingCentre (dot, dot);

    // Point lumineux, avec un halo comme une braise
    if (snapshot.state != LiveClock::State::Paused && snapshot.state != LiveClock::State::Stopped)
    {
        const auto c = dotArea.getCentre();
        juce::ColourGradient halo (colour.withAlpha (0.45f), c.x, c.y, colour.withAlpha (0.0f), c.x + dot * 1.4f, c.y, true);
        g.setGradientFill (halo);
        g.fillEllipse (dotArea.expanded (dot * 0.9f));
    }

    g.setColour (colour);
    g.fillEllipse (dotArea);
    inner.removeFromLeft (7.0f * s);

    g.setColour (T.dark ? colour.brighter (0.25f) : colour.darker (0.35f));
    g.setFont (font);
    g.drawText (label, inner, juce::Justification::centredLeft, true);
}

void TimeAudioProcessorEditor::timerCallback()
{
    if (processor.consumeSizeRestoreRequest())
        restoreSavedSize();

    if (processor.isDarkMode() != darkMode)
        applyTheme();

    snapshot = processor.getSnapshot();

    pauseButton.setToggleState (snapshot.paused, juce::dontSendNotification);
    pauseButton.setButtonText (snapshot.paused ? "Reprendre" : "Pause");

    const int n = (int) snapshot.rows.size();
    sessionsButton.setCountText (n == 1 ? fr ("1 entrée") : juce::String (n) + fr (" entrées"));

    if (expanded)
    {
        historyList.updateContent();
        historyList.repaint();
    }

    repaint (unit (16, 16, 408, 140).toNearestInt());   // chrono + état
}

void TimeAudioProcessorEditor::openSheet (Sheet::Mode mode)
{
    sheet = std::make_unique<Sheet> (*this, processor, mode);
    addAndMakeVisible (*sheet);
    sheet->setBounds (getLocalBounds());
    sheet->setScale (scale);
    sheet->toFront (true);
    sheet->focusFirstField();
}

void TimeAudioProcessorEditor::closeSheet()
{
    if (sheet == nullptr)
        return;

    // Destruction différée : on est peut-être dans le clic d'un bouton du panneau.
    sheet->setVisible (false);
    juce::MessageManager::callAsync ([safe = juce::Component::SafePointer<TimeAudioProcessorEditor> (this)]
    {
        if (safe != nullptr)
        {
            safe->sheet.reset();
            safe->timerCallback();
        }
    });
}

//==============================================================================
//  Historique
//==============================================================================
int TimeAudioProcessorEditor::getNumRows()
{
    return (int) snapshot.rows.size();
}

void TimeAudioProcessorEditor::paintListBoxItem (int row, juce::Graphics& g, int width, int height, bool)
{
    if (! juce::isPositiveAndBelow (row, (int) snapshot.rows.size()))
        return;

    const auto& item = snapshot.rows[(size_t) row];
    const float s = scale;
    auto r = juce::Rectangle<float> (0, 0, (float) width, (float) height).reduced (10.0f * s, 0);

    juce::String titleText, detail, duration;
    juce::Colour durationColour = themeFor (*this).text;

    switch (item.kind)
    {
        case HistoryRow::Kind::CurrentSession:
            titleText = "Session en cours";
            detail    = formatDate (item.startMs) + fr (" · ") + formatClock (item.startMs) + fr (" → maintenant");
            duration  = formatDuration (item.seconds, false);
            durationColour = themeFor (*this).accent;
            break;

        case HistoryRow::Kind::Session:
            titleText = "Session";
            detail    = formatDate (item.startMs) + fr (" · ") + formatClock (item.startMs) + fr (" → ") + formatClock (item.endMs);
            duration  = formatDuration (item.seconds, false);
            break;

        case HistoryRow::Kind::Adjustment:
            titleText = "Ajustement";
            detail    = formatDate (item.startMs) + fr (" · ") + formatClock (item.startMs);
            duration  = (item.seconds < 0 ? fr ("− ") : juce::String ("+ ")) + formatDuration (item.seconds, false);
            durationColour = themeFor (*this).secondary;
            break;

        case HistoryRow::Kind::Prior:
            titleText = fr ("Temps antérieur");
            detail    = fr ("Avant Time · cliquer pour modifier");
            duration  = formatDuration (item.seconds, false);
            break;
    }

    auto right = r.removeFromRight (110.0f * s);
    g.setColour (durationColour);
    g.setFont (uiFont (14.0f * s, FontWeight::Medium));
    g.drawText (duration, right, juce::Justification::centredRight, false);

    auto top = r.withTrimmedBottom (r.getHeight() * 0.5f);
    auto bottom = r.withTrimmedTop (r.getHeight() * 0.5f);

    g.setColour (themeFor (*this).text);
    g.setFont (uiFont (13.0f * s, FontWeight::Medium));
    g.drawText (titleText, top.withTrimmedTop (2.0f * s), juce::Justification::bottomLeft, true);

    g.setColour (themeFor (*this).secondary);
    g.setFont (uiFont (11.0f * s));
    g.drawText (detail, bottom.withTrimmedBottom (2.0f * s).withTrimmedTop (2.0f * s), juce::Justification::topLeft, true);

    if (row < getNumRows() - 1)
    {
        g.setColour (themeFor (*this).separator);
        g.fillRect (juce::Rectangle<float> (10.0f * s, (float) height - 1.0f, (float) width - 20.0f * s, 1.0f));
    }
}

void TimeAudioProcessorEditor::listBoxItemClicked (int row, const juce::MouseEvent&)
{
    if (juce::isPositiveAndBelow (row, (int) snapshot.rows.size())
         && snapshot.rows[(size_t) row].kind == HistoryRow::Kind::Prior)
        openSheet (Sheet::Mode::Prior);
}
