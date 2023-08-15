#include <iostream>

#include "plot.h"

Plot::Plot(SharedData *data)
{
	m_data = data;

	initData();
}

void Plot::compute()
{
    sf::Vector2f coord;

    updatePlotSettings();

    for (unsigned int x(0); x < m_windowWidth; x++)
    {
        for (unsigned int y(0); y < m_windowHeight; y++)
        {
            if (m_data->eventType == Event::CAMERA_MOVED || m_data->eventType == Event::WINDOW_RESIZED)
                return;
            
            coord = screenToWorld(x, y);

            // YOUR WORK HERE

            setPixelColor(x, y, sf::Color(10, 10, 10));
        }
    }

    m_data->eventType = Event::NONE;
}

void Plot::initData()
{
    m_data->pixels = new sf::Uint8[DEFAULT_WIN_WIDTH * DEFAULT_WIN_HEIGHT * 4];

    int index(0);
    for (unsigned int x(0); x < DEFAULT_WIN_WIDTH; x++)
    {
        for (unsigned int y(0); y < DEFAULT_WIN_HEIGHT; y++)
        {
            index = (x + y * DEFAULT_WIN_WIDTH) * 4;

            m_data->pixels[index] = 10;
            m_data->pixels[index + 1] = 10;
            m_data->pixels[index + 2] = 10;
            m_data->pixels[index + 3] = 255;
        }
    }
}

void Plot::updatePlotSettings()
{
    m_data->mutex.lock();

    m_data->eventType = Event::NONE;

    m_plotBounds = m_data->plotBounds;
    m_windowWidth = m_data->windowWidth;
    m_windowHeight = m_data->windowHeight;

    m_data->mutex.unlock();
}

void Plot::setPixelColor(unsigned int x, unsigned int y, sf::Color color)
{
    m_data->mutex.lock();

    int index = (x + y * m_windowWidth) * 4;

    m_data->pixels[index] = color.r;
    m_data->pixels[index + 1] = color.g;
    m_data->pixels[index + 2] = color.b;
    m_data->pixels[index + 3] = color.a;

    m_data->mutex.unlock();
}

sf::Vector2f Plot::screenToWorld(sf::Vector2i pos)
{
    return screenToWorld(pos.x, pos.y);
}

sf::Vector2f Plot::screenToWorld(int x, int y)
{
    sf::Vector2f coord;
    float xSpan(m_plotBounds.xMax - m_plotBounds.xMin);
    float ySpan(m_plotBounds.yMax - m_plotBounds.yMin);

    coord.x = lerp(0.f, (float)m_windowWidth, (float)x) * xSpan + m_plotBounds.xMin;
    coord.y = lerp(0.f, (float)m_windowHeight, (float)y) * ySpan + m_plotBounds.yMin;

    return coord;
}

sf::Vector2i Plot::worldToScreen(sf::Vector2f pos)
{
    return worldToScreen(pos.x, pos.y);
}

sf::Vector2i Plot::worldToScreen(float x, float y)
{
    sf::Vector2i pos;

    pos.x = (int)(lerp(m_plotBounds.xMin, m_plotBounds.xMax, x) * m_windowWidth);
    pos.y = (int)(lerp(m_plotBounds.yMin, m_plotBounds.yMax, y) * m_windowHeight);

    return pos;
}

float Plot::lerp(float rangeMin, float rangeMax, float x)
{
    return (x - rangeMin) / (rangeMax - rangeMin);
}
