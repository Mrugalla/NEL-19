#pragma once
#include <JuceHeader.h>

struct Link : public juce::HyperlinkButton {
    Link(const juce::String& link = juce::String("https://www.google.de")) :
        juce::HyperlinkButton{link, juce::URL(link)}
    { setAlpha(0); }
};

class AboutComponent :
    public juce::Component {
public:
    AboutComponent(float upscale) :
        image(),
        upscaleFactor(upscale),

        discordLink("https://discord.gg/zEXr94u8yR"),
        githubLink("https://github.com/Mrugalla"),
        paypalLink("https://www.paypal.com/paypalme/alteoma/4.20"),
        juceLink("https://juce.com/"),
        joshsLink("https://discord.gg/G7EkyE3u"),
        pnsLink("https://www.bluecataudio.com/Products/Product_PlugNScript/")
    {
        addAndMakeVisible(discordLink);
        addAndMakeVisible(githubLink);
        addAndMakeVisible(paypalLink);
        addAndMakeVisible(juceLink);
        addAndMakeVisible(joshsLink);
        addAndMakeVisible(pnsLink);
        setOpaque(true);
    }

    void paint(juce::Graphics& g) override {
        g.setImageResamplingQuality(juce::Graphics::lowResamplingQuality);
        g.drawImageAt(image, 0, 0, false);
    }

    void mouseDown(const juce::MouseEvent&) override { setVisible(false); }

    void resized() override {
        auto aboutImage = juce::ImageCache::getFromMemory(BinaryData::about_png, BinaryData::about_pngSize);
        image = juce::Image(juce::Image::RGB, getWidth(), getHeight(), true);
        juce::Graphics g{ image };
        g.setImageResamplingQuality(juce::Graphics::lowResamplingQuality);
        g.drawImageWithin(aboutImage, getX(), getY(), getWidth(), getHeight(), juce::RectanglePlacement::fillDestination, false);

        initBounds(pnsLink, 2, 24, 76, 19);
        initBounds(joshsLink, 2, 45, 94, 17);
        initBounds(juceLink, 2, 64, 82, 15);
        initBounds(discordLink, 24, 98, 102, 7);
        initBounds(githubLink, 63, 107, 62, 7);
        initBounds(paypalLink, 62, 116, 63, 7);
    }

    void setCursor(const juce::MouseCursor& crs, const juce::MouseCursor& linkCursor) {
        setMouseCursor(crs);
        discordLink.setMouseCursor(linkCursor);
        githubLink.setMouseCursor(linkCursor);
        paypalLink.setMouseCursor(linkCursor);
        juceLink.setMouseCursor(linkCursor);
        joshsLink.setMouseCursor(linkCursor);
        pnsLink.setMouseCursor(linkCursor);
    }
private:
    juce::Image image;
    float upscaleFactor;
    Link discordLink, githubLink, paypalLink, juceLink, joshsLink, pnsLink;

    void initBounds(Link& button, int x, int y, int width, int height) {
        button.setBounds(
            (int)(x * upscaleFactor),
            (int)(y * upscaleFactor),
            (int)(width * upscaleFactor),
            (int)(height * upscaleFactor)
        );
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AboutComponent)
};
