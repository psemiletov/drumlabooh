# Drumlabooh: LV2/VSTi drum machine

Drumlabooh is an easy way to write drum tracks as MIDI at your DAW, using Hydrogen, Drumlabooh/Drumrox kits or SFZ.

For all details, docs and drumkits see the [Drumlabooh site](https://psemiletov.github.io/drumlabooh/)


## Features

* Linux LV2/VSTi, Windows VSTi plugin format

* Stereo (with built-in mixer) output

* Built-in mixer with Pan, Volume, mute controls and pan mode option

* Supported sample kit formats: Hydrogen, Drumlabooh/Drumrox, SFZ

* Up to 36 instruments with layers (max 127 layers)

* Automatic open hihat mute on hihat close

* Drumkit image (if provided) at plugin window

* Written in C++. based on JUSE toolkit


## Linux: Build from the source

To build Drumlabooh form the source, you need to install some dependencies first. Drumlabooh is based on JUCE. JUCE source code will be fetched by configuration process, but JUCE needs additional libraries.

Dependencies (Ubuntu packages names):

```libasound2-dev
libfreetype6-dev
libx11-dev
libxcomposite-dev
libxcursor-dev
libxext-dev
libxinerama-dev
libxrandr-dev
libxrender-dev```

Dependencies (Arch packages names):

```alsa-lib
freetype2
libx11
libxcomposite
libxcursor
libxext
libxinerama
libxrandr
libxrender```

Install them if not installed, and then, under sudo or root:

```mkdir b
cd b
cmake ..
make
make install```

This installs Lv2 and VSTi versions of the plugin to ```/usr/local/lib/LV2``` and ```/usr/local/lib/VST3``` by default. To redefine te destination dir, set another dest dir via ```CMAKE_INSTALL_PREFIX``` variable. For example, from ```/usr/local``` to ```/usr```:

```cmake -DCMAKE_INSTALL_PREFIX=/usr ..```

Drumlabooh uses:

[JUCE](https://juce.com) (GPL3)

[speex_resampler_cpp](https://github.com/libaudioverse/speex_resampler_cpp) (Public Domain + Speex License, Copyright (C) 2007 Jean-Marc Valin)