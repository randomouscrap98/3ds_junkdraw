# 3ds_junkdraw <img alt="icon" align="left" src="icon.png">

An extremely simple drawing app for the 3DS. Focus is simplicity and ease of reimplementation.

## About
This started as an attempt at implementing a multiplayer drawing app. It was my first 3DS app and I
kind of gave up on it for several years. I figured that was a waste, so I cleaned it up and 
reduced the scope. It is currently just a simple drawing app, no multiplayer.

Some design decisions are based around [bugs in citro2d](https://github.com/devkitPro/citro2d/issues/31),
I'm sorry in advance.

## Controls
<img width="250" align="right" src="screenshot1.png" alt="screenshot">

* **START** : menu
* **SELECT** : change layers
* **L** : color picker
* **A** : pencil tool
* **B** : eraser tool
* **Y** : slow pen tool
* **UP/DOWN** : zoom in/out
* **LEFT/RIGHT** : change brush width
* **R+L** : change palette (the default is the one that starts with bright red followed by black)
* **R+LEFT/RIGHT** : REALLY change brush width
* **R+UP/DOWN** : change page (can have thousands)
* **R+L+UP/DOWN** : REALLY change page (please don't do this)

## Debugging
It's difficult to debug on the 3DS. As such, if you need to inspect the drawing data at a granular
level, drawing specific ranges of strokes, you can do so using [debug.html](debug.html).

### Directions for use
- Save your drawing as you normally do within 3ds junkdraw
- Copy the raw drawing (named `raw` inside the `/3ds/junkdraw/saves/<DRAWNAME>` folder) onto your computer. 
- Load [debug.html](debug.html) on your computer (save the file then open it in your browser)
- Use `Choose file` on the debug page to load your raw drawing. You will also need to click `Load`
- Once loaded, you will be able to inspect every stroke, as well as redraw any range of strokes you desire.

You can use this to "curate" specific ranges of strokes for export, or simply export entire pages for 
whatever reason. Please note that this is a debug page first and foremost, and won't be updated with
many ease of use features.