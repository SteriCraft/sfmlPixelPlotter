#include "mainWindow.h"
#include "plot.h"

int main()
{
    SharedData data;

    Plot plot(&data);
    Display::MainWindow* win = new Display::MainWindow(&data);

    while (win->isOpen())
    {
        if (data.eventType == Event::CAMERA_MOVED)
        {
            plot.compute(); // Do you work here
        }
        else if (data.eventType == Event::WINDOW_RESIZED)
        {
            plot.updatePlotSettings();
        }
    }

    delete win;

    return 0;
}
