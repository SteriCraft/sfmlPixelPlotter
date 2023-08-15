# sfmlPixelPlotter
An SFML based plotter with a separate thread for drawing, using the GPU to draw pixel by pixel on screen.

## Compiling on Windows
To compile this template, you need a include the SFML library.
Follow this tutorial to adapt it depending on your system and IDE:
https://www.sfml-dev.org/tutorials/2.6/

## Use
MainWindow class is a separated thread handling the drawing.
You can modify it, and use the method "update()" to draw your SFML objects.

To compute pixel by pixel, use the Plot class and its method "computer()".
It will then send the pixel buffer to the MainWindow and draw it for you.