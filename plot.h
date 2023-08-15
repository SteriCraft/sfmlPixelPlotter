#pragma once

#include "sharedData.h"

class Plot
{
	public:
		Plot(SharedData *data);

		void compute();
		void updatePlotSettings();

	private:
		void initData();
		void swapBuffers();
		void setPixelColor(unsigned int x, unsigned int y, sf::Color color);

		SharedData *m_data;
		Bounds m_plotBounds;
		unsigned int m_windowWidth;
		unsigned int m_windowHeight;

		sf::Uint8 *pixelsBuffer;
};
