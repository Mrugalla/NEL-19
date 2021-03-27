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
        const auto bounds = getLocalBounds().toFloat().reduced(nelG::Thicc);
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

struct RandomizerButton :
    public NELComp
{
    RandomizerButton(Nel19AudioProcessor& p, NELGUtil& u) :
        NELComp(p, u, makeTooltip())
    {
        setMouseCursor(u.cursors[NELGUtil::Cursor::Hover]);
    }
private:
    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat().reduced(nelG::Thicc);
        g.setColour(juce::Colour(nelG::ColBlack));
        g.fillRoundedRectangle(bounds, nelG::Rounded);
        g.setColour(juce::Colour(nelG::ColGreen));
        g.drawRoundedRectangle(bounds, nelG::Rounded, nelG::Thicc);
        
        const auto minDimen = std::min(bounds.getWidth(), bounds.getHeight());
        const auto radius = minDimen * .5f;
        const auto pointSize = radius * .4f;
        const auto pointRadius = pointSize * .5f;
        const auto d4 = minDimen / 4.f;
        const auto x0 = d4 * 1.2f + bounds.getX();
        const auto x1 = d4 * 2.8f + bounds.getX();
        for (auto i = 1; i < 4; ++i) {
            const auto y = d4 * i + bounds.getY();
            g.fillEllipse(x0 - pointRadius, y - pointRadius, pointSize, pointSize);
            g.fillEllipse(x1 - pointRadius, y - pointRadius, pointSize, pointSize);
        }
    }
    void mouseUp(const juce::MouseEvent& evt) override {
        if (evt.mouseWasDraggedSinceMouseDown()) return;
        juce::Random rand;
        auto state = processor.apvts.state;
        const auto numChildren = state.getNumChildren();
        for (auto c = 0; c < numChildren; ++c) {
            auto child = state.getChild(c);
            if (child.hasType("PARAM")) {
                const auto pID = child.getProperty("id").toString();
                auto param = processor.apvts.getParameter(pID);
                const auto value = param->convertFrom0to1(rand.nextFloat());
                juce::ParameterAttachment attach(*param, [this](float) {}, nullptr);
                attach.setValueAsCompleteGesture(value);
            }
        }
        tooltip = makeTooltip();
        getTopLevelComponent()->repaint();
    }
    void mouseExit(const juce::MouseEvent& evt) override { tooltip = makeTooltip(); }
    juce::String makeTooltip() {
        juce::Random rand;
        const auto count = 100.f;
        const auto v = static_cast<int>(std::rint(rand.nextFloat() * count));
        switch (v) {
        case 0: return "Do it!";
        case 1: return "Don't you dare it!";
        case 2: return "But... what if it goes wrong???";
        case 3: return "Nature is random too, so this is basically analog, right?";
        case 4: return "Life is all about exploration..";
        case 5: return "What if I don't even exist?";
        case 6: return "Idk, it's all up to you.";
        case 7: return "This randomizes the parameter values. Yeah..";
        case 8: return "Born too early to explore space, born just in time to hit the randomizer.";
        case 9: return "Imagine someone sitting there writing down all these phrases.";
        case 10: return "This will not save your snare from sucking ass.";
        case 11: return "Producer-san >.< d.. don't tickle me there!!!";
        case 12: return "I mean, whatever.";
        case 13: return "Never commit. Just dream!";
        case 14: return "I wonder, what will happen if I...";
        case 15: return "Hit it for the digital warmth.";
        case 16: return "Do you like cats? They are so cute :3";
        case 17: return "We should collab some time, bro.";
        case 18: return "Did you just open the plugin UI just to see what's in here this time?";
        case 19: return "I should make a phaser. Would you want a phaser in here?";
        case 20: return "No time for figuring out parameter values manually, right?";
        case 21: return "My cat is meowing at the door, because there is a mouse.";
        case 22: return "Yeeeaaaaahhhh!!!! :)";
        case 23: return "Ur hacked now >:) no just kidding ^.^";
        case 24: return "What would you do if your computer could handle 1mil phasers?";
        case 25: return "It's " + (juce::Time::getCurrentTime().getHours() < 10 ? juce::String("0") + static_cast<juce::String>(juce::Time::getCurrentTime().getHours()) : static_cast<juce::String>(juce::Time::getCurrentTime().getHours())) + ":" + (juce::Time::getCurrentTime().getMinutes() < 10 ? juce::String("0") + static_cast<juce::String>(juce::Time::getCurrentTime().getMinutes()) : static_cast<juce::String>(juce::Time::getCurrentTime().getMinutes())) + " o'clock now.";
        case 26: return "I was a beat maker, too, but then I took a compressor to the knee.";
        case 27: return "It's worth a try.";
        case 28: return "Omg, your music is awesome dude. Keep it up!";
        case 29: return "I wish there was an anime about music producers.";
        case 30: return "Days are too short, but I also don't want gravity to get heavier.";
        case 31: return "Yo, let's order some pizza!";
        case 32: return "I wanna be the very best, like no one ever was!";
        case 33: return "Hm... yeah, that could be cool.";
        case 34: return "Maybe...";
        case 35: return "Well.. perhaps.";
        case 36: return "Here we go again.";
        case 37: return "What is the certainty of a certainty meaning a certain certainty?";
        case 38: return "My favourite car is an RX7 so i found it quite funny when Izotope released that plugin.";
        case 39: return "Do you know echobode? It's one of my favourites.";
        case 40: return "I never managed to make a proper eurobeat even though I love that genre.";
        case 41: return "Wanna lose control?";
        case 42: return "Do you have any more of dem randomness pls?";
        case 43: return "How random do you want it to be, sir? Yes.";
        case 44: return "Programming is not creative. I am a computer.";
        case 45: return "We should all be more mindful to each other.";
        case 46: return "Next-Level AI will randomize ur params!";
        case 47: return "All the Award-Winning Audio-Engineers use this button!!";
        case 48: return "The fact that you can't undo it only makes it better.";
        case 49: return "When things are almost as fast as light, reality bends.";
        case 50: return "I actually come from the future. Don't tell anyone pls.";
        case 51: return "You're mad!";
        case 52: return "Your ad could be here! ;)";
        case 53: return "What colours does your beat sound like?";
        case 54: return "I wish Dyson Spheres existed already!";
        case 55: return "This is going to be so cool! OMG";
        case 56: return "Plants. There should be more of them.";
        case 57: return "10 Vibrato Mistakes Every Noob Makes: No 7 Will Make U Give Up On Music!";
        case 58: return "Yes, I'll add more of these some other time.";
        case 59: return "The world wasn't ready for No Man's Sky. That's all.";
        case 60: return "Temposynced Tremolos are not Sidechain Compressors.";
        case 61: return "I can't even!";
        case 62: return "Let's drift off into the distance together..";
        case 63: return "When I started making this plugin I thought I'd make a tape emulation.";
        case 64: return "Scientists still trying to figure this one out..";
        case 65: return "Would you recommend this button to your friends?";
        case 66: return "This is a very bad feature. Don't use it!";
        case 67: return "I don't know what to say about this button..";
        case 68: return "A parallel universe, in which you will use this button now, exists.";
        case 69: return "This is actually message no 69, haha";
        case 70: return "Who needs control anyway?";
        case 71: return "I have the feeling this time it will turn out really cool!";
        case 72: return "Turn all parameters up right to 11.";
        case 73: return "Tranquilize Your Music. Idk why, but it sounds cool.";
        case 74: return "I'm indecisive...";
        case 75: return "That's a good idea!";
        case 76: return "Once upon a time there was a traveller who clicked this button..";
        case 77: return "10/10 Best Decision!";
        case 78: return "Only really skilled audio professionals use this feature.";
        case 79: return "What would your melody's name if it was a human being?";
        case 80: return "What if humanity was just a failed experiment by a higher species?";
        case 81: return "Enter the black hole to stop time!";
        case 82: return "Did you remember to water your plants yet?";
        case 83: return "I'm just a simple button. Nothing special to see here.";
        case 84: return "You're using this plugin. That makes you a cool person.";
        case 85: return "Only the greatest DSP technology in this parameter randomizer!";
        case 86: return "I am not fun at parties indeed.";
        case 87: return "This button makes it worse!";
        case 88: return "I am not sure what this is going to do.";
        case 89: return "If your music was a mountain, what shape would it be like?";
        case 90: return "This is the best Vibrato Plugin in the world. Tell ur friends";
        case 91: return "Do you feel the vibrations?";
        case 92: return "Defrost or Reheat? You decide.";
        case 93: return "Don't forget to hydrate yourself, king.";
        case 94: return "How long does it take to get to the next planet at this speed?";
        case 95: return "What if there is a huge wall around the whole universe?";
        case 96: return "Controlled loss of control. So basically drifting! Yeah!";
        case 97: return "I talk to the wind. My words are all carried away.";
        case 98: return "Captain, we need to warp now! There is no time.";
        case 99: return "Where are we now?";
        case 100: return "Randomize me harder, daddy!";
        default: "Are you sure?";
        }
        return "You are not supposed to read this message!";
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RandomizerButton)
};