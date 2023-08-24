# Drumlabooh | Драмлабух: About

## Features

* Linux LV2/VSTi plugin format

* Stereo (with built-in mixer) output

* Supported sample kit formats: Hydrogen, Drumrox, SFZ

* Up to 36 instruments with layers (max 127 layers)

* Automatic open hihat mute on hihat close

* Drumkit image at plugin window

* Based on [JUSE toolkit](https://juce.com)


## Some history

Drumlabooh was written as replacement of [Drumrox](https://github.com/psemiletov/drumrox), my fork of DrMr (see Nicklan's DrMr (https://github.com/nicklan/drmr) and Filipe Coelho's (https://github.com/falkTX/drmr)). DrMr is C/Gtk+2 LV2 plugin that I used for a years, but then it stopped support current Hydrogen's drumkit format. First I "fixed" kits XML files manually, and at some point made a fork of DrMr called Drumrox and fix it at the XML parsing level.

It was a spring 2023.

The C language is not my favourite, so I converted codebase to C++ and wrote new drumkit's loader using Pugi xml parser. Soon SFZ and the own, simple kit format has been added. Also I replaced note on/off leds to clickable buttons, made some redesign of the window, but I still feeled myself not comfortable with old code - I did not understand some parts of it. I found LV2 documentation too unclear for me. Alas!

After upgrade of my PC box I started to see that Ardour sometimed record MIDI events with some pre-delay (or negative shift), and I readed about this bug and that it was not be fixed. I would be happy to use Drumrox in another DAW, Reaper, but DrMr UI is based on GTK2, and Reaper does not support GTK2 GUI!

I spend some time to play with some small toolkits that produces X-based GUI, then (of course!) started to write the own one, but tried JUCE and... That how Drumlabooh was born!

From the Drumrox I took my code of drumkits loader. ported it to JUCE file supports format, and resampler I took from Speex.