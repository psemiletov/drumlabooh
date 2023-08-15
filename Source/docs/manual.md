# User's Manual

Drumrox is aimed to be lightweight but powerful LV2 instrument to use selected Hydrogen or Drumrox drumkit on MIDI track in your DAW. With the current GTK2 interface, Drumrox can work normally with **Ardour and Mixbus**, but not with Reaper and Qtractor.

To use Drumrox, you need to install some drumkits first. For Hydrogen drumkits, run Hydrogen, then go there to **Drumkits - Online import** menu and download kits from Hydrogen or Musical Artifacts repositories. To install Drumrox format kits, download them from [Drumrox kits](https://github.com/psemiletov/drumrox-kits) or [Drumrox kits at Telegram] (https://t.me/drumrox_kits), and copy or unpack drumkits folders to ```$HOME/drumrox-kits```. Each kit = one folder.


## Drumrox stereo

The simpliest way to hear your drums it's to use the plain **Drumrox**, not the **Drumrox-multi**.

When you add Drumrox to the track at your DAW, you get the stereo output, to output all kit's instruments (kick, snare, hihats, etc), initially mixed at Drumrox's built-in mixer with pan and gain knobs. Also you can change the [paning options](https://en.wikipedia.org/wiki/Pan_law).

To start play with drums, select the drumkit from the drop-down list. There are all kits that Drumrox found at:

```/usr/share/hydrogen/data/drumkits
/usr/local/share/hydrogen/data/drumkits
/usr/share/drmr/drumkits
/usr/share/drumrox-kits
$HOME/.hydrogen/data/drumkits
$HOME/.drmr/drumkits
$HOME/sfz-kits
$HOME/drumrox-kits

```

As you see, Drumrox searches at Hydrogen's directories, legacy DrMr's paths, and its own ```$HOME/drumrox-kits```. Drumrox kits must be placed to ```$HOME/drumrox-kits```, and SFZ kits to ```$HOME/sfz-kits```.

All scanned kits will be available at Drumrox from the drop-down menu.

You can change the kit during playing. It can cost a bit of time, so wait until the new kit is loaded. Drumrox resamples instruments when load, if the session's sample rate differs from the kit's sample rate.

By clicking on buttons with drum sample names you can hear them without any MIDI input.

On saving the session, Drumrox saves the full path of the current drumkit, so if you move the drumkit to another directory, Drumkit can't find it later.


## Drumrox multi

This version of Drumrox provides 32 output mono channels, one per sample of the drumkit. In this mode, Drumrox's internal mixer is disabled, you need to set gains and panings via track strips at DAW.

How to use it with Ardour?

Watch this video guide:

[English language version](https://www.youtube.com/watch?v=NqzB98iONMA)
[Видео на русском](https://www.youtube.com/watch?v=k_ImLh4pSvI&t=8s)

Or read this:

1. Create MIDI track with Drumrox-muiti as the instrument.

2. Load drumkit into the Drumrox-multi. For example, Hydrogen's GMRockKit. It has 18 instruments inside. Thus, we need just 18 output channels from the 32 available.

3. By default, Drumrox-multi sounds at stereo-output. We don't need that. Let's create the individual channels or buses for each drum/precussion sample.

4. Right click on the channel strip and select **Fan out to tracks** or **Fan out to buses**. Now you have one track or bus per channel to control each of 32 drumkit channels. Please note that tracks and buses are mono, so to use stereo FX on them, do the right click on the effect, select **Pin connection** and connect pins to make stereo:

![image](https://user-images.githubusercontent.com/8168336/248489497-e8748cad-c457-49b6-a2f9-62559b8f1434.png)

5. All new tracks/channels are in the group now. To make them properly being muted, soloed, etc, do the right click at the group header, and select **Remove group**:

![image](https://user-images.githubusercontent.com/8168336/248851730-6f7bab35-ec86-45eb-af5c-3d19b5fed5c2.png)

6. Now, remove unneeded tracks (for example, the kit has only 8 instruments, but you have 32 tracks from Drumrox!)
