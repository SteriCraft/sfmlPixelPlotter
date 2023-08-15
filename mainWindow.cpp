#include <iostream>
#include <sstream>
#include <iomanip>
#include <cmath>

#include "mainWindow.h"

using namespace Display;

MainWindow::MainWindow(SharedData *data)
{
	m_window = nullptr;
	m_status.currentState = State::OPENED;
	m_data = data;

	initDebugPanel();

	m_windowThread = new std::thread(&MainWindow::windowLoop, this);
}

MainWindow::~MainWindow()
{
	m_status.mutex.lock();
	m_status.cmd = Command::CLOSE;
	m_status.mutex.unlock();

	m_windowThread->join();

	deleteDebugPointers();
	delete m_window;
	delete m_dynamicView;
	delete m_staticView;
	delete m_windowThread;
}

bool MainWindow::isOpen()
{
	bool opened(false);

	m_status.mutex.lock();
	opened = (m_status.currentState == State::OPENED);
	m_status.mutex.unlock();

	return opened;
}

// PRIVATE
void MainWindow::windowLoop()
{
	m_staticView = new sf::View(sf::FloatRect(0.f, 0.f, DEFAULT_WIN_WIDTH, DEFAULT_WIN_HEIGHT));

	m_zoom = DEFAULT_ZOOM;
	m_cameraPosition = DEFAULT_CAMERA_POSITION;
	m_dynamicView = new sf::View(m_cameraPosition, sf::Vector2f(DEFAULT_WIN_WIDTH, DEFAULT_WIN_HEIGHT));
	m_dynamicView->zoom(m_zoom);
	getBounds();

	m_window = new sf::RenderWindow(sf::VideoMode(DEFAULT_WIN_WIDTH, DEFAULT_WIN_HEIGHT), "SFML Plot");

	m_showDebug = true;

	m_framerate = 0.f;
	m_timePoint = m_chrono.getElapsedTime();
	while (m_window->isOpen())
	{
		if (timeForNextFrame())
		{
			checkCommands();

			handleEvents();

			update();
		}
	}
}

bool MainWindow::timeForNextFrame()
{
	int duration(m_chrono.getElapsedTime().asMilliseconds() - m_timePoint.asMilliseconds());

	if (duration >= 1000.f / FPS_TARGET)
	{
		m_framerate = 1000.f / duration;

		m_timePoint = m_chrono.getElapsedTime();
		return true;
	}

	return false;
}

void MainWindow::handleEvents()
{
	sf::Event event;
	while (m_window->pollEvent(event))
	{
		if (event.type == sf::Event::Closed)
		{
			m_status.currentState = State::CLOSED;
			m_window->close();
		}
		else if (event.type == sf::Event::KeyPressed)
		{
			if (event.key.code == sf::Keyboard::F3)
			{
				m_showDebug = !m_showDebug;
			}
			else if (event.key.code == sf::Keyboard::Escape)
			{
				m_status.currentState = State::CLOSED;
				m_window->close();
			}
			else if (event.key.code == sf::Keyboard::G)
			{
				m_grid.show = !m_grid.show;
			}
		}
		else if (event.type == sf::Event::MouseButtonPressed)
		{
			if (event.mouseButton.button == sf::Mouse::Left)
			{
				m_moving = true;
				m_lastMousePosition = sf::Mouse::getPosition(*m_window);
			}
		}
		else if (event.type == sf::Event::MouseButtonReleased)
		{
			if (event.mouseButton.button == sf::Mouse::Left)
			{
				m_moving = false;
			}
		}
		else if (event.type == sf::Event::MouseMoved)
		{
			if (m_moving)
			{
				sf::Vector2i currentMousePosition = sf::Mouse::getPosition(*m_window);
				sf::Vector2i delta = currentMousePosition - m_lastMousePosition;

				m_cameraPosition = m_window->mapPixelToCoords(m_window->mapCoordsToPixel(m_cameraPosition, *m_dynamicView) - delta, *m_dynamicView);

				getBounds();

				m_dynamicView->setCenter(m_cameraPosition);
				m_window->setView(*m_dynamicView);

				m_data->eventType = Event::CAMERA_MOVED;
				m_lastMousePosition = currentMousePosition;
			}
		}
		else if (event.type == sf::Event::MouseWheelScrolled)
		{
			m_data->eventType = Event::CAMERA_MOVED;

			if (event.mouseWheelScroll.delta < 0)
			{
				m_dynamicView->zoom(1.1f);
				m_zoom *= 1.1f;
			}
			else
			{
				m_dynamicView->zoom(1.f / 1.1f);
				m_zoom /= 1.1f;
			}

			getBounds();

			m_window->setView(*m_dynamicView);
		}
		else if (event.type == sf::Event::Resized)
		{
			m_data->eventType = Event::WINDOW_RESIZED;

			m_data->mutex.lock();
			m_data->windowWidth = event.size.width;
			m_data->windowHeight = event.size.height;
			resizePixelData();
			m_data->mutex.unlock();

			getBounds();

			m_dynamicView->setCenter(m_cameraPosition);
			m_dynamicView->setSize((float)m_data->windowWidth, (float)m_data->windowHeight);
			m_dynamicView->zoom(m_zoom);

			m_staticView->reset(sf::FloatRect(0.f, 0.f, (float)m_data->windowWidth, (float)m_data->windowHeight));
		}
	}
}

void MainWindow::checkCommands()
{
	m_status.mutex.lock();

	if (m_status.cmd == Command::CLOSE)
	{
		m_status.cmd = Command::NONE;
		m_status.currentState = State::CLOSED;

		m_window->close();
	}

	m_status.mutex.unlock();
}

void MainWindow::update()
{
	m_window->clear();

	// Pixel data is drawn in the referential of the window
	updateScreenBuffer();
	
	// ========= ELEMENTS IN SFML REFERENTIAL =========
	//m_window->setView(*m_dynamicView);
	//
	// Draw them here
	// ================================================

	if (m_grid.show)
	{
		computeGrid();
		createGridLabels();
		drawGrid();
	}

	if (m_showDebug)
		drawDebugInfo();

	m_window->display();
}

void MainWindow::updateScreenBuffer()
{
	m_window->setView(*m_staticView);

	m_screenImage.create(m_data->windowWidth, m_data->windowHeight, m_data->pixels);
	m_screenTexture.loadFromImage(m_screenImage);
	m_screenSprite.setTexture(m_screenTexture);

	m_window->draw(m_screenSprite);
}

void MainWindow::computeGrid()
{
	float step;
	float gridXMin(m_data->plotBounds.xMin), gridXMax(m_data->plotBounds.xMax);
	float gridYMin(m_data->plotBounds.yMin), gridYMax(m_data->plotBounds.yMax);

	Line line;

	step = log10f(m_dynamicView->getSize().x);
	step = roundf(step);
	step--;
	step = pow(10.f, step);

	m_grid.m_abscissLines.clear();
	m_grid.m_ordinateLines.clear();

	if (step > 0.0001f)
	{
		if (gridXMin < 0.f)
		{
			gridXMin += fabsf(fmodf(gridXMin, step)) - step;
		}
		else
		{
			gridXMin -= fabsf(fmodf(gridXMin, step));
		}

		if (gridYMin < 0.f)
		{
			gridYMin += fabsf(fmodf(gridYMin, step)) - step;
		}
		else
		{
			gridYMin -= fabsf(fmodf(gridYMin, step));
		}

		if (gridXMax < 0.f)
		{
			gridXMax += fabsf(fmodf(gridXMax, step));
		}
		else
		{
			gridXMax -= fabsf(fmodf(gridXMax, step)) - step;
		}

		if (gridYMax < 0.f)
		{
			gridYMax += fabsf(fmodf(gridYMax, step));
		}
		else
		{
			gridYMax -= fabsf(fmodf(gridYMax, step)) - step;
		}

		for (float x(gridXMin); x <= gridXMax; x += step)
		{
			line.plotPos = x;
			line.pos = m_window->mapCoordsToPixel(sf::Vector2f(line.plotPos, 0.f), *m_dynamicView).x;

			if (fabs(x) <= 0.1f * step)
				line.type = LineType::ABSOLUTE_MAIN;
			else if (fmodf(fabsf(x), step * 10.f) < step * 0.5f)
				line.type = LineType::MAIN;
			else
				line.type = LineType::NORMAL;

			m_grid.m_ordinateLines.push_back(line);
		}

		for (float y(gridYMin); y <= gridYMax; y += step)
		{
			line.plotPos = y;
			line.pos = m_window->mapCoordsToPixel(sf::Vector2f(0.f, line.plotPos), *m_dynamicView).y;

			if (fabs(y) <= 0.1f * step)
				line.type = LineType::ABSOLUTE_MAIN;
			else if (fmodf(fabsf(y), step * 10.f) < step * 0.5f)
				line.type = LineType::MAIN;
			else
				line.type = LineType::NORMAL;

			m_grid.m_abscissLines.push_back(line);
		}
	}
}

void MainWindow::createGridLabels()
{
	for (unsigned int i(0); i < m_grid.m_abscissLines.size(); i++)
	{
		m_grid.m_abscissLines[i].valueLabelTxt.setFont(*m_debugPanel.font);
		m_grid.m_abscissLines[i].valueLabelTxt.setCharacterSize(DEBUG_FONT_SIZE);
		m_grid.m_abscissLines[i].valueLabelTxt.setString(decimal2str(m_grid.m_abscissLines[i].plotPos, 3));
		m_grid.m_abscissLines[i].valueLabelTxt.setPosition(0.f, (float)m_grid.m_abscissLines[i].pos);
	}

	for (unsigned int i(0); i < m_grid.m_ordinateLines.size(); i++)
	{
		m_grid.m_ordinateLines[i].valueLabelTxt.setFont(*m_debugPanel.font);
		m_grid.m_ordinateLines[i].valueLabelTxt.setCharacterSize(DEBUG_FONT_SIZE);
		m_grid.m_ordinateLines[i].valueLabelTxt.setString(decimal2str(m_grid.m_ordinateLines[i].plotPos, 3));
		m_grid.m_ordinateLines[i].valueLabelTxt.setPosition((float)m_grid.m_ordinateLines[i].pos, 0.f);
	}
}

void MainWindow::drawGrid()
{
	m_window->setView(*m_staticView);

	sf::Color normalColor =	{  90,  90,  90 };
	sf::Color mainColor =	{ 150, 150, 150 };

	for (unsigned int i(0); i < m_grid.m_abscissLines.size(); i++)
	{
		float posY = (float)m_grid.m_abscissLines[i].pos;

		if (m_grid.m_abscissLines[i].type == LineType::ABSOLUTE_MAIN)
		{
			sf::RectangleShape line(sf::Vector2f((float)m_data->windowWidth, 2));

			line.setPosition(0, posY);
			line.setFillColor(mainColor);

			m_window->draw(line);
			m_window->draw(m_grid.m_abscissLines[i].valueLabelTxt);
		}
		else if (m_grid.m_abscissLines[i].type == LineType::MAIN)
		{
			sf::Vertex line[] =
			{
				sf::Vertex(sf::Vector2f(0,							posY), mainColor),
				sf::Vertex(sf::Vector2f((float)m_data->windowWidth, posY), mainColor)
			};

			m_window->draw(line, 2, sf::Lines);
			m_window->draw(m_grid.m_abscissLines[i].valueLabelTxt);
		}
		else
		{
			sf::Vertex line[] =
			{
				sf::Vertex(sf::Vector2f(0,							posY), normalColor),
				sf::Vertex(sf::Vector2f((float)m_data->windowWidth, posY), normalColor)
			};

			m_window->draw(line, 2, sf::Lines);
			m_window->draw(m_grid.m_abscissLines[i].valueLabelTxt);
		}
	}

	for (unsigned int i(0); i < m_grid.m_ordinateLines.size(); i++)
	{
		float posX = (float)m_grid.m_ordinateLines[i].pos;

		if (m_grid.m_ordinateLines[i].type == LineType::ABSOLUTE_MAIN)
		{
			sf::RectangleShape line(sf::Vector2f((float)m_data->windowHeight, 2));

			line.setPosition(posX, 0);
			line.setFillColor(mainColor);
			line.rotate(90);

			m_window->draw(line);
			m_window->draw(m_grid.m_ordinateLines[i].valueLabelTxt);
		}
		else if (m_grid.m_ordinateLines[i].type == LineType::MAIN)
		{
			sf::Vertex line[] =
			{
				sf::Vertex(sf::Vector2f(posX, 0),							mainColor),
				sf::Vertex(sf::Vector2f(posX, (float)m_data->windowHeight), mainColor)
			};

			m_window->draw(line, 2, sf::Lines);
			m_window->draw(m_grid.m_ordinateLines[i].valueLabelTxt);
		}
		else
		{
			sf::Vertex line[] =
			{
				sf::Vertex(sf::Vector2f(posX, 0),							normalColor),
				sf::Vertex(sf::Vector2f(posX, (float)m_data->windowHeight), normalColor)
			};

			m_window->draw(line, 2, sf::Lines);
			m_window->draw(m_grid.m_ordinateLines[i].valueLabelTxt);
		}
	}
}

void MainWindow::initDebugPanel()
{
	m_debugPanel.font = new sf::Font();
	m_debugPanel.font->loadFromFile("consola.ttf");

	m_debugPanel.fpsTxt = new sf::Text();
	m_debugPanel.fpsTxt->setFont(*m_debugPanel.font);
	m_debugPanel.fpsTxt->setCharacterSize(DEBUG_FONT_SIZE);

	m_debugPanel.mousePlotPositionTxt = new sf::Text();
	m_debugPanel.mousePlotPositionTxt->setFont(*m_debugPanel.font);
	m_debugPanel.mousePlotPositionTxt->setCharacterSize(DEBUG_FONT_SIZE);

	m_debugPanel.plotBoundsTxt = new sf::Text();
	m_debugPanel.plotBoundsTxt->setFont(*m_debugPanel.font);
	m_debugPanel.plotBoundsTxt->setCharacterSize(DEBUG_FONT_SIZE);

	m_debugPanel.cameraPosTxt = new sf::Text();
	m_debugPanel.cameraPosTxt->setFont(*m_debugPanel.font);
	m_debugPanel.cameraPosTxt->setCharacterSize(DEBUG_FONT_SIZE);

	m_debugPanel.zoomTxt = new sf::Text();
	m_debugPanel.zoomTxt->setFont(*m_debugPanel.font);
	m_debugPanel.zoomTxt->setCharacterSize(DEBUG_FONT_SIZE);

	m_debugPanel.helpTxt = new sf::Text();
	m_debugPanel.helpTxt->setFont(*m_debugPanel.font);
	m_debugPanel.helpTxt->setCharacterSize(DEBUG_FONT_SIZE);
}

void MainWindow::drawDebugInfo()
{
	m_window->setView(*m_staticView);

	float lastPosY(0.f);

	// Update sf::Text
	m_debugPanel.fpsTxt->setString("FPS: " + decimal2str(m_framerate));
	lastPosY += m_debugPanel.fpsTxt->getGlobalBounds().height + DEBUG_SPACING;

	std::string plotBoundsStr("-- Plot bounds --\n");
	plotBoundsStr += "X: [" + std::to_string(m_data->plotBounds.xMin) + " ; " + std::to_string(m_data->plotBounds.xMax) + "]\n";
	plotBoundsStr += "Y: [" + std::to_string(m_data->plotBounds.yMin) + " ; " + std::to_string(m_data->plotBounds.yMax) + "]";
	m_debugPanel.plotBoundsTxt->setString(plotBoundsStr);
	m_debugPanel.plotBoundsTxt->setPosition(0, lastPosY + 5 * DEBUG_SPACING);
	lastPosY += m_debugPanel.plotBoundsTxt->getGlobalBounds().height + 6 * DEBUG_SPACING;

	sf::Vector2f mousePlotPosition(m_window->mapPixelToCoords(sf::Mouse::getPosition(*m_window), *m_dynamicView));
	m_debugPanel.mousePlotPositionTxt->setString("Mouse plot position: " + std::to_string(mousePlotPosition.x) + " ; " + std::to_string(mousePlotPosition.y));
	m_debugPanel.mousePlotPositionTxt->setPosition(0, lastPosY);
	lastPosY += m_debugPanel.mousePlotPositionTxt->getGlobalBounds().height + DEBUG_SPACING;

	m_debugPanel.cameraPosTxt->setString("Camera position: " + std::to_string(m_cameraPosition.x) + " ; " + std::to_string(m_cameraPosition.y));
	m_debugPanel.cameraPosTxt->setPosition(0, lastPosY);
	lastPosY += m_debugPanel.cameraPosTxt->getGlobalBounds().height + DEBUG_SPACING;

	m_debugPanel.zoomTxt->setString("Zoom: " + std::to_string(m_zoom));
	m_debugPanel.zoomTxt->setPosition(0, lastPosY);
	lastPosY += m_debugPanel.zoomTxt->getGlobalBounds().height + DEBUG_SPACING;

	m_debugPanel.helpTxt->setString("Debug panel: F3\nGrid: G\nMove: Mouse left click\nZoom: Mouse scrollwheel");
	m_debugPanel.helpTxt->setPosition(0, lastPosY + 5 * DEBUG_SPACING);
	lastPosY += m_debugPanel.helpTxt->getGlobalBounds().height + 6 * DEBUG_SPACING;


	// Draw sf::Text
	m_window->draw(*m_debugPanel.fpsTxt);
	m_window->draw(*m_debugPanel.plotBoundsTxt);
	m_window->draw(*m_debugPanel.mousePlotPositionTxt);
	m_window->draw(*m_debugPanel.cameraPosTxt);
	m_window->draw(*m_debugPanel.zoomTxt);
	m_window->draw(*m_debugPanel.helpTxt);
}

void MainWindow::deleteDebugPointers()
{
	delete m_debugPanel.font;
	delete m_debugPanel.fpsTxt;
	delete m_debugPanel.mousePlotPositionTxt;
	delete m_debugPanel.plotBoundsTxt;
	delete m_debugPanel.cameraPosTxt;
	delete m_debugPanel.zoomTxt;
	delete m_debugPanel.helpTxt;
}

void MainWindow::getBounds()
{
	m_data->plotBounds.xMin = m_cameraPosition.x - m_dynamicView->getSize().x / 2;
	m_data->plotBounds.xMax = m_cameraPosition.x + m_dynamicView->getSize().x / 2;

	m_data->plotBounds.yMin = m_cameraPosition.y - m_dynamicView->getSize().y / 2;
	m_data->plotBounds.yMax = m_cameraPosition.y + m_dynamicView->getSize().y / 2;
}

void MainWindow::resizePixelData()
{
	delete m_data->pixels;
	m_data->pixels = new sf::Uint8[m_data->windowWidth * m_data->windowHeight * 4];
}

std::string MainWindow::decimal2str(float value, unsigned int precision)
{
	std::stringstream stream;

	stream << std::fixed << std::setprecision(precision) << value;

	return stream.str();
}
