# NEL-19 - VST3 - 64bit - Windows only
An open-source high-fidelity random vibrato.


NEL-19 requires the latest C++ dependencies. you can dl and install them here until i make a proper installer that takes care of that:
https://aka.ms/vs/16/release/vc_redist.x64.exe

NEL-19 has:

- Depth-Parameter. Use it to adjust the depth of the vibrato relative to its current buffer size.
- Frequency-Parameter. Use it to adjust the frequency of the randomized LFO.
- Width-Parameter. It mixes in a unique LFO for every other channel.
- STUDIO-Button. Enables Lookahead.
- Buffersize-Textfield. Buffersizes between 1-7ms are rather subtle and have low latency. Everything else is for creative use up to 1000ms.
- MIDI Learn-Button. Use either pitchbend- or midi CC to control the vibrato manually in addition to the randomizer.
- Vibrato Visualizer. It shows what the internal delay is currently doing.

Here are a bunch of videos, where you can check out the plugin in action and topics around it:
https://youtu.be/TVq6FPY_8pU

Join my plugin's discord to discuss feature requests, bugs and to get informed about updates:
https://discord.gg/EEnSNuKZCh

------------------------FAQ:------------------------

Is there a chance to get a MAC version?

Not soon unfortunately. I neither own a mac nor a license to compile for mac. If you have these you could go ahead and compile my plugin for mac yourself, since it's open-source.

---

Why does the plugin not show up in my DAW?

You can try several things
- Check if your DAW supports VST3 64bit
- Check if you copied it into the right folder: C:\Program Files\Common Files\VST3
- Install the latest C++ Dependencies. (sry for this rather geeky solution. this is what an installer would do normally)

---

Does this plugin degrade the audio quality?

I like to compare it to tape wow-and-flutter-emulations, since they also randomly vibrate the signal, but unlike them this plugin adds no saturation, filters or other tape degradation stuff. I used cubic spline interpolation on the delay to make sure that everything sounds really clean. Only thing that could make this even better would be oversampling, but that only matters for the more creative vibratos that enter the realms of scratches, so I didn't do that yet.

---

Can this plugin be used live?

Turn up the depth parameter to the maximum and then use the buffersize-textfield to adjust the depth of the effect in order to get the lowest possible latency. I made sure that it can be used as good as possible live then. Initially I implemented an alternative algorithm trying to compensate for the latency with some tricks if you disable the Studio-Button, but I personally like the approach with the textfield more.

---

Is this plugin safe to use in a commercial project?

I tested it in Cubase, Reaper, Studio One and FL Studio myself and a friend tested it in Ableton. Also I had a bunch of other testers in various of these DAWs already as well. On top of that I ran it with the official VST3 Validator from Steinberg, which throws ridiculous edge cases at plugins in order to test their stability. So it should really be safe. If it ever causes a crash though just throw it out of your VST-Folder. It will most likely not damage your projects. I use it regularly myself as well. Oh, and also please inform me about crashes! It's the only way for me to grow as a developer. :)

---

How to use this to make a LOFI™ sound?

Put the frequency to 4-6hz, since that's supposed to be the wow-rate, dial in some depth with a subtle buffersize (2-7ms), then use some other plugin to add saturation and filters.

---

How do I use the MIDI Learn feature?

You basically just hit that button, then turn a knob on your master keyboard or use the pitchbend wheel, while having some midi track routed to the plugin and it will notice and remember that until you close the DAW. (I'll implement a feature that remembers that in the future)

---

I am a developer and I want to know how to use your dsp code in my own plugins.

Go to Source/Tape.h. It has all the important stuff.

---

Can I recruit you as a dsp developer for my own VST project?

I'd be happy to apply for such jobs in the future, but I'm still spending most of my time studying for university and spend my entire free time learning about dsp, so having another side job would be too much for me now. I hope I remember to update this when my situation has changed^^

---

I think you are really cool and I want to support you by transfering money to your paypal.

Nice, thxx. https://www.paypal.com/paypalme/alteoma

---

I think you are really cool and I want to support you, but I don't want to give you money.

Ok :D Maybe make a cool video about the plugin and share it on tiktok or something.

---

I have a cool idea for a feature or for a workflow improvement.

I love that! Come to my discord pls.

---

Your plugin sucks because I either can't install it or I used it and don't like it.

Even though I definitely consider this plugin a rather solid release, I do plan to extend its features a lot in the future. So maybe it will stop to suck for you some time.
