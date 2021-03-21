#pragma once
#include <JuceHeader.h>

class SpaceEditor :
	public juce::Component {
public:
    SpaceEditor() {
		setOpaque(true);
    }
    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colour(nelG::ColBeige));
    }
	void update() {  }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpaceEditor)
};
