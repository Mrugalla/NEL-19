#pragma once
#include <JuceHeader.h>

class SpaceEditor :
	public NELComp {
public:
    SpaceEditor(Nel19AudioProcessor& p, NELGUtil& u) :
        NELComp(p, u)
    {
		setOpaque(true);
    }
    void paint(juce::Graphics& g) override { g.fillAll(juce::Colour(nelG::ColBeige)); }
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpaceEditor)
};
