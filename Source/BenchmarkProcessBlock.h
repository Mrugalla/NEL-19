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

	using Clock = std::chrono::steady_clock;
	using TimePoint = std::chrono::time_point<Clock>;
	using Duration = std::chrono::duration<double>;
	using AtomicDuration = std::atomic<Duration>;
	using Micro = std::chrono::microseconds;
	using Nano = std::chrono::nanoseconds;

	struct ProcessBlock :
		public Timer
	{
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
			auto nano = std::chrono::duration_cast<Nano>(duration.load());
			
			// go to Benchmark/log.txt
			
			const auto desktop = SpecialLoc::userDesktopDirectory;
			const auto folder = File::getSpecialLocation(desktop).getChildFile("Benchmark");
			const auto file = folder.getChildFile("log.txt");
			file.appendText(String(nano.count()) + "\n");
		}

		AtomicDuration duration;
	};
}