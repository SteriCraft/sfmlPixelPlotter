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
		void setPixelColor(unsigned int x, unsigned int y, sf::Color color);

		sf::Vector2f screenToWorld(sf::Vector2i pos);
		sf::Vector2f screenToWorld(int x, int y);
		sf::Vector2i worldToScreen(sf::Vector2f pos);
		sf::Vector2i worldToScreen(float x, float y);

		static float lerp(float rangeMin, float rangeMax, float x);

		SharedData *m_data;
		Bounds m_plotBounds;
		unsigned int m_windowWidth;
		unsigned int m_windowHeight;
};
