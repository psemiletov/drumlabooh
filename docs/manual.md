# Drumlabooh | Драмлабух: User's Manual

Drumlabooh is aimed to not interfere with creativity. All that you need is to create a new MIDI track at your DAW, select the drumkit from the list, and play. Yes indeed, Drumlabooh is not for inventors of new, synthesized drum sounds. Drumlabooh uses the existing, sampled drumkits, so you need to install them first.


## Get kits

For Hydrogen drumkits, run Hydrogen, then go there to **Drumkits - Online import** menu and download kits from Hydrogen or Musical Artifacts repositories. To install Drumlabooh format kits, download them from [Drum sklad](https://github.com/psemiletov/drum_sklad) or [Drumlabooh kits at Telegram] (https://t.me/drum_sklad), and copy or unpack drumkits folders to ```$HOME/drumlabooh-kits```. Each kit = one folder.

Similary, Dramlabooh searches SFZ kits at ```$HOME/sfz-kits```

All kits those found will be available from the menu (drop-down list) at the top left corner of the plugin.


Note: actually, Drumalbooh seraches kits at many locations.

Hydrogen format:

```/usr/share/hydrogen/data/drumkits
/usr/local/share/hydrogen/data/drumkits
/usr/share/drmr/drumkits
$HOME/.hydrogen/data/drumkits
$HOME/.drmr/drumkits
```

Drumlabooh/Drumrox format:

```
/usr/share/drumrox-kits
$HOME/drumrox-kits
/usr/share/drumlabooh-kits
$HOME/drumlabooh-kits

```

SFZ format:

```
$HOME/sfz-kits
```

When Drumlabooh loads the kit samples, it converts them (if needed) to the DAW's project session rate, so the conversion can take some time. Please note that any resampling changes how samples sounds, at some degree.

## Built-in mixer

Below the drumkit selector you see the cells with loaded samples. Here the layout of each cell:

1 Led - lights when note plays.

2 Instrument name - taken from the kit xml file, or in another way.

3 Pan control - values from 0 to 1, 0.5 is a middle.

4. Gain slider - non-linear, values from -96 to +6 dB.

5. Mute switch - mutes instrument.


## Built-in options

By the right side of the plugin window you see, below the drumkit info section, the Options.

Base MIDI note - default 36, means at witch MIDI note number we map the first instrument at the kit. MIDI note 36 = C at 1-st octave.

Pan mode - allow to select the pan mode for Drumlabooh's internal mixer engine. See the [article](https://en.wikipedia.org/wiki/Panning_law)