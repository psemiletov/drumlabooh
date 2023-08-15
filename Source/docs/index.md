# Drumpecker: plugin (LV, VSTi) drum machine

Drumpecker is an easy way to write drum tracks as MIDI at your DAW, using Hydrogen kits, Drumpeker and Drumrox kits, and SFZ.

Drumrox is LV2 drum machine (based on DrMr) to load Hydrogen and Drumrox drumkits. Drumrox is compatible with MODERN Hydrogen kit format. More of that, Drumrox has own simple drumkit format and supports SFZ.


DONATE MANUAL GET KITS

[User's manual](manual.md)

**Downloads**: [Drumrox github](https://github.com/psemiletov/drumpecker) *
[AUR](https://aur.archlinux.org/packages/drumpecker)

**Links**: [Drumpecker group at Telegram](https://t.me/drumpecker) * [Drumrox kits](https://github.com/psemiletov/drumrox-kits) * [More Drumrox kits at Telegram](https://t.me/drumrox_kits)

## News

Drumrox 3.2.0 (SFZ support + GUI redesign)

Hello!

* This release adds SFZ support.

Actually I was eagered to play AVL Blonde Bop kit (avaliable as SFZ at http://www.bandshed.net/avldrumkits/) with Drumrox, so now it works! Needly to say, that it is not the best way to deal with AVL kits, and Avldrums.lv2 gives more features for them, because of built-in MIDNAM and more precise "exclusive groups" (hihats, crash cymbal, ride cymbal). Drumrox supports just hihats group for the auto-mute, and does it in the different way.

Drumrox searches SFZ's at $HOME/sfz-kits.

Besides AVL SFZ, you can try other SFZ drumkits, for example from musical-artifacts.com.

Thus, currently Drumrox supports Hydrogen kits, Drumrox kits, and SFZ.

* Another new thing is the redesigned GUI. Yes, it is still GTK2-based. I removed the clickable leds, frame captions, but added buttons instead. The buttons with sample names. You can click them to hear, you can watch how they are triggered from DAW, so buttons are just as good old leds, but bigger and with the labels. The plugin window now became wider, and fits to 1280p with big drum kits.

Stay tuned!

Peter Semiletov, Kiev, 04 july 2023

## Features

* LV2 plugin format (currently with GTK2 GUI, works with Ardour and Mixbus only)

* Stereo (with built-in mixer) and 32-channels versions

* Supported sample kit formats: Hydrogen, Drumrox, SFZ

* Up to 32 instruments with layers

* Automatic open hihat mute on hihat close

* Drumkit image at plugin window


![image](https://user-images.githubusercontent.com/8168336/250968814-1b15341c-f59e-413b-8276-807a05089021.png)


## Some history

Drumrox is based on Nicklan's DrMr (https://github.com/nicklan/drmr) and Filipe Coelho's DrMr (https://github.com/falkTX/drmr). The first one can save/load the preset by index, so when you install new kits or delete some, indexes are messing up. falkTX's DrMr deals with presets in more comfortable way, via the names. I (Peter Semiletov) used it everyday, but at the some point of time it becomes incompatible with new Hydrogen kits, and I've edited them manually to fix (removing some section in XML). Fresh install of Hydrogen converts my edited kits to the modern kit format again, and it was simplier to add some code than to edit the kits again. And when I've started to do that, I understand that I want to clean up the code, etc, etc. That how Drumrox continues DrMr...

