# NEL - VST3 - 64bit - Windows only
An open-source creative vibrato.


NEL requires C++14 dependencies. you can download and install them here until i make a proper installer that takes care of that:
https://aka.ms/vs/16/release/vc_redist.x64.exe

----------- INTRODUCTION: -----------

NEL is a vibrato based on resampling. That means, it essentially uses a feed-forward delay to modulate the signal. Different modulators can be used to create different vibrato textures. The plugin's latency is the average of the delay's position to prevent timing issues. Most creative vibrato plugins are pretty lofi, but this one attempts to be clean, opening up the possibility to seperate the distortion process from the vibrato if needed.

Disclaimer: If any of the features mentioned in this info are not in the version of the plugin that you use, it's because I have not pushed the changes to the 'master' repository yet. But beware of the 'develop' branch. It might be really buggy at times.

----------- PARAMETERS: -----------

*I'll write a new parameter list once I've added all of the essential parameters of the new version*

----------- EXAMPLES: -----------

Here are a bunch of videos, where you can check out the plugin in action and topics around it:
https://youtu.be/TVq6FPY_8pU

Join my plugin's discord to discuss feature requests, bugs and to get informed about updates:
https://discord.gg/EEnSNuKZCh

----------- TIPPS: -----------

*this will be rewritten when the new version is done*

------------------------FAQ:------------------------

Is there a chance to get a MAC version?

Tbh it's a bit discouraging to see how all my fellow mac devs constantly have to buy new gear to fix mac os bugs. That's all time wasted that could be spent fixing actual dsp bugs or implementing new features. But if I ever find a way to expand my compatibility to mac in a reasonable way, I'll do it.

---

What do I need to download from all these files?

If you're just a musician / producer / beat maker / mixing engineer (or whatever) just download NEL.vst3

---

Why does the plugin not show up in my DAW?

You can try several things
- Check if your DAW supports VST3 64bit
- Check if you copied it into the right folder, which is: C:\Program Files\Common Files\VST3
- Install the latest C++ Dependencies

or Inform Me!!

---

Is this plugin safe to use in a commercial project?

The version on the master branch should be fine. The develop one can be really buggy at times. It's where I test new features and stuff.

---

What are future plans for this plugin?

I once had a MIDI Learn-feature in it and I threw it out for now because I want to implement a much more powerful and modular system with different modulators. Also I'd like to add multiband-features, sidechain-features, parallel-processing features and a bunch of other cool stuff. This plugin is getting better and better. One day it will be the best vibrato in the world.

---

I am a developer and I want to know more about your code base.

Check out my youtube channel "Beats basteln :3" and/or come to my development discord.

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
