# NEL - VST3 - 64bit - Windows only
An open-source creative vibrato.


NEL requires C++14 dependencies. you can download and install them here until i make a proper installer that takes care of that:
https://aka.ms/vs/16/release/vc_redist.x64.exe

----------- INTRODUCTION: -----------

NEL is a time-domain vibrato. That means, it essentially uses an internal feed-forward delay to modulate the signal. An LFO is synthesized from weighted pseudo-random values and modulates the distance of that delay. The more its rate changes the stronger the vibrato. Meanwhile the plugin compensates for the length of that delay automatically in order to prevent timing issues automatically. Also I'm using Lanczos Sinc Interpolation on the delay's readhead, which means it sounds clean!

----------- PARAMETERS: -----------

-> Depth.
Use this to adjust the depth of the vibrato. The higher this value is the further apart the random-values of the LFO can be.
-> Depth Max.
You can choose between a bunch of values that set the maximum depth of the vibrato here. This enables you to tweak the behaviour of the depth-parameter. Use low values for a subtle vibrato and high values for strong scratches.
-> Freq.
This defines the frequency in which new random values are picked for the LFO.
-> Shape.
The more you turn up the shape-parameter, the more it will squarify the output of the LFO.
-> Width.
Turn this up to increase the influence of a 2nd LFO to your other channel.
-> L/R;M/S-Switch.
Using the plugin in L/R gives the left and right channel individual LFOs, while in M/S mode the mid- and side-channels are altered seperatedly.
-> Mix.
100% Wet is a vibrato. But dial down the parameter to any other non-zero value to get chorus- or flanger-effects.
-> Parameter-Randomizer.
I didn't implement a preset system yet, but just hitting the parameter randomizer can have a very similiar effect.

----------- EXAMPLES: -----------

Here are a bunch of videos, where you can check out the plugin in action and topics around it:
https://youtu.be/TVq6FPY_8pU

Join my plugin's discord to discuss feature requests, bugs and to get informed about updates:
https://discord.gg/EEnSNuKZCh

----------- TIPPS: -----------

>> USING IT LIVE:
If you want to use NEL live, in realtime, maybe while playing the guitar, turn up the depth as much as possible and then adjust the depth max-parameter to get the sound you want. The lower the depth max-value is the less latency the plugin produces. Values <= 5 should be unnoticable.

>> LOFI AESTHETICS:
Turn the frequency to around 4hz, dial in a bit of shape and you get these very occasional fluctuations like in tape wow- and flutter effects.

>> MONO COMPATIBLE DEPTH:
Use M/S mode, so that you can dial in width without introducing flanging in mono.

>> REVERB SENDS:
Use L/R mode, so that it sounds really wide and then send it into your reverbs to make a dramatic detune.

>> VERSATILE:
Turn down the mix a bit and use really low depth max values, so that the effect essentially turns into a flanger/chorus.

>> OTHERWORLDLY:
Turn down the frequency a lot and select the highest depth max possible, then dial in some width to send your signals into massively different directions.

>> SCRATCH
Automate the depth on a strong depth max to compose a bunch of broken scratches.

------------------------FAQ:------------------------

Is there a chance to get a MAC version?

This is the question I hear the most and I'm in the process of informing myself about mac compatibility, but it's not going to happen tomorrow. Stay tuned I guess.

---

Why does the plugin not show up in my DAW?

You can try several things
- Check if your DAW supports VST3 64bit
- Check if you copied it into the right folder, which is: C:\Program Files\Common Files\VST3
- Install the latest C++ Dependencies. (sry, for this rather geeky solution. this is what an installer would do normally)

---

Is this plugin safe to use in a commercial project?

I tested it in Cubase, Reaper, Studio One and FL Studio myself and a friend tested it in Ableton. Also I had a bunch of other testers in various of these DAWs already as well. On top of that I ran it through the official VST3 Validator from Steinberg, which throws ridiculous edge cases at plugins in order to test their stability and it worked, so it should really be safe. If it ever causes a crash though just throw it out of your VST-Folder. It will not damage your projects. I use it regularly myself as well. Oh, and also please inform me about crashes! It's the only way for me to grow as a developer. :)

---

What are future plans for this plugin?

I once had a MIDI Learn-feature in it and I threw it out for now because I want to implement a much more powerful and modular system with different modulators. Also I'd like to add multiband-features, sidechain-features, parallel-processing features and a bunch of other cool stuff. This plugin is getting better and better. One day it will be the best vibrato of the world.

---

I am a developer and I want to know how to use your dsp code in my own plugins.

Go to Source/DSP.h. It has all the important stuff. Also talk to me on discord.

---

Can I recruit you as a dsp developer for my own VST project?

I'd be happy to apply for such jobs in the future, but I'm still spending most of my time studying for university and spend my entire free time learning about dsp, so having another side job would be too much for me now. I hope I remember to update this when my situation has changed^^

---

I think you are really cool and I want to support you.

Nice. The most straight-forward way to support me is by transfering money to my paypal. https://www.paypal.com/paypalme/alteoma
But I'd also really appreciate you to give me feedback about the plugin and recommend it to your friends and followers on various social media pages.

---

I have a cool idea for a feature or for a workflow improvement.

I love that! Come to my discord pls.

-----------------------------------------------
