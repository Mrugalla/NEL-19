# NEL - VST3 - 64bit - Windows/Mac
An open-source creative vibrato.

![2023_06_12_NEL](https://github.com/Mrugalla/NEL-19/assets/54960398/a2f9e52b-6c93-4413-b9db-1be1e5f34eb1)

----------- INTRODUCTION: -----------

NEL is a vibrato plugin. It uses a feed-forward delay to modulate the signal in pitch and time.
Various modulators can be used to create all kinds of vibrato textures, which makes this plugin pretty versatile.
If lookahead is enabled, the plugin's latency compensates for half the internal delay's buffer size in order to average out the modulation in time.
Turn it off for a more realtime-friendly workflow, for example as a guitar-effect. Also reduce the buffer size of the plugin then!
Most creative vibrato plugins sound lofi, they degrade the sound quality.
But this one attempts to be as clean as possible, you can even activate oversampling to reduce sidelobe artefacts from fast modulation.

------------------------FAQ:------------------------

What do I need to download from all these files?

If you're a musician / producer / beat maker / mixing engineer (or whatever)
just download NEL from the release section of this page.
(It's to the right somewhere and says DOWNLOAD)

---

Why does the plugin not show up in my DAW?

NEL requires C++20 dependencies. you can download and install them here until i make a proper installer that takes care of that:
https://aka.ms/vs/16/release/vc_redist.x64.exe

You can try several things:
- Check if your DAW supports VST3/AU 64bit
- Check if you copied it into the right folder, which is: C:\Program Files\Common Files\VST3 on windows
- Install the latest C++ Dependencies

if none of this helped, inform me pls!

-----------------------------------------------
