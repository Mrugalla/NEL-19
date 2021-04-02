#pragma once
#include <JuceHeader.h>
#include <functional>

struct Settings :
    public NELComp
{
    struct ButtonImg :
        public NELComp {
        enum class State { Norm, Hover };

        ButtonImg(const juce::Image& image, Nel19AudioProcessor& p, NELGUtil& u, juce::URL url, juce::String tooltp = "") :
            NELComp(p, u, tooltp),
            onClick([url]() { url.launchInDefaultBrowser(); }),
            img(image.createCopy()),
            state(State::Norm)
        {
            setMouseCursor(u.cursors[NELGUtil::Cursor::Hover]);
        }
    private:
        std::function<void()> onClick;
        juce::Image img;
        State state;

        void paint(juce::Graphics& g) override {
#if DebugLayout
            g.setColour(juce::Colours::red);
            g.drawRect(getLocalBounds());
#endif
            g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
            const auto bounds = getLocalBounds().toFloat().reduced(nelG::Thicc);
            g.setColour(juce::Colour(nelG::ColBlack));
            g.fillRoundedRectangle(bounds, nelG::Rounded);
            if (state == State::Hover) {
                g.setColour(juce::Colour(nelG::ColYellow));
                g.drawRoundedRectangle(bounds, nelG::Rounded, nelG::Thicc);
            }
            g.drawImage(img, bounds, juce::RectanglePlacement::Flags::centred, false);
        }

        void mouseEnter(const juce::MouseEvent&) override { state = State::Hover; repaint(); }
        void mouseExit(const juce::MouseEvent&) override { state = State::Norm; repaint(); }
        void mouseUp(const juce::MouseEvent& evt) override {
            if (evt.mouseWasDraggedSinceMouseDown()) return;
            onClick();
        }
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ButtonImg)
    };

    struct Button :
        public NELComp {
        enum class State { Norm, Hover };

        Button(juce::String nme, Nel19AudioProcessor& p, NELGUtil& u, std::function<void()> clck = []() {}, juce::String tooltp = "") :
            NELComp(p, u, tooltp),
            onClick(clck),
            name(nme),
            state(State::Norm)
        {
            setMouseCursor(u.cursors[NELGUtil::Cursor::Hover]);
        }
        Button(juce::String nme, Nel19AudioProcessor& p, NELGUtil& u, juce::URL url, juce::String tooltp = "") :
            NELComp(p, u, tooltp),
            onClick([url]() { url.launchInDefaultBrowser(); }),
            name(nme),
            state(State::Norm)
        {
            setMouseCursor(u.cursors[NELGUtil::Cursor::Hover]);
        }
        void rename(juce::String nme) {
            name = nme;
            repaint();
        }
    private:
        std::function<void()> onClick;
        juce::String name;
        State state;

        void paint(juce::Graphics& g) override {
            const auto bounds = getLocalBounds().toFloat().reduced(nelG::Thicc);
            g.setColour(juce::Colour(nelG::ColBlack));
            g.fillRoundedRectangle(bounds, nelG::Rounded);
            if (state == State::Norm)
                g.setColour(juce::Colour(nelG::ColGreen));
            else
                g.setColour(juce::Colour(nelG::ColYellow));
            g.drawRoundedRectangle(bounds, nelG::Rounded, nelG::Thicc);
            const auto numLines = name.length();
            g.drawFittedText(name, bounds.toNearestInt(), juce::Justification::centred, numLines, 0);
        }

        void mouseEnter(const juce::MouseEvent&) override { state = State::Hover; repaint(); }
        void mouseExit(const juce::MouseEvent&) override { state = State::Norm; repaint(); }
        void mouseUp(const juce::MouseEvent& evt) override {
            if (evt.mouseWasDraggedSinceMouseDown()) return;
            onClick();
        }
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Button)
    };

    struct PageHelp :
        public NELComp
    {
        PageHelp(Settings& s, Nel19AudioProcessor& p, NELGUtil& u, juce::String tooltp = "") :
            NELComp(p, u, tooltp),
            manualB("Manual", p, u, juce::URL("https://github.com/Mrugalla/NEL-19"), "My Github page also acts as the manual."),
            tutorialsB("Tutorials", p, u, juce::URL("https://www.youtube.com/channel/UCBTAQ7ro8z1f8jufCDGogTw"), "Videos about NEL and my devlog on YouTube."),
            discordB("Bugs- &\nFeature Requests", p, u, juce::URL("https://discord.gg/xpTGJJNAZG"), "Join my Discord to discuss things."),
            tooltipsB(utils.tooltipEnabled ? "Tooltips\nEnabled" : "Tooltips\nDisabled", p, u, [this]() { tooltipsFunc(); }, "If you can read this tooltips are currently enabled.. or you encountered a bug.")
        {
            setInterceptsMouseClicks(false, true);
            addAndMakeVisible(manualB);
            addAndMakeVisible(tutorialsB);
            addAndMakeVisible(tooltipsB);
            addAndMakeVisible(discordB);
        }
    private:
        Button manualB, tutorialsB, discordB, tooltipsB;
        void resized() override {
            const auto bounds = getLocalBounds().toFloat().reduced(nelG::Thicc);
            const auto numTokyoButtons = 4.f;
            auto x = bounds.getX();
            const auto y = bounds.getY();
            const auto width = bounds.getWidth() / numTokyoButtons;
            const auto height = bounds.getHeight();
            const auto minDimen = std::min(width, height);
            const auto radius = minDimen * .5f;
            const auto margin = radius * .4f;
            manualB.setBounds(juce::Rectangle<float>(x, y, width, height).reduced(margin).toNearestInt());
            x += width;
            tutorialsB.setBounds(juce::Rectangle<float>(x, y, width, height).reduced(margin).toNearestInt());
            x += width;
            discordB.setBounds(juce::Rectangle<float>(x, y, width, height).reduced(margin).toNearestInt());
            x += width;
            tooltipsB.setBounds(juce::Rectangle<float>(x, y, width, height).reduced(margin).toNearestInt());
        }

        void tooltipsFunc() {
            utils.switchToolTip(processor);
            if (utils.tooltipEnabled) tooltipsB.rename("Tooltips\nEnabled");
            else tooltipsB.rename("Tooltips\nDisabled");
        }

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PageHelp)
    };

    struct PageThanks :
        public NELComp
    {
        PageThanks(Settings& s, Nel19AudioProcessor& p, NELGUtil& u) :
            NELComp(p, u),
            pnsB(nelG::load(BinaryData::bluecat_png, BinaryData::bluecat_pngSize), p, u, juce::URL(""), "My DSP Journey would not have started without Plug'n Script. Thank you!"),
            juceB(nelG::load(BinaryData::juce_png, BinaryData::juce_pngSize), p, u, juce::URL(""), "Thank you for maintaining and improving this awesome framework!"),
            joshB(nelG::load(BinaryData::tap_png, BinaryData::tap_pngSize), p, u, juce::URL(""), "Where would I be now without your community? Thank you!")
        {
            setInterceptsMouseClicks(false, true);
            addAndMakeVisible(pnsB);
            addAndMakeVisible(juceB);
            addAndMakeVisible(joshB);
        }
    private:
        juce::Rectangle<float> titleBounds, thanksAllBounds, lionelBounds;
        ButtonImg pnsB, juceB, joshB;
        void paint(juce::Graphics& g) override {
            g.setColour(juce::Colour(nelG::ColBlack).withAlpha(.5f));
            g.fillRoundedRectangle(titleBounds, nelG::Rounded);
            g.fillRoundedRectangle(thanksAllBounds, nelG::Rounded);
            g.fillRoundedRectangle(lionelBounds, nelG::Rounded);
            
            g.setColour(juce::Colour(nelG::ColGreen));
            g.drawFittedText("Thank You", titleBounds.toNearestInt(), juce::Justification::centred, 1);
            juce::String txt;
            txt += "Special thanks go out to my fiancée, Alina, my cat, Helmi and my son, Lionel.\n";
            txt += "You give me the energy to push myself and work hard every day.\n";
            txt += "\n";
            txt += "I love you! <3\n";
            txt += "\n\n";
            txt += "Also thanks to all the other people who were part of my journey so far.\n";
            txt += "If you're not mentioned here explicitely I just didn't want\n";
            txt += "to implement a scrollbar yet! :)";
            g.drawFittedText(txt, lionelBounds.toNearestInt(), juce::Justification::left, 420, 0);
        }
        void resized() override {
            const auto bounds = getLocalBounds().toFloat().reduced(nelG::Thicc);
            const auto titleHeight = bounds.getHeight() * .2f;
            titleBounds = juce::Rectangle<float>(
                bounds.getX(),
                bounds.getY(),
                bounds.getWidth(),
                titleHeight
                ).reduced(nelG::Thicc);
            const auto thanksAllWidth = bounds.getWidth() * .25f;
            const auto thanksHeight = bounds.getHeight() - titleHeight;
            thanksAllBounds = juce::Rectangle<float>(
                bounds.getX(),
                titleBounds.getBottom(),
                thanksAllWidth,
                thanksHeight
            ).reduced(nelG::Thicc);
            lionelBounds = juce::Rectangle<float>(
                bounds.getX() + thanksAllWidth,
                titleBounds.getBottom(),
                bounds.getWidth() - thanksAllWidth,
                thanksHeight
            ).reduced(nelG::Thicc);

            const auto numButtons = 3.f;
            const auto x = thanksAllBounds.getX();
            auto y = thanksAllBounds.getY();
            const auto width = thanksAllBounds.getWidth();
            const auto height = thanksAllBounds.getHeight() / numButtons;
            const auto minDimen = std::min(width, height);
            const auto radius = minDimen * .5f;
            const auto margin = radius * .1f;
            pnsB.setBounds(juce::Rectangle<float>(x, y, width, height).reduced(margin).toNearestInt());
            y += height;
            juceB.setBounds(juce::Rectangle<float>(x, y, width, height).reduced(margin).toNearestInt());
            y += height;
            joshB.setBounds(juce::Rectangle<float>(x, y, width, height).reduced(margin).toNearestInt());
        }

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PageThanks)
    };

    struct PageAboutMe :
        public NELComp
    {
        PageAboutMe(Settings& s, Nel19AudioProcessor& p, NELGUtil& u, juce::String tooltp = "") :
            NELComp(p, u, tooltp)
        {}

        void paint(juce::Graphics& g) override {
            const auto bounds = getLocalBounds().toFloat().reduced(nelG::Thicc);
            g.setColour(juce::Colour(nelG::ColBlack));
            g.fillRoundedRectangle(bounds, nelG::Rounded);
            g.setColour(juce::Colour(nelG::ColGreen));
            juce::String str;
            str += "My name is Florian Mrugalla and I was born '91.\n";
            str += "When I was still in school I constantly made music.\n";
            str += "I was obsessed and I never slept, because I made beats.\n";
            str += "My beats were all I cared for.\n";
            str += "I was browsing for new free VST Plugins every week.\n";
            str += "VST Developers were my idols, because they kept surprising me\n";
            str += "with new ways to highlight things.\n";
            str += "I always knew I'd end up here. It just took a while.\n";
            str += "Now things get really interesting :)";
            g.drawFittedText(str, bounds.toNearestInt(), juce::Justification::centred, 420, 0);
        }
    };

    struct PageAbout :
        public NELComp {
        PageAbout(Settings& s, Nel19AudioProcessor& p, NELGUtil& u) :
            NELComp(p, u),
            thanksB("Thank\nYou", p, u, [&]() { s.flipPage(std::make_unique<PageThanks>(s, p, u)); }, "A page dedicated to everyone who has helped me on my journey."),
            paypalB("Donate", p, u, juce::URL("https://paypal.me/AlteOma?locale.x=de_DE"), "I wish I didn't need money, but we all do."),
            meB("About\nMe", p, u, [&]() { s.flipPage(std::make_unique<PageAboutMe>(s, p, u)); }, "Do you want to know more about me? :)")
        {
            setInterceptsMouseClicks(false, true);
            addAndMakeVisible(thanksB);
            addAndMakeVisible(paypalB);
            addAndMakeVisible(meB);
        }
    private:
        Button thanksB, paypalB, meB;
        void resized() override {
            const auto bounds = getLocalBounds().toFloat().reduced(nelG::Thicc);
            const auto numTokyoButtons = 3.f;
            auto x = bounds.getX();
            const auto y = bounds.getY();
            const auto width = bounds.getWidth() / numTokyoButtons;
            const auto height = bounds.getHeight();
            const auto minDimen = std::min(width, height);
            const auto radius = minDimen * .5f;
            const auto margin = radius * .4f;
            thanksB.setBounds(juce::Rectangle<float>(x, y, width, height).reduced(margin).toNearestInt());
            x += width;
            paypalB.setBounds(juce::Rectangle<float>(x, y, width, height).reduced(margin).toNearestInt());
            x += width;
            meB.setBounds(juce::Rectangle<float>(x, y, width, height).reduced(margin).toNearestInt());
        }

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PageAbout)
    };

    struct PagePerformance :
        public NELComp
    {
        PagePerformance(Settings& s, Nel19AudioProcessor& p, NELGUtil& u) :
            NELComp(p, u)
        {}
        void paint(juce::Graphics& g) override {
            const auto bounds = getBounds().toFloat().reduced(nelG::Thicc);
            g.setColour(juce::Colour(nelG::ColBlack));
            g.fillRoundedRectangle(bounds, nelG::Rounded);
            g.setColour(juce::Colour(nelG::ColGreen));
            g.drawFittedText("There is nothing in here yet,\nbut I keep this page so I remember that I want to add something.", bounds.toNearestInt(), juce::Justification::centred, 1);
        }
    };

    struct PageInit :
        public NELComp {
        PageInit(Settings& s, Nel19AudioProcessor& p, NELGUtil& u, juce::String tooltp = "") :
            NELComp(p, u, tooltp),
            helpB("Help", p, u, [&]() { s.flipPage(std::make_unique<PageHelp>(s, p, u)); }, "Need some help?"),
            aboutB("About\nNEL-19", p, u, [&]() { s.flipPage(std::make_unique<PageAbout>(s, p, u)); }, "Learn more about my plugin and me."),
            performanceB("Performance\nSettings", p, u, [&]() { s.flipPage(std::make_unique<PagePerformance>(s, p, u)); }, "Go here if you need stuff to go brrr more")
        {
            setInterceptsMouseClicks(false, true);
            addAndMakeVisible(helpB);
            addAndMakeVisible(aboutB);
            addAndMakeVisible(performanceB);
        }
    private:
        Button helpB, aboutB, performanceB;
        void paint(juce::Graphics& g) override {
            
        }
        void resized() override {
            const auto bounds = getLocalBounds().toFloat().reduced(nelG::Thicc);
            const auto numTokyoButtons = 3.f;
            auto x = bounds.getX();
            const auto y = bounds.getY();
            const auto width = bounds.getWidth() / numTokyoButtons;
            const auto height = bounds.getHeight();
            const auto minDimen = std::min(width, height);
            const auto radius = minDimen * .5f;
            const auto margin = radius * .4f;
            helpB.setBounds(juce::Rectangle<float>(x, y, width, height).reduced(margin).toNearestInt());
            x += width;
            aboutB.setBounds(juce::Rectangle<float>(x, y, width, height).reduced(margin).toNearestInt());
            x += width;
            performanceB.setBounds(juce::Rectangle<float>(x, y, width, height).reduced(margin).toNearestInt());
        }

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PageInit)
    };

    Settings(Nel19AudioProcessor& p, NELGUtil& u) :
        NELComp(p, u),
        page()
    {
    }
    ~Settings() override { page.reset(); }
    void open() {
        page = std::make_unique<PageInit>(*this, processor, utils);
        addAndMakeVisible(*page);
        page->setBounds(getLocalBounds());
        setVisible(true);
    }
    void close() {
        setVisible(false);
        page.reset();
    }

    template<typename T>
    void flipPage(std::unique_ptr<T> c) {
        addAndMakeVisible(*c);
        c->setBounds(getLocalBounds());
        setVisible(true);
        page = std::move(c);
    }
    std::unique_ptr<NELComp> page;
private:
    void paint(juce::Graphics& g) override {
        const auto bounds = getLocalBounds().toFloat().reduced(nelG::Thicc);
        g.setColour(juce::Colour(0xbb000000));
        g.fillRoundedRectangle(bounds, nelG::Rounded);
        const auto& vstLogo = nelG::load(BinaryData::vst3_logo_small_png, BinaryData::vst3_logo_small_pngSize, 1);
        const auto vstLogoX = static_cast<int>(bounds.getWidth() - vstLogo.getWidth());
        const auto vstLogoY = static_cast<int>(bounds.getHeight() - vstLogo.getHeight());
        g.drawImageAt(vstLogo, vstLogoX, vstLogoY, false);
    }
    void resized() override { if(page != nullptr) page->setBounds(getLocalBounds()); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Settings)
};

struct SettingsButton :
    public NELComp
{
    SettingsButton(Nel19AudioProcessor& p, Settings& s, NELGUtil& u) :
        NELComp(p, u, "All the extra stuff. Do you feel extra?"),
        settings(s)
    {
        setMouseCursor(u.cursors[NELGUtil::Cursor::Hover]);
    }
private:
    Settings& settings;
    
    void paint(juce::Graphics& g) override {
#if DebugLayout
        g.setColour(juce::Colours::red);
        g.drawRect(getLocalBounds());
#endif
        const auto width = static_cast<float>(getWidth());
        const auto height = static_cast<float>(getHeight());
        const juce::Point<float> centre(width, height);
        const auto minDimen = std::min(width, height);
        juce::Rectangle<float> bounds(
            (width - minDimen) * .5f,
            (height - minDimen) * .5f,
            minDimen,
            minDimen
        );
        bounds.reduce(nelG::Thicc, nelG::Thicc);
        g.setColour(juce::Colour(nelG::ColBlack));
        g.fillRoundedRectangle(bounds, nelG::Rounded);
        if (settings.isVisible()) {
            g.setColour(juce::Colour(nelG::ColRed));
            g.drawRoundedRectangle(bounds, nelG::Rounded, nelG::Thicc);
            g.drawFittedText("X", getLocalBounds(), juce::Justification::centred, 1, 0);
        }
        else {
            g.setColour(juce::Colour(nelG::ColGreen));
            g.drawRoundedRectangle(bounds, nelG::Rounded, nelG::Thicc);
            const auto boundsHalf = bounds.reduced(std::min(bounds.getWidth(), bounds.getHeight()) * .25f);
            g.drawEllipse(boundsHalf.reduced(nelG::Thicc * .5f), nelG::Thicc);
            const auto minDimen = std::min(boundsHalf.getWidth(), boundsHalf.getHeight());
            const auto radius = minDimen * .5f;
            auto bumpSize = radius * .4f;
            juce::Line<float> bump(0, radius, 0, radius + bumpSize);
            const auto translation = juce::AffineTransform::translation(bounds.getCentre());
            for (auto i = 0; i < 4; ++i) {
                const auto x = static_cast<float>(i) / 4.f;
                auto rotatedBump = bump;
                const auto rotation = juce::AffineTransform::rotation(x * nelG::Tau);
                rotatedBump.applyTransform(rotation.followedBy(translation));
                g.drawLine(rotatedBump, nelG::Thicc);
            }
            bumpSize *= .6f;
            bump.setStart(0, radius); bump.setEnd(0, radius + bumpSize);
            for (auto i = 0; i < 4; ++i) {
                const auto x = static_cast<float>(i) / 4.f;
                auto rotatedBump = bump;
                const auto rotation = juce::AffineTransform::rotation(nelG::PiQuart + x * nelG::Tau);
                rotatedBump.applyTransform(rotation.followedBy(translation));
                g.drawLine(rotatedBump, nelG::Thicc);
            }
        }   
    }
    void mouseUp(const juce::MouseEvent& evt) override {
        if (evt.mouseWasDraggedSinceMouseDown()) return;
        if (settings.isVisible()) settings.close();
        else settings.open();
    }
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SettingsButton)
};

/*
* lucas feedback:
ich frage mich wie es wäre, wenn man die knobs genau entgegengesetzt macht. 0%
ist dieser stern und 100% der perfekte kreis. weil je mehr solcher zacken hinzukommen,
desto voller ist ja eig der kreis. das wäre gui wise interessant zu testen.

und ich hätte gerne:
click mix knob for bypass.
dann kriegt der ne pinke umrandung wenn es bypass ist.
und wenn einem das feature aufn sack geht, kann man es dort diablen wo auch
tooltips disabled werden können

kannst du noch einen mode reinmachen, der nicht random sondern nur ne pure sine ist?

kannst du noch nen Plattenspieler-Stop machen. also einfach nur der pitch down effekt?
das wäre so geil
vor allem für UK Garage son backspin sound
*/