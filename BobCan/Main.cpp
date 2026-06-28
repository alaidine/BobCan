#include <SFML/Graphics.hpp>
#include <spdlog/spdlog.h>

#include <chrono>
#include <string>
#include <set>

#define LOG_INFO(...) spdlog::info(__VA_ARGS__)
#define LOG_ERROR(...) spdlog::error(__VA_ARGS__)
#define LOG_WARN(...) spdlog::warn(__VA_ARGS__)

constexpr auto GAME_WIDTH = 1280;
constexpr auto GAME_HEIGHT = 720;

enum Skills
{
	WALK = 0,
	SHIELD,
	PUNCH,
	SHOOT
};

struct Player
{
	sf::Sprite* sprite;
	sf::Texture texture;
	std::set<Skills> currentSkills;
	int hp;

	Player()
	{
		texture = sf::Texture("Resources/Bob.png");
		sprite = new sf::Sprite(texture);
		sprite->setScale({ 2.0f, 2.0f });
		sprite->setPosition({ 100, 300 });
		hp = 20;
	}

	~Player()
	{
		delete sprite;
	}
};

struct Boss
{
	sf::Sprite* sprite;
	sf::Texture texture;
	int hp;

	Boss()
	{
		texture = sf::Texture("Resources/Boss.png");
		sprite = new sf::Sprite(texture);
		sprite->setScale({ 2.0f, 2.0f });
		sprite->setTextureRect(sf::IntRect({ 0, 0 }, { 128, 128 }));
		sprite->setPosition({ 400, 300 });
		hp = 50;
	}

	~Boss()
	{
		delete sprite;
	}
};

struct World
{
	std::vector<sf::RectangleShape> rectangles;

	World()
	{
		sf::RectangleShape mainPlatform({ 20.0f, 30.0f });
		mainPlatform.setPosition({
			(GAME_WIDTH / 2) - (mainPlatform.getSize().x / 2),
			(GAME_HEIGHT / 2) - (mainPlatform.getSize().y / 2)
		});
		mainPlatform.setFillColor(sf::Color::Black);

		rectangles.push_back(mainPlatform);
	}
};

struct Game
{
	World world;
	Player player;
	Boss boss;

	void update()
	{

	}
};

char lore[] =
	"This is the lore";

int main(void)
{
	sf::RenderWindow window{ sf::VideoMode({ GAME_WIDTH, GAME_HEIGHT }), "Bob can" };
	const sf::Font quaverPixelFont("Resources/quaver.ttf");
	sf::Text testText(quaverPixelFont, "Bob can walk", 50);
	sf::Text fpsText(quaverPixelFont);
	sf::Texture backgroundTexture("Resources/Background.png");
	sf::Sprite background(backgroundTexture);
	sf::Clock clock;
	Game game;
	
	std::chrono::high_resolution_clock::time_point start;
	std::chrono::high_resolution_clock::time_point end;
	float fps = 0;

	LOG_INFO("Created a Window with resolution: 1280x720");
	LOG_INFO("Loaded font: Pixel Font Quaver");

	fpsText.setFillColor(sf::Color::Black);
	fpsText.setScale({ .7f, .7f });
	fpsText.setPosition({ 5, 5 });
	fpsText.setString("0 fps");

	window.setFramerateLimit(60);

	LOG_INFO("Framerate limit set at 60 fps");

	// Game loop
	while (window.isOpen())
	{
		while (const std::optional event = window.pollEvent())
		{
			if (event->is<sf::Event::Closed>() ||
				(event->is<sf::Event::KeyPressed>() &&
				 event->getIf<sf::Event::KeyPressed>()->code == sf::Keyboard::Key::Escape))
			{
				window.close();
			}
		}

		game.update();

#ifdef _DEBUG
		if (clock.getElapsedTime() >= sf::seconds(0.25f))
		{
			fpsText.setString(fmt::format("{} fps", (int)fps));
			clock.restart();
		}
#endif

		start = std::chrono::high_resolution_clock::now();

		// Draw
		window.clear(sf::Color::White);

		window.draw(background);
		window.draw(fpsText);

		for (const sf::RectangleShape& rectangle : game.world.rectangles)
		{
			window.draw(rectangle);
		}

		window.draw(*game.player.sprite);
		window.draw(*game.boss.sprite);

		window.display();

		end = std::chrono::high_resolution_clock::now();
		fps = (float)1e9 / (float)std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
	}

	return 0;
}
