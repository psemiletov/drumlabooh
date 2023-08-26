# Drumlabooh | Драмлабух: Download and install

[Drumlabooh github source](https://github.com/psemiletov/drumlabooh)

[AUR](https://aur.archlinux.org/packages/drumlabooh)

To build Drumlabooh form the source, you need to install some dependencies first. Drumlabooh is based on JUCE, JUCE source code will be fetched by configuration process, so JUCE needs additional libraries.

Dependencies (Ubuntu packages names):

```
libasound2-dev
libfreetype6-dev
libx11-dev
libxcomposite-dev
ibxcursor-dev
libxext-dev
libxinerama-dev
libxrandr-dev
libxrender-dev
```

And then, under sudo or root:


```
mkdir b
cd b
cmake ..
make
make install
```

This installs Lv2 and VSTi versions of the plugin to /usr/local/lib/LV2 and /usr/local/lib/VST3 by default. To redefine te destination dir, set another dest dir via CMAKE_INSTALL_PREFIX variable. For example, from ```/usr/local``` to ```/usr```:

```cmake -DCMAKE_INSTALL_PREFIX=/usr ..```

