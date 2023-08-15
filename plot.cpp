#include <iostream>

#include "plot.h"

Plot::Plot(SharedData *data)
{
	m_data = data;

	initData();
}

void Plot::compute()
{
    updatePlotSettings();

    for (unsigned int x(0); x < m_windowWidth; x++)
    {
        for (unsigned int y(0); y < m_windowHeight; y++)
        {
            setPixelColor(x, y, sf::Color(10, 10, 10));
        }
    }
    
    swapBuffers();
}

void Plot::initData()
{
    pixelsBuffer = new sf::Uint8[DEFAULT_WIN_WIDTH * DEFAULT_WIN_HEIGHT * 4];
    m_data->pixels = new sf::Uint8[DEFAULT_WIN_WIDTH * DEFAULT_WIN_HEIGHT * 4];

    int index(0);
    for (unsigned int x(0); x < DEFAULT_WIN_WIDTH; x++)
    {
        for (unsigned int y(0); y < DEFAULT_WIN_HEIGHT; y++)
        {
            index = (x + y * DEFAULT_WIN_WIDTH) * 4;

            pixelsBuffer[index]     = 10;
            pixelsBuffer[index + 1] = 10;
            pixelsBuffer[index + 2] = 10;
            pixelsBuffer[index + 3] = 255;
        }
    }

    swapBuffers();
}

void Plot::swapBuffers()
{
    if (m_data->eventType != Event::WINDOW_RESIZED)
    {
        m_data->mutex.lock();

        int index(0);
        for (unsigned int x(0); x < m_data->windowWidth; x++)
        {
            for (unsigned int y(0); y < m_data->windowHeight; y++)
            {
                index = (x + y * m_data->windowWidth) * 4;

                m_data->pixels[index] = pixelsBuffer[index];
                m_data->pixels[index + 1] = pixelsBuffer[index + 1];
                m_data->pixels[index + 2] = pixelsBuffer[index + 2];
                m_data->pixels[index + 3] = pixelsBuffer[index + 3];
            }
        }

        m_data->mutex.unlock();

        m_data->eventType = Event::NONE;
    }
}

void Plot::updatePlotSettings()
{
    m_data->mutex.lock();

    m_data->eventType = Event::NONE;

    m_plotBounds = m_data->plotBounds;
    m_windowWidth = m_data->windowWidth;
    m_windowHeight = m_data->windowHeight;

    delete pixelsBuffer;
    pixelsBuffer = new sf::Uint8[m_windowWidth * m_windowHeight * 4];

    m_data->mutex.unlock();
}

void Plot::setPixelColor(unsigned int x, unsigned int y, sf::Color color)
{
    int index = (x + y * m_windowWidth) * 4;

    pixelsBuffer[index]     = color.r;
    pixelsBuffer[index + 1] = color.g;
    pixelsBuffer[index + 2] = color.b;
    pixelsBuffer[index + 3] = color.a;
}
