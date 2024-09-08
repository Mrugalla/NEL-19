#pragma once
#include <JuceHeader.h>

namespace presets
{
	inline void init(juce::ApplicationProperties& appProperties)
	{
        const auto& user = *appProperties.getUserSettings();
        const auto fileSettings = user.getFile();
        const auto directorySettings = fileSettings.getParentDirectory();
        const auto directoryPresets = directorySettings.getChildFile("Presets");
        if (!directoryPresets.exists())
            directoryPresets.createDirectory();
        auto ext = ".nel";
        
        const auto load = [&](juce::String&& name, const juce::File& direc, const void* data, int size)
        {
            const auto file = direc.getChildFile(name + ext);
            if (file.existsAsFile())
                return;
            const auto result = file.create();
            if (result.failed())
                return;
            const auto str = juce::String::createStringFromData(data, size);
            file.appendText(str);
        };

        load("Drums", directoryPresets, BinaryData::Drums_nel, BinaryData::Drums_nelSize);
        load("Flanger", directoryPresets, BinaryData::Flanger_nel, BinaryData::Flanger_nelSize);
        load("Lofi", directoryPresets, BinaryData::Lofi_nel, BinaryData::Lofi_nelSize);
        load("Lunatic", directoryPresets, BinaryData::Lunatic_nel, BinaryData::Lunatic_nelSize);
        load("Phase Distortion", directoryPresets, BinaryData::Phase_Distortion_nel, BinaryData::Phase_Distortion_nelSize);
        load("Vibrato", directoryPresets, BinaryData::Vibrato_nel, BinaryData::Vibrato_nelSize);

		const auto directoryColours = directorySettings.getChildFile("Colours");
		if (!directoryColours.exists())
			directoryColours.createDirectory();
		ext = ".col";

		load("Blue", directoryColours, BinaryData::Blue_col, BinaryData::Blue_colSize);
        load("Creamy", directoryColours, BinaryData::Creamy_col, BinaryData::Creamy_colSize);
		load("Dark", directoryColours, BinaryData::Dark_col, BinaryData::Dark_colSize);
		load("Frosty", directoryColours, BinaryData::Frosty_col, BinaryData::Frosty_colSize);
        load("GRiP", directoryColours, BinaryData::GRiP_col, BinaryData::GRiP_colSize);
        load("Lime", directoryColours, BinaryData::Lime_col, BinaryData::Lime_colSize);
		load("Milka", directoryColours, BinaryData::Milka_col, BinaryData::Milka_colSize);
		load("Nowgad", directoryColours, BinaryData::Nowgad_col, BinaryData::Nowgad_colSize);
        load("Techy", directoryColours, BinaryData::Techy_col, BinaryData::Techy_colSize);
	}
}