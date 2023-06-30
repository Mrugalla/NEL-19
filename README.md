# NEL - VST3 - 64bit - Windows only
An open-source creative vibrato.

![2023_06_12_NEL](https://github.com/Mrugalla/NEL-19/assets/54960398/a2f9e52b-6c93-4413-b9db-1be1e5f34eb1)

NEL requires C++20 dependencies. you can download and install them here until i make a proper installer that takes care of that:
https://aka.ms/vs/16/release/vc_redist.x64.exe

----------- INTRODUCTION: -----------

NEL is a vibrato plugin based on resampling. It uses a feed-forward delay to modulate the input signal. A lot of different modulators can be used to create various types of vibrato textures. That makes this plugin useful in many different situations. If lookahead is enabled, its latency is half the internal delay's buffer size, the average of the delay's position, to prevent timing issues. It can also be turned off if the effect is supposed to be used live, for example as a guitar-effect. Most creative vibrato plugins are pretty lofi, but this one attempts to be as clean as possible, with the option to even oversample the effect, opening up the possibility to seperate the distortion processes from the vibrato ones if needed.

------------------------FAQ:------------------------

Is there a chance to get a MAC version?

Whenever I push a new stable version of this plugin I ask one of my mac-friends to compile it again for me.

---

What do I need to download from all these files?

If you're a musician / producer / beat maker / mixing engineer (or whatever)
just download NEL from the release section of this page.
(It's to the right somewhere and says DOWNLOAD)

---

Why does the plugin not show up in my DAW?

You can try several things:
- Check if your DAW supports VST3 64bit
- Check if you copied it into the right folder, which is: C:\Program Files\Common Files\VST3 on windows
- Install the latest C++ Dependencies

else: inform me pls!

---

Is this plugin safe to use in a commercial project?

I have used it in many beats now, so I'm pretty confident about it.

---

I am a developer and I want to know more about your code base.

Check out my youtube channel "Beats basteln :3" and/or come to my development discord.

---

Can I recruit you as a dsp developer for my own VST project?

I'm sometimes searching for jobs and if you like my work, just ask me :)

---

I think you are really cool and I want to support you.

Nice. The most straight-forward way to support me is by transfering money to my paypal. https://www.paypal.com/paypalme/alteoma
But I'd also really appreciate you to give me feedback about the plugin and recommend it to your friends and followers on various social media pages.
Bug reports are important ways to support me as well.
Only a clean code that doesn't crash anyone's DAWs makes everyone happy :3

---

I have a cool idea for a feature or for a workflow improvement.

I love that! Please tell me about it :)

-----------------------------------------------
