# Drumlabooh: LV2/VSTi drum machine

Drumlabooh is an easy way to write drum tracks as MIDI at your DAW, using Hydrogen kits, Drumlabooh/Drumrox kits or SFZ.

For all details, docs and drumkits see the [Drumlabooh site](https://psemiletov.github.io/drumlabooh/)


## Features

* Linux LV2/VSTi, Windows VSTi plugin format

* Supported sample kit formats: Drumlabooh/Drumrox, Hydrogen, SFZ

* Stereo (with built-in mixer) or multi (36 channels) output

* Built-in mixer with Pan, Volume, mute controls and Pan mode option

* FX for each instrument slot: LP, HP resonance filters and Analog knob to make the sound warmer

* Up to 36 instruments with layers (max 127 layers)

* Round Robin and Cycle Random modes for each instrument

* Automatic open hihat mute on hihat close

* Drumkit image (if provided) at plugin window

* Written in C++, based on JUCE toolkit


## Linux: Quick install

To install/update binaries and kits, use [drumlabooh-net-installer-script.sh](https://github.com/psemiletov/drumlabooh/releases/download/8.0.0/drumlabooh-net-installer-script.sh) or [drumlabooh-net-install](https://github.com/psemiletov/drumlabooh/releases/download/8.0.0/drumlabooh-net-install) (static build console program).


## Linux: Build from the source

To build Drumlabooh form the source, you need to install some dependencies first. Drumlabooh is based on JUCE. JUCE source code will be fetched by configuration process, but JUCE needs additional libraries.

Dependencies (Debian/Ubuntu packages names):

``pkgconf
libasound2-dev
libfreetype6-dev
libx11-dev
libxcomposite-dev
libxcursor-dev
libxext-dev
libxinerama-dev
libxrandr-dev
libxrender-dev
libfontconfig1-dev //FOR UBUNTU 24.x!!!!
``


Dependencies (Arch packages names):


``alsa-lib
freetype2
libx11
libxcomposite
libxcursor
libxext
libxinerama
libxrandr
libxrender``

Install them if not installed, and then, under sudo or root:

```
mkdir b
cd b
cmake ..
make
make install
```

This installs LV2 and VSTi versions of the plugin to ``/usr/local/lib/LV2`` and ``/usr/local/lib/VST3`` by default. To redefine te destination dir, set another dest dir via ``CMAKE_INSTALL_PREFIX`` variable. For example, from ``/usr/local`` to ``/usr``:

```
cmake -DCMAKE_INSTALL_PREFIX=/usr ..
```

## Notes for packagers/maintainers

By default, Drumlabooh cmake file fetches and installs the all drumkits from https://github.com/psemiletov/drum_sklad. You can turn this behavior off using ```cmake .. -DINSTALLKITS=OFF``` option, and make the separated data package with drum kits from 
```https://github.com/psemiletov/drum_sklad/archive/refs/heads/main.zip```


## Drumlabooh uses:

[JUCE](https://juce.com) (GPL3)

[resampler by David Bryant](https://github.com/dbry/audio-resampler) (BSD-3-Clause license)


