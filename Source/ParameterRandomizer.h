#pragma once

#pragma once
#include <JuceHeader.h>

struct RandomizerButton :
    public Component
{
    RandomizerButton(Nel19AudioProcessor& p, Utils& u) :
        Component(p, u, makeTooltip(), Utils::Cursor::Hover),
        randomizables()
    {
    }
    void addRandomizable(Parameter* randomizableParameter) {
        randomizables.push_back(randomizableParameter);
    }
protected:
    std::vector<Parameter*> randomizables;

    void paint(juce::Graphics& g) override {
        const auto width = static_cast<float>(getWidth());
        const auto height = static_cast<float>(getHeight());
        const juce::Point<float> centre(width, height);
        auto minDimen = std::min(width, height);
        juce::Rectangle<float> bounds(
            (width - minDimen) * .5f,
            (height - minDimen) * .5f,
            minDimen,
            minDimen
        );
        bounds.reduce(nelG::Thicc, nelG::Thicc);
        g.setColour(utils.colours[Utils::Background]);
        g.fillRoundedRectangle(bounds, nelG::Thicc);
        g.setColour(utils.colours[Utils::Normal]);
        g.drawRoundedRectangle(bounds, nelG::Thicc, nelG::Thicc);

        minDimen = std::min(bounds.getWidth(), bounds.getHeight());
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
        for (auto randomizable : randomizables)
            if(randomizable->isActive())
                randomizable->setValueNormalized(rand.nextFloat());
        tooltip = makeTooltip();
    }
    void mouseExit(const juce::MouseEvent&) override { tooltip = makeTooltip(); }
    juce::String makeTooltip() {
        juce::Random rand;
        static constexpr float count = 134.f;
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
        case 53: return "What coloursheme does your tune sound like?";
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
        case 78: return "Beware! Only really skilled audio professionals use this feature.";
        case 79: return "What would be your melody's name if it was a human being?";
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
        case 101: return "Drama!";
        case 102: return "Turn it up! Well, this is not a knob, but you know, it's cool.";
        case 103: return "You like it dangerous, huh?";
        case 104: return "We are under attack.";
        case 105: return "Yes, you want this!";
        case 106: return "The randomizer is better than your presets!";
        case 107: return "Are you a decide-fan, or a random-enjoyer?";
        case 108: return "Let's get it started! :)";
        case 109: return "Do what you have to do...";
        case 110: return "This is a special strain of random. ;)";
        case 111: return "Return to the battlefield or get killed.";
        case 112: return "~<* Easy Peazy Lemon Squeezy *>~";
        case 113: return "Why does it sound like dubstep?";
        case 114: return "Excuse me.. Have you seen my sanity?";
        case 115: return "In case of an emergency, push the button!";
        case 116: return "Based.";
        case 117: return "Life is a series of random collisions.";
        case 118: return "It is actually possible to add too much salt to spaghetti.";
        case 119: return "You can't go wrong with random, except when you do.";
        case 120: return "I have not implemented undo yet, but you like to live dangerously :)";
        case 121: return "404 - Creative message not found. Contact our support pls.";
        case 122: return "Press jump twice to perform a doub.. oh wait, wrong app.";
        case 123: return "And now for the ultimate configuration!";
        case 124: return "Subscribe for more random messages! Only 40$/mon";
        case 125: return "I love you <3";
        case 126: return "Me? Well...";
        case 127: return "What happens if I press this?";
        case 128: return "Artificial Intelligence! Not used here, but it sounds cool.";
        case 129: return "My internet just broke so why not just write another msg in here, right?";
        case 130: return "Mood.";
        case 131: return "I'm only a randomizer, after all...";
        case 132: return "There is a strong correlation between you and awesomeness.";
        case 133: return "Yes! Yes! Yes!";
        case 134: return "Up for a surprise?";
        default: "Are you sure?";
        }
        return "You are not supposed to read this message!";
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RandomizerButton)
};