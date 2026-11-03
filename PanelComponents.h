// TB-3030 panel parts: knobs, rotary switches, silver keys, LEDs, slide switches, LCD
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace tbui
{
inline juce::Font font (float h, bool bold = true)
{
    return juce::Font (juce::FontOptions ("Arial", h, bold ? juce::Font::bold : juce::Font::plain));
}

namespace col
{
    const juce::Colour panelTop    { 0xffd2d2cc };
    const juce::Colour panelBottom { 0xffbdbdb7 };
    const juce::Colour ink         { 0xff1c1c1c };
    const juce::Colour red         { 0xffe8392a };
    const juce::Colour redText     { 0xffe0402d };
    const juce::Colour black       { 0xff141414 };
    const juce::Colour rail        { 0xff2a2a2b };
    const juce::Colour railText    { 0xffd9d9d4 };
}

//==============================================================================
class TBLookAndFeel : public juce::LookAndFeel_V4
{
public:
    TBLookAndFeel()
    {
        setColour (juce::TextButton::buttonColourId, juce::Colour (0xff4a4a4c));
        setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xffc23a2b));
        setColour (juce::TextButton::textColourOffId, juce::Colour (0xffe6e6e0));
        setColour (juce::TextButton::textColourOnId, juce::Colours::white);
        setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff2d2d2f));
        setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xffc23a2b));
        setColour (juce::BubbleComponent::backgroundColourId, juce::Colour (0xff1e1e1e));
        setColour (juce::BubbleComponent::outlineColourId, juce::Colour (0xff777777));
        setColour (juce::TooltipWindow::textColourId, juce::Colours::white);
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                           float startA, float endA, juce::Slider& s) override
    {
        const auto style = s.getProperties()["style"].toString();
        auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (2.0f);
        const float r = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
        const auto c = bounds.getCentre();
        const float angle = startA + pos * (endA - startA);

        if (style == "switch")
        {
            // big chrome rotary switch with a bar handle (PATT.GROUP / MODE)
            g.setColour (juce::Colour (0xff111111));
            g.fillEllipse (juce::Rectangle<float> (r * 2, r * 2).withCentre (c));
            juce::ColourGradient ring (juce::Colour (0xffe8e8e8), c.x - r, c.y - r, juce::Colour (0xff6a6a6a), c.x + r, c.y + r, false);
            g.setGradientFill (ring);
            g.fillEllipse (juce::Rectangle<float> (r * 1.78f, r * 1.78f).withCentre (c));
            g.setColour (juce::Colour (0xff2a2a2a));
            g.drawEllipse (juce::Rectangle<float> (r * 1.78f, r * 1.78f).withCentre (c), 1.5f);
            // bar handle
            juce::Path bar;
            bar.addRoundedRectangle (-r * 0.30f, -r * 0.86f, r * 0.60f, r * 1.72f, r * 0.12f);
            auto tr = juce::AffineTransform::rotation (angle).translated (c.x, c.y);
            g.setColour (juce::Colours::black.withAlpha (0.35f));
            g.fillPath (bar, tr.translated (3.0f, 4.0f));
            juce::ColourGradient chrome (juce::Colour (0xfffafafa), c.x - r * 0.4f, c.y - r * 0.6f,
                                         juce::Colour (0xff8d8d8d), c.x + r * 0.5f, c.y + r * 0.6f, false);
            chrome.addColour (0.45, juce::Colour (0xffcfcfcf));
            chrome.addColour (0.55, juce::Colour (0xffa8a8a8));
            g.setGradientFill (chrome);
            g.fillPath (bar, tr);
            g.setColour (juce::Colour (0xff3a3a3a));
            g.strokePath (bar, juce::PathStrokeType (1.4f), tr);
            // pointer groove
            juce::Path groove;
            groove.addRectangle (-1.2f, -r * 0.80f, 2.4f, r * 0.30f);
            g.setColour (juce::Colour (0xff222222));
            g.fillPath (groove, tr);
            return;
        }

        // ---- ticks
        const bool big = style == "big";
        const int ticks = 11;
        g.setColour (col::ink);
        for (int i = 0; i < ticks; ++i)
        {
            float a = startA + (endA - startA) * (float) i / (float) (ticks - 1);
            float r1 = r * 0.93f, r2 = r * ((i == 0 || i == ticks - 1 || i == ticks / 2) ? 1.0f : 0.98f);
            juce::Point<float> p1 (c.x + r1 * std::sin (a), c.y - r1 * std::cos (a));
            juce::Point<float> p2 (c.x + r2 * std::sin (a), c.y - r2 * std::cos (a));
            g.drawLine ({ p1, p2 }, big ? 2.2f : 1.8f);
        }

        // ---- knob body
        const float kr = r * 0.80f;
        g.setColour (juce::Colours::black.withAlpha (0.30f));
        g.fillEllipse (juce::Rectangle<float> (kr * 2, kr * 2).withCentre (c.translated (2.5f, 3.5f)));
        g.setColour (juce::Colour (0xff1a1a1a));
        g.fillEllipse (juce::Rectangle<float> (kr * 2, kr * 2).withCentre (c));
        // knurled skirt
        const float sr = kr * 0.90f;
        juce::ColourGradient skirt (juce::Colour (0xffe6e6e2), c.x - sr, c.y - sr, juce::Colour (0xff7d7d7a), c.x + sr, c.y + sr, false);
        g.setGradientFill (skirt);
        g.fillEllipse (juce::Rectangle<float> (sr * 2, sr * 2).withCentre (c));
        g.setColour (juce::Colour (0xff555552).withAlpha (0.55f));
        const int knurls = big ? 48 : 36;
        for (int i = 0; i < knurls; ++i)
        {
            float a = angle + juce::MathConstants<float>::twoPi * (float) i / (float) knurls;
            juce::Point<float> p1 (c.x + sr * 0.80f * std::sin (a), c.y - sr * 0.80f * std::cos (a));
            juce::Point<float> p2 (c.x + sr * 0.99f * std::sin (a), c.y - sr * 0.99f * std::cos (a));
            g.drawLine ({ p1, p2 }, 1.0f);
        }
        // cap
        const float cr = sr * 0.74f;
        juce::ColourGradient cap (juce::Colour (0xfff7f7f4), c.x - cr * 0.6f, c.y - cr * 0.8f, juce::Colour (0xffa9a9a5), c.x + cr * 0.6f, c.y + cr, false);
        g.setGradientFill (cap);
        g.fillEllipse (juce::Rectangle<float> (cr * 2, cr * 2).withCentre (c));
        g.setColour (juce::Colour (0xff8a8a86));
        g.drawEllipse (juce::Rectangle<float> (cr * 2, cr * 2).withCentre (c), 1.0f);
        // pointer
        juce::Path ptr;
        ptr.addRoundedRectangle (-2.2f, -sr * 0.98f, 4.4f, sr * 0.62f, 1.5f);
        g.setColour (juce::Colour (0xff202020));
        g.fillPath (ptr, juce::AffineTransform::rotation (angle).translated (c.x, c.y));
    }
};

//==============================================================================
class Led : public juce::Component
{
public:
    Led() { setInterceptsMouseClicks (false, false); }
    void setOn (bool b) { if (b != on) { on = b; repaint(); } }
    bool isOn() const { return on; }
    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        auto c = b.getCentre();
        const float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.28f;
        if (on)
        {
            juce::ColourGradient glow (col::red.withAlpha (0.65f), c.x, c.y, col::red.withAlpha (0.0f), c.x + r * 1.8f, c.y, true);
            g.setGradientFill (glow);
            g.fillEllipse (juce::Rectangle<float> (r * 3.6f, r * 3.6f).withCentre (c));
        }
        g.setColour (juce::Colour (0xff3a0d0a));
        g.fillEllipse (juce::Rectangle<float> (r * 2.3f, r * 2.3f).withCentre (c));
        juce::ColourGradient body (on ? juce::Colour (0xffff8a70) : juce::Colour (0xff8a2318), c.x - r * 0.4f, c.y - r * 0.5f,
                                   on ? juce::Colour (0xffe0251a) : juce::Colour (0xff4a0f0b), c.x + r, c.y + r, false);
        g.setGradientFill (body);
        g.fillEllipse (juce::Rectangle<float> (r * 2, r * 2).withCentre (c));
    }
private:
    bool on = false;
};

//==============================================================================
// Silver rectangular push key like the original panel
class PanelKey : public juce::Component
{
public:
    std::function<void (bool shift)> onPress;
    PanelKey() { setMouseClickGrabsKeyboardFocus (false); setRepaintsOnMouseActivity (true); }

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat().reduced (1.0f);
        g.setColour (juce::Colour (0xff2b2b2b));
        g.fillRoundedRectangle (b, 3.0f);
        auto face = b.reduced (2.5f).translated (0.0f, down ? 1.5f : 0.0f);
        juce::ColourGradient grad (juce::Colour (down ? 0xffbdbdb8 : 0xfff2f2ee), face.getX(), face.getY(),
                                   juce::Colour (down ? 0xff8f8f8a : 0xffa6a6a1), face.getX(), face.getBottom(), false);
        grad.addColour (0.5, juce::Colour (down ? 0xffacaca7 : 0xffd9d9d4));
        g.setGradientFill (grad);
        g.fillRoundedRectangle (face, 2.0f);
        g.setColour (juce::Colours::white.withAlpha (isMouseOver() ? 0.35f : 0.18f));
        g.drawLine (face.getX() + 2, face.getY() + 1.5f, face.getRight() - 2, face.getY() + 1.5f, 1.2f);
        g.setColour (juce::Colour (0xff6f6f6b));
        g.drawRoundedRectangle (face, 2.0f, 0.8f);
    }
    void mouseDown (const juce::MouseEvent& e) override
    {
        down = true; repaint();
        if (onPress) onPress (e.mods.isShiftDown() || e.mods.isCtrlDown());
    }
    void mouseUp (const juce::MouseEvent&) override { down = false; repaint(); }
private:
    bool down = false;
};

//==============================================================================
// small slide switch (WAVEFORM saw/square, CLOCK int/host)
class SlideSwitch : public juce::ToggleButton
{
public:
    SlideSwitch() { setMouseClickGrabsKeyboardFocus (false); }
    void paintButton (juce::Graphics& g, bool, bool) override
    {
        auto b = getLocalBounds().toFloat().reduced (1.0f);
        g.setColour (juce::Colour (0xff1b1b1b));
        g.fillRoundedRectangle (b, 3.0f);
        auto slot = b.reduced (3.0f);
        auto thumb = slot.withWidth (slot.getWidth() * 0.5f);
        if (getToggleState()) thumb.setX (slot.getCentreX());
        juce::ColourGradient grad (juce::Colour (0xfff0f0ec), thumb.getX(), thumb.getY(), juce::Colour (0xff9a9a96), thumb.getX(), thumb.getBottom(), false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (thumb, 2.0f);
        g.setColour (juce::Colour (0xff666662));
        for (int i = 1; i < 4; ++i)
        {
            float xx = thumb.getX() + thumb.getWidth() * (float) i / 4.0f;
            g.drawLine (xx, thumb.getY() + 3, xx, thumb.getBottom() - 3, 1.0f);
        }
    }
};

//==============================================================================
class Lcd : public juce::Component
{
public:
    Lcd() { setInterceptsMouseClicks (false, false); }
    void setLines (const juce::StringArray& l, const juce::Array<float>& sizes)
    {
        if (l == lines) return;
        lines = l; fontSizes = sizes; repaint();
    }
    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        g.setColour (juce::Colour (0xff111111));
        g.fillRoundedRectangle (b, 5.0f);
        auto screen = b.reduced (5.0f);
        juce::ColourGradient bg (juce::Colour (0xff3b1d12), screen.getX(), screen.getY(), juce::Colour (0xff1e0d08), screen.getX(), screen.getBottom(), false);
        g.setGradientFill (bg);
        g.fillRoundedRectangle (screen, 3.0f);
        auto area = screen.reduced (10.0f, 4.0f);
        float total = 0; for (int i = 0; i < lines.size(); ++i) total += fontSizes[i] + 4.0f;
        float yy = area.getY() + (area.getHeight() - total) * 0.5f;
        for (int i = 0; i < lines.size(); ++i)
        {
            g.setFont (juce::Font (juce::FontOptions ("Consolas", fontSizes[i], juce::Font::bold)));
            g.setColour (i == 1 ? juce::Colour (0xffffb347) : juce::Colour (0xffe8913a));
            g.drawText (lines[i], juce::Rectangle<float> (area.getX(), yy, area.getWidth(), fontSizes[i] + 4.0f),
                        juce::Justification::centredLeft, true);
            yy += fontSizes[i] + 4.0f;
        }
    }
private:
    juce::StringArray lines;
    juce::Array<float> fontSizes;
};

} // namespace tbui
