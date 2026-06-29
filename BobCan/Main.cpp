#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <spdlog/spdlog.h>

#include <chrono>
#include <string>
#include <set>

#define LOG_INFO(...) spdlog::info(__VA_ARGS__)
#define LOG_ERROR(...) spdlog::error(__VA_ARGS__)
#define LOG_WARN(...) spdlog::warn(__VA_ARGS__)

constexpr auto GAME_WIDTH = 1280;
constexpr auto GAME_HEIGHT = 720;
constexpr auto GRAVITY = 40;

enum Skill
{
	WALK = 0,
	SHIELD,
	PUNCH,
	SHOOT
};

struct Bullet
{
	sf::Vector2f position;
	sf::Vector2f direction;
	sf::RectangleShape rect;

	Bullet()
	{
		rect.setSize({ 10.0f, 10.0f });
		rect.setFillColor(sf::Color::Red);
	}
};

struct Player
{
	sf::Sprite* sprite;
	sf::Texture texture;
	std::set<Skill> currentSkills;
	std::vector<Bullet> bullets;
	sf::Vector2f currentPosition;
	bool falling;
	int hp;

	Player()
	{
		texture = sf::Texture("Resources/Bob.png");
		sprite = new sf::Sprite(texture);
		currentPosition = sf::Vector2f({ 200, 300 });
		sprite->setScale({ 2.0f, 2.0f });
		sprite->setPosition(currentPosition);
		hp = 20;
		falling = false;
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
	sf::Vector2f currentPosition;
	sf::IntRect textureRect;
	sf::Clock clock;
	int currentAnimationFrame;
	int hp;

	Boss()
	{
		texture = sf::Texture("Resources/Boss.png");
		sprite = new sf::Sprite(texture);
		currentPosition = sf::Vector2f({ 530, 260 });
		sprite->setScale({ 2.0f, 2.0f });
		textureRect = sf::IntRect({ 0, 0 }, { 128, 128 });
		sprite->setTextureRect(textureRect);
		sprite->setPosition(currentPosition);
		currentAnimationFrame = 0;
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
		sf::RectangleShape mainPlatform({ 1000.0f, 200.0f });
		mainPlatform.setPosition({
			(GAME_WIDTH / 2) - (mainPlatform.getSize().x / 2),
			GAME_HEIGHT - mainPlatform.getSize().y - 20.0f
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
	bool gameOver = false;
	sf::Font quaverPixelFont;
	sf::Text* gameOverText;

	Game()
	{
		quaverPixelFont = sf::Font("Resources/quaver.ttf");
		gameOverText = new sf::Text(quaverPixelFont, "GAME OVER", 50);
		gameOverText->setFillColor(sf::Color::Black);

		gameOverText->setPosition({
			(GAME_WIDTH / 2) - (gameOverText->getGlobalBounds().size.x / 2),
			(GAME_HEIGHT / 2) - (gameOverText->getGlobalBounds().size.y / 2),
			});
	}

	~Game()
	{
		delete gameOverText;
	}

	void update(sf::RenderWindow& window)
	{
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::A))
		{
			player.currentPosition += sf::Vector2f({ -3.0f, 0.0f });
		}
		else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::D))
		{
			player.currentPosition += sf::Vector2f({ 3.0f, 0.0f });
		}

		if (player.falling)
		{
			player.currentPosition += sf::Vector2f({ 0.0f, 3.0f });
		}

		for (const sf::RectangleShape rectangle : world.rectangles)
		{
			sf::FloatRect rectGlobalBounds = rectangle.getGlobalBounds();
			sf::FloatRect playerGlobalBounds = player.sprite->getGlobalBounds();

			if (rectGlobalBounds.findIntersection(playerGlobalBounds) == std::nullopt)
			{
				if (player.falling == false)
					player.falling = true;
				LOG_INFO("Player is falling...");
			}
			else
			{
				player.falling = false;
			}
		}

		player.sprite->setPosition(player.currentPosition);

		if (boss.clock.getElapsedTime() >= sf::milliseconds(100))
		{
			boss.currentAnimationFrame++;
			boss.textureRect.position.x += 128;
			boss.clock.restart();
		}

		if (boss.currentAnimationFrame == 13)
		{
			boss.currentAnimationFrame = 0;
			boss.textureRect.position.x = 0;
		}

		boss.sprite->setTextureRect(boss.textureRect);

		if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left) &&
			boss.sprite->getGlobalBounds().contains({ (float)sf::Mouse::getPosition(window).x, (float)sf::Mouse::getPosition(window).y }))
		{
			LOG_INFO("Killed the monster");
			gameOver = true;
		}
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

		game.update(window);

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

		if (game.gameOver)
		{
			window.draw(*game.gameOverText);
		}
		else
		{
			window.draw(background);
			window.draw(fpsText);

			for (const sf::RectangleShape& rectangle : game.world.rectangles)
			{
				window.draw(rectangle);
			}

			window.draw(*game.player.sprite);
			window.draw(*game.boss.sprite);
		}
		
		window.display();

		end = std::chrono::high_resolution_clock::now();
		fps = (float)1e9 / (float)std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
	}

	return 0;
}
