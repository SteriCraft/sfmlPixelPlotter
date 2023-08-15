#include "mainWindow.h"
#include "plot.h"

/*
-> To draw pixel by pixel, go to plot.cpp -> compute()
-> To draw SFML objects, go to mainWindow.cpp -> update();
*/

int main()
{
    SharedData data;

    Plot plot(&data);
    Display::MainWindow* win = new Display::MainWindow(&data);

    while (win->isOpen())
    {
        if (data.eventType == Event::CAMERA_MOVED)
        {
            plot.compute();
        }
        else if (data.eventType == Event::WINDOW_RESIZED)
        {
            plot.updatePlotSettings();
        }
    }

    delete win;

    return 0;
}
