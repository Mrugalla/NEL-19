#pragma once
#include <JuceHeader.h>

namespace outtake {
	static void drawRandGrid(juce::Graphics& g, juce::Rectangle<int> bounds, int numVerticalLines = 32, int numHorizontalLines = 16, juce::Colour c = juce::Colours::white, float maxHillSize = .1f) {
        const auto width = static_cast<float>(bounds.getWidth());
        const auto height = static_cast<float>(bounds.getHeight());
        const auto mhsHeight = maxHillSize * height;

        juce::Random rand;

        g.setColour(c);
        std::vector<std::vector<juce::Point<float>>> points;
        points.resize(numVerticalLines);
        for (auto& p : points)
            p.resize(numHorizontalLines);
        for (auto x = 0; x < numVerticalLines; ++x) {
            auto xRatio = float(x) / (numVerticalLines - 1.f);
            auto xStart = xRatio * width;
            auto xEnd = xRatio * width * 2 - width * .5f;
            for (auto y = 0; y < numHorizontalLines; ++y) {
                auto yRatio = (1.f * y) / (numHorizontalLines);
                auto yStart = height * (yRatio * (1.f + 2.f * maxHillSize) * numHorizontalLines) / ((numHorizontalLines - 1.f) - maxHillSize);

                auto xE = xStart + yRatio * (xEnd - xStart);
                auto randVal = rand.nextFloat();
                auto yE = yStart - mhsHeight * randVal * randVal;
                points[x][y] = { xE, yE };
            }
        }
        for (auto x = 0; x < numVerticalLines - 1; ++x) {
            const auto xp1 = x + 1;
            for (auto y = 0; y < numHorizontalLines - 1; ++y) {
                const auto yp1 = y + 1;
                juce::Line<float> horizontalLine(points[x][y], points[xp1][y]);
                juce::Line<float> verticalLine(points[x][y], points[x][yp1]);
                g.drawLine(horizontalLine);
                g.drawLine(verticalLine);
            }
        }
	}
}