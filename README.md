# NEL - VST3 - 64bit - Windows only
An open-source creative vibrato.


NEL requires C++14 dependencies. you can download and install them here until i make a proper installer that takes care of that:
https://aka.ms/vs/16/release/vc_redist.x64.exe

----------- INTRODUCTION: -----------

NEL is a vibrato plugin based on resampling. It essentially uses a feed-forward delay to modulate the signal. A lot of different modulators can be used to create different vibrato textures. That makes this plugin useful in many different situations. Its latency is the average of the delay's position to prevent timing issues when lookahead is enabled, but it can also be turned off if the effect is supposed to be used live, for example as a guitar-effect. Most creative vibrato plugins are pretty lofi, but this one attempts to be as clean as possible, opening up the possibility to seperate the distortion processes from the vibrato ones if needed.

Disclaimer: If any of the features mentioned in this info are not in the version of the plugin that you use, it's because I have not pushed the changes to the 'master' repository yet. But beware of the 'develop' branch. It might be really buggy at times.

------------------------FAQ:------------------------

Is there a chance to get a MAC version?

Tbh it's a bit discouraging to see how all my fellow mac devs constantly have to buy new gear to fix mac os bugs. That's all time wasted that could be spent fixing actual dsp bugs or implementing new features. But if I ever find a way to expand my compatibility to mac in a reasonable way, I'll do it.

---

What do I need to download from all these files?

If you're a musician / producer / beat maker / mixing engineer (or whatever) just download NEL.vst3

---

Why does the plugin not show up in my DAW?

You can try several things
- Check if your DAW supports VST3 64bit
- Check if you copied it into the right folder, which is: C:\Program Files\Common Files\VST3
- Install the latest C++ Dependencies

or inform me!!

---

Is this plugin safe to use in a commercial project?

I hope so, but if you find any bugs, please contact me!

---

I am a developer and I want to know more about your code base.

Check out my youtube channel "Beats basteln :3" and/or come to my development discord.

---

Can I recruit you as a dsp developer for my own VST project?

I'd be happy to apply for such jobs in the future, but I'm still spending most of my time studying for university and spend my entire free time learning about dsp and audio programming in general, so having another side job would be too much for me now. I hope I remember to update this message when my situation has changed^^

---

I think you are really cool and I want to support you.

Nice. The most straight-forward way to support me is by transfering money to my paypal. https://www.paypal.com/paypalme/alteoma
But I'd also really appreciate you to give me feedback about the plugin and recommend it to your friends and followers on various social media pages. Bug reports are important ways to support me as well. Only a clean code that doesn't crash anyone's daws can make everyone happy.

---

I have a cool idea for a feature or for a workflow improvement.

I love that! Come to my discord pls.

-----------------------------------------------
