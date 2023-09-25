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
        const auto ext = ".nel";
        
        const auto load = [&](juce::String&& name, const void* data, int size)
        {
            const auto file = directoryPresets.getChildFile(name + ext);
            if (file.existsAsFile())
                file.deleteFile();
            const auto result = file.create();
            if (result.failed())
                return;
            const auto str = juce::String::createStringFromData(data, size);
            file.appendText(str);
        };

        load("Drums", BinaryData::Drums_nel, BinaryData::Drums_nelSize);
        load("Flanger", BinaryData::Flanger_nel, BinaryData::Flanger_nelSize);
        load("Lofi", BinaryData::Lofi_nel, BinaryData::Lofi_nelSize);
        load("Lunatic", BinaryData::Lunatic_nel, BinaryData::Lunatic_nelSize);
        load("Phase Distortion", BinaryData::Phase_Distortion_nel, BinaryData::Phase_Distortion_nelSize);
        load("Vibrato", BinaryData::Vibrato_nel, BinaryData::Vibrato_nelSize);
	}
}