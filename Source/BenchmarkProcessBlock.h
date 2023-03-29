#pragma once
#include <JuceHeader.h>
#include <chrono>

namespace benchmark
{
	using Timer = juce::Timer;
	using String = juce::String;
	using StringArray = juce::StringArray;
	using File = juce::File;
	using SpecialLoc = File::SpecialLocationType;
	using AudioProcessor = juce::AudioProcessor;
	using AudioBuffer = juce::AudioBuffer<float>;
	using MidiBuffer = juce::MidiBuffer;

	using Clock = std::chrono::steady_clock;
	using TimePoint = std::chrono::time_point<Clock>;
	using Duration = std::chrono::duration<double>;
	using AtomicDuration = std::atomic<Duration>;
	using Micro = std::chrono::microseconds;
	using Nano = std::chrono::nanoseconds;

	struct Measure
	{
		Measure(AtomicDuration& _duration) :
			duration(_duration),
			timeStart(Clock::now().time_since_epoch())
		{
		}

		~Measure()
		{
			const auto now = Clock::now().time_since_epoch();
			duration.store(now - timeStart);
		}

		AtomicDuration& duration;
		Duration timeStart;
	};

	inline void processBlock(AudioProcessor& p, int numIterations = 1024, int numChannels = 2, int blockSize = 512)
	{
		AtomicDuration duration;
		AudioBuffer buffer(numChannels, blockSize);
		MidiBuffer midi;

		const String name(String(__TIME__).replaceCharacter(':', '_') + "_log.txt");

		const auto desktop = SpecialLoc::userDesktopDirectory;
		const auto folder = File::getSpecialLocation(desktop).getChildFile("Benchmark2");
		if (!folder.exists())
			folder.createDirectory();
		const auto file = folder.getChildFile(name);
		if (file.exists())
			file.deleteFile();
		file.create();

		auto min = std::numeric_limits<long long>::max();
		auto max = std::numeric_limits<long long>::min();
		long long sum = 0;

		for (auto i = 0; i < numIterations; ++i)
		{
			{
				Measure measure(duration);
				p.processBlock(buffer, midi);
			}

			const auto time = std::chrono::duration_cast<Micro>(duration.load()).count();

			if (time < min)
				min = time;
			if (time > max)
				max = time;
			sum += time;

			file.appendText(String(time) + "\n");
		}

		const auto avg = sum / numIterations;
		file.appendText("\nmin: " + String(min));
		file.appendText("\nmax: " + String(max));
		file.appendText("\navg: " + String(avg));
	}

	struct ProcessBlock :
		public Timer
	{
		ProcessBlock() :
			Timer(),
			duration()
		{
			const auto desktop = SpecialLoc::userDesktopDirectory;
			const auto folder = File::getSpecialLocation(desktop).getChildFile("Benchmark");
			if (!folder.exists())
				folder.createDirectory();
			const auto file = folder.getChildFile("log.txt");
			if (file.exists())
				file.deleteFile();
			file.create();

			startTimerHz(12);
		}

		~ProcessBlock()
		{
			const auto desktop = SpecialLoc::userDesktopDirectory;
			const auto folder = File::getSpecialLocation(desktop).getChildFile("Benchmark");
			const auto file = folder.getChildFile("log.txt");
			StringArray lines;
			file.readLines(lines);
			
			auto averageValue = 0.;
			for (auto i = 0; i < lines.size(); ++i)
			{
				const auto line = lines[i];
				const auto value = line.getIntValue();
				averageValue += static_cast<double>(value);
			}

			averageValue /= static_cast<double>(lines.size());

			file.appendText("\navr: " + String(averageValue));
		}
		
		void operator()()
		{
			Measure measure(duration);
		}

		void timerCallback() override
		{
			auto time = std::chrono::duration_cast<Micro>(duration.load());
			
			const auto desktop = SpecialLoc::userDesktopDirectory;
			const auto folder = File::getSpecialLocation(desktop).getChildFile("Benchmark");
			const auto file = folder.getChildFile("log.txt");
			file.appendText(String(time.count()) + "\n");
		}

		AtomicDuration duration;
	};
}