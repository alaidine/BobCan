#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <spdlog/spdlog.h>

#include <string>
#include <set>
#include <vector>
#include <cmath>
#include <optional>

#define LOG_INFO(...) spdlog::info(__VA_ARGS__)
#define LOG_ERROR(...) spdlog::error(__VA_ARGS__)
#define LOG_WARN(...) spdlog::warn(__VA_ARGS__)

constexpr auto GAME_WIDTH = 1280;
constexpr auto GAME_HEIGHT = 720;

// Physique (valeurs par frame, fenetre limitee a 60 fps).
constexpr float MOVE_SPEED = 4.0f;
constexpr float GRAVITY = 0.8f;
constexpr float JUMP_FORCE = -15.0f;
constexpr float GROUND_Y = 500.0f;     // Sommet de la plateforme principale.
constexpr float PLAYER_SIZE = 64.0f;   // Sprite Bob 32px * scale 2.

// Combat.
constexpr int PLAYER_MAX_HP = 20;
constexpr int BOSS_MAX_HP = 60;
constexpr int PLAYER_BULLET_DMG = 5;
constexpr int PUNCH_DMG = 4;
constexpr int BOSS_BULLET_DMG = 4;
constexpr float PLAYER_BULLET_SPEED = 12.0f;
constexpr float BOSS_BULLET_SPEED = 6.0f;

enum Skill
{
	WALK = 0,
	JUMP,
	SHIELD,
	PUNCH,
	SHOOT
};

enum class GameState
{
	Intro,
	Playing,
	SkillTree,
	Victory
};

static sf::Vector2f normalize(sf::Vector2f v)
{
	float len = std::sqrt(v.x * v.x + v.y * v.y);
	if (len == 0.0f)
		return { 0.0f, 0.0f };
	return { v.x / len, v.y / len };
}

struct Bullet
{
	sf::Vector2f position;
	sf::Vector2f direction;
	float speed;
	sf::RectangleShape rect;

	Bullet(sf::Vector2f pos, sf::Vector2f dir, float spd, sf::Color color)
		: position(pos), direction(dir), speed(spd)
	{
		rect.setSize({ 10.0f, 10.0f });
		rect.setFillColor(color);
		rect.setPosition(position);
	}
};

struct Player
{
	sf::Sprite* sprite;
	sf::Texture texture;
	std::set<Skill> currentSkills;
	std::vector<Bullet> bullets;
	sf::Vector2f currentPosition;
	float velocityY;
	bool onGround;
	bool shielding;
	int hp;
	sf::Clock punchClock;

	Player()
	{
		texture = sf::Texture("Resources/Bob.png");
		sprite = new sf::Sprite(texture);
		sprite->setScale({ 2.0f, 2.0f });
		currentSkills.insert(WALK); // Bob peut toujours marcher.
		reset();
	}

	~Player()
	{
		delete sprite;
	}

	// Remet Bob en vie au point de depart sans toucher aux competences acquises.
	void reset()
	{
		currentPosition = sf::Vector2f({ 200.0f, 300.0f });
		sprite->setPosition(currentPosition);
		velocityY = 0.0f;
		onGround = false;
		shielding = false;
		hp = PLAYER_MAX_HP;
		bullets.clear();
	}

	bool has(Skill s) const { return currentSkills.count(s) > 0; }
};

struct Boss
{
	sf::Sprite* sprite;
	sf::Texture texture;
	sf::Vector2f currentPosition;
	sf::IntRect textureRect;
	sf::Clock animClock;
	sf::Clock shootClock;
	std::vector<Bullet> bullets;
	int currentAnimationFrame;
	int hp;

	Boss()
	{
		texture = sf::Texture("Resources/Boss.png");
		sprite = new sf::Sprite(texture);
		currentPosition = sf::Vector2f({ 530.0f, 260.0f });
		sprite->setScale({ 2.0f, 2.0f });
		textureRect = sf::IntRect({ 0, 0 }, { 128, 128 });
		sprite->setTextureRect(textureRect);
		sprite->setPosition(currentPosition);
		currentAnimationFrame = 0;
		reset();
	}

	~Boss()
	{
		delete sprite;
	}

	void reset()
	{
		hp = BOSS_MAX_HP;
		bullets.clear();
		shootClock.restart();
	}

	sf::Vector2f center() const
	{
		return currentPosition + sf::Vector2f({ 128.0f, 128.0f }); // 256px / 2.
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

// Un noeud de l'arbre de competences affiche a l'ecran.
struct SkillNode
{
	Skill id;
	std::string name;
	Skill prereq;    // Competence requise pour debloquer celle-ci.
	bool isRoot;     // WALK : deja acquise, sert de racine.
	sf::Vector2f pos; // Coin haut-gauche du noeud a l'ecran.
};

struct Game
{
	World world;
	Player player;
	Boss boss;
	GameState state = GameState::Intro;
	int skillPoints = 0;
	int selectedNode = 1;

	sf::Font font;
	std::vector<SkillNode> nodes;

	Game()
	{
		font = sf::Font("Resources/quaver.ttf");

		// Arbre ramifie :
		//          [WALK]
		//          /    \
		//      [JUMP]   [PUNCH]
		//        |         |
		//    [SHIELD]    [SHOOT]
		nodes = {
			{ WALK,   "WALK",   WALK,  true,  { 580.0f, 130.0f } },
			{ JUMP,   "JUMP",   WALK,  false, { 430.0f, 270.0f } },
			{ SHIELD, "SHIELD", JUMP,  false, { 430.0f, 400.0f } },
			{ PUNCH,  "PUNCH",  WALK,  false, { 730.0f, 270.0f } },
			{ SHOOT,  "SHOOT",  PUNCH, false, { 730.0f, 400.0f } },
		};
	}

	sf::Vector2f playerCenter() const
	{
		return player.currentPosition + sf::Vector2f({ PLAYER_SIZE / 2.0f, PLAYER_SIZE / 2.0f });
	}

	const SkillNode& nodeById(Skill id) const
	{
		for (const SkillNode& n : nodes)
			if (n.id == id)
				return n;
		return nodes[0];
	}

	bool nodeUnlocked(const SkillNode& n) const { return player.has(n.id); }

	// Debloquable si non racine, pas encore acquise et son prerequis est acquis.
	bool nodeAvailable(const SkillNode& n) const
	{
		if (n.isRoot || player.has(n.id))
			return false;
		return player.has(n.prereq);
	}

	void updatePlaying(sf::RenderWindow& window)
	{
		// Deplacement horizontal (WALK est toujours acquise).
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::A))
			player.currentPosition.x -= MOVE_SPEED;
		else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::D))
			player.currentPosition.x += MOVE_SPEED;

		// Bouclier maintenu (competence SHIELD).
		player.shielding = player.has(SHIELD) &&
			sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::LShift);

		// Gravite et atterrissage sur le sol invisible au niveau de la plateforme.
		player.velocityY += GRAVITY;
		player.currentPosition.y += player.velocityY;

		float feet = player.currentPosition.y + PLAYER_SIZE;
		if (feet >= GROUND_Y)
		{
			player.currentPosition.y = GROUND_Y - PLAYER_SIZE;
			player.velocityY = 0.0f;
			player.onGround = true;
		}
		else
		{
			player.onGround = false;
		}

		// Garder Bob dans l'ecran.
		if (player.currentPosition.x < 0.0f)
			player.currentPosition.x = 0.0f;
		if (player.currentPosition.x > GAME_WIDTH - PLAYER_SIZE)
			player.currentPosition.x = GAME_WIDTH - PLAYER_SIZE;

		player.sprite->setPosition(player.currentPosition);

		// Animation du boss.
		if (boss.animClock.getElapsedTime() >= sf::milliseconds(100))
		{
			boss.currentAnimationFrame++;
			boss.textureRect.position.x += 128;
			boss.animClock.restart();
		}
		if (boss.currentAnimationFrame > 13)
		{
			boss.currentAnimationFrame = 0;
			boss.textureRect.position.x = 0;
		}
		boss.sprite->setTextureRect(boss.textureRect);

		// Le boss tire regulierement vers Bob.
		if (boss.shootClock.getElapsedTime() >= sf::milliseconds(1200))
		{
			sf::Vector2f dir = normalize(playerCenter() - boss.center());
			boss.bullets.emplace_back(boss.center(), dir, BOSS_BULLET_SPEED, sf::Color::Magenta);
			boss.shootClock.restart();
		}

		updateBullets();
		checkVictoryAndDeath();
	}

	void updateBullets()
	{
		for (auto it = player.bullets.begin(); it != player.bullets.end(); )
		{
			it->position += it->direction * it->speed;
			it->rect.setPosition(it->position);

			bool remove = false;
			if (boss.hp > 0 &&
				boss.sprite->getGlobalBounds().findIntersection(it->rect.getGlobalBounds()) != std::nullopt)
			{
				boss.hp -= PLAYER_BULLET_DMG;
				remove = true;
				LOG_INFO("Boss touche ! HP boss: {}", boss.hp);
			}
			if (it->position.x < -20 || it->position.x > GAME_WIDTH + 20 ||
				it->position.y < -20 || it->position.y > GAME_HEIGHT + 20)
				remove = true;

			it = remove ? player.bullets.erase(it) : it + 1;
		}

		for (auto it = boss.bullets.begin(); it != boss.bullets.end(); )
		{
			it->position += it->direction * it->speed;
			it->rect.setPosition(it->position);

			bool remove = false;
			if (player.sprite->getGlobalBounds().findIntersection(it->rect.getGlobalBounds()) != std::nullopt)
			{
				if (!player.shielding)
				{
					player.hp -= BOSS_BULLET_DMG;
					LOG_INFO("Bob touche ! HP: {}", player.hp);
				}
				remove = true;
			}
			if (it->position.x < -20 || it->position.x > GAME_WIDTH + 20 ||
				it->position.y < -20 || it->position.y > GAME_HEIGHT + 20)
				remove = true;

			it = remove ? boss.bullets.erase(it) : it + 1;
		}
	}

	void checkVictoryAndDeath()
	{
		if (boss.hp <= 0)
		{
			LOG_INFO("L'alien est vaincu !");
			state = GameState::Victory;
			return;
		}
		if (player.hp <= 0)
			die();
	}

	void die()
	{
		skillPoints++;
		LOG_INFO("Bob est tombe. Points de competence disponibles: {}", skillPoints);
		player.reset();
		boss.reset();
		selectFirstAvailableNode();
		state = GameState::SkillTree;
	}

	void tryPunch()
	{
		if (boss.hp <= 0)
			return;
		if (player.punchClock.getElapsedTime() < sf::milliseconds(400))
			return;
		if (boss.sprite->getGlobalBounds().findIntersection(player.sprite->getGlobalBounds()) != std::nullopt)
		{
			boss.hp -= PUNCH_DMG;
			player.punchClock.restart();
			LOG_INFO("Punch ! HP boss: {}", boss.hp);
		}
	}

	void selectFirstAvailableNode()
	{
		for (int i = 0; i < (int)nodes.size(); i++)
		{
			if (nodeAvailable(nodes[i]))
			{
				selectedNode = i;
				return;
			}
		}
	}

	void tryUnlockSelected()
	{
		if (skillPoints <= 0)
			return;
		const SkillNode& n = nodes[selectedNode];
		if (!nodeAvailable(n))
			return;
		player.currentSkills.insert(n.id);
		skillPoints--;
		LOG_INFO("Competence debloquee: {}", n.name);
	}

	void restart()
	{
		player.currentSkills.clear();
		player.currentSkills.insert(WALK);
		player.reset();
		boss.reset();
		skillPoints = 0;
		selectedNode = 1;
		state = GameState::Intro;
	}

	void onKeyPressed(sf::Keyboard::Key key, sf::RenderWindow& window)
	{
		switch (state)
		{
		case GameState::Intro:
			if (key == sf::Keyboard::Key::Enter)
				state = GameState::Playing;
			break;

		case GameState::Playing:
			if (key == sf::Keyboard::Key::Space && player.has(JUMP) && player.onGround)
			{
				player.velocityY = JUMP_FORCE;
				player.onGround = false;
			}
			else if (key == sf::Keyboard::Key::F && player.has(PUNCH))
			{
				tryPunch();
			}
			break;

		case GameState::SkillTree:
			if (key == sf::Keyboard::Key::Left)
				selectedNode = (selectedNode + (int)nodes.size() - 1) % (int)nodes.size();
			else if (key == sf::Keyboard::Key::Right)
				selectedNode = (selectedNode + 1) % (int)nodes.size();
			else if (key == sf::Keyboard::Key::Enter)
				tryUnlockSelected();
			else if (key == sf::Keyboard::Key::Space)
				state = GameState::Playing;
			break;

		case GameState::Victory:
			if (key == sf::Keyboard::Key::Enter)
				restart();
			break;
		}
	}

	void onMousePressed(sf::RenderWindow& window)
	{
		if (state != GameState::Playing || !player.has(SHOOT))
			return;
		sf::Vector2i m = sf::Mouse::getPosition(window);
		sf::Vector2f dir = normalize(sf::Vector2f({ (float)m.x, (float)m.y }) - playerCenter());
		if (dir.x == 0.0f && dir.y == 0.0f)
			dir = { 1.0f, 0.0f };
		player.bullets.emplace_back(playerCenter(), dir, PLAYER_BULLET_SPEED, sf::Color::Red);
	}

	void centerX(sf::Text& t, float y)
	{
		float w = t.getGlobalBounds().size.x;
		t.setPosition({ (GAME_WIDTH - w) / 2.0f, y });
	}

	void drawIntro(sf::RenderWindow& window)
	{
		sf::Text title(font, "BobCan", 90);
		title.setFillColor(sf::Color::Black);
		centerX(title, 70.0f);
		window.draw(title);

		sf::Text lore(font,
			"After a long day at work, Bob steps outside...\n"
			"and walks straight into an alien invasion.\n\n"
			"At first he can only walk. But each time he falls,\n"
			"he rises again - stronger, with new skills to learn.\n\n"
			"Grow your skill tree. Defeat the alien. Go home.", 28);
		lore.setFillColor(sf::Color::Black);
		centerX(lore, 230.0f);
		window.draw(lore);

		sf::Text prompt(font, "Press ENTER to begin", 34);
		prompt.setFillColor(sf::Color(90, 90, 90));
		centerX(prompt, 610.0f);
		window.draw(prompt);
	}

	void drawVictory(sf::RenderWindow& window)
	{
		sf::Text title(font, "VICTORY", 90);
		title.setFillColor(sf::Color::Black);
		centerX(title, 170.0f);
		window.draw(title);

		sf::Text msg(font,
			"The alien is defeated.\nBob can finally go home.", 36);
		msg.setFillColor(sf::Color::Black);
		centerX(msg, 330.0f);
		window.draw(msg);

		sf::Text prompt(font, "Press ENTER to play again", 30);
		prompt.setFillColor(sf::Color(90, 90, 90));
		centerX(prompt, 520.0f);
		window.draw(prompt);
	}

	void drawSkillTree(sf::RenderWindow& window)
	{
		sf::RectangleShape overlay({ (float)GAME_WIDTH, (float)GAME_HEIGHT });
		overlay.setFillColor(sf::Color(245, 245, 245));
		window.draw(overlay);

		sf::Text title(font, "SKILL TREE", 60);
		title.setFillColor(sf::Color::Black);
		centerX(title, 25.0f);
		window.draw(title);

		const sf::Vector2f boxSize(120.0f, 50.0f);

		// Liens prerequis -> noeud.
		for (const SkillNode& n : nodes)
		{
			if (n.isRoot)
				continue;
			const SkillNode& p = nodeById(n.prereq);
			sf::Vertex link[] = {
				sf::Vertex{ p.pos + boxSize / 2.0f, sf::Color(150, 150, 150) },
				sf::Vertex{ n.pos + boxSize / 2.0f, sf::Color(150, 150, 150) },
			};
			window.draw(link, 2, sf::PrimitiveType::Lines);
		}

		// Noeuds.
		for (int i = 0; i < (int)nodes.size(); i++)
		{
			const SkillNode& n = nodes[i];
			sf::RectangleShape box(boxSize);
			box.setPosition(n.pos);

			if (nodeUnlocked(n))
				box.setFillColor(sf::Color(90, 180, 90));   // Acquise : vert.
			else if (nodeAvailable(n))
				box.setFillColor(sf::Color(210, 170, 50));  // Disponible : jaune.
			else
				box.setFillColor(sf::Color(120, 120, 120)); // Verrouillee : gris.

			if (i == selectedNode)
			{
				box.setOutlineThickness(4.0f);
				box.setOutlineColor(sf::Color::Red);
			}
			window.draw(box);

			sf::Text label(font, n.name, 24);
			label.setFillColor(sf::Color::White);
			sf::FloatRect lb = label.getGlobalBounds();
			label.setPosition({
				n.pos.x + (boxSize.x - lb.size.x) / 2.0f,
				n.pos.y + (boxSize.y - lb.size.y) / 2.0f - 6.0f });
			window.draw(label);
		}

		sf::Text points(font, "Skill points: " + std::to_string(skillPoints), 30);
		points.setFillColor(sf::Color(30, 30, 30));
		centerX(points, 520.0f);
		window.draw(points);

		sf::Text help(font, "LEFT / RIGHT select    ENTER unlock    SPACE resume", 24);
		help.setFillColor(sf::Color(90, 90, 90));
		centerX(help, 600.0f);
		window.draw(help);
	}

	void drawBar(sf::RenderWindow& window, sf::Vector2f pos, float w, float h,
		float ratio, sf::Color color, const std::string& name)
	{
		if (ratio < 0.0f)
			ratio = 0.0f;
		sf::RectangleShape bg({ w, h });
		bg.setPosition(pos);
		bg.setFillColor(sf::Color(60, 60, 60));
		window.draw(bg);

		sf::RectangleShape fg({ w * ratio, h });
		fg.setPosition(pos);
		fg.setFillColor(color);
		window.draw(fg);

		sf::Text label(font, name, 18);
		label.setFillColor(sf::Color::Black);
		label.setPosition({ pos.x, pos.y - 24.0f });
		window.draw(label);
	}

	void drawHud(sf::RenderWindow& window)
	{
		drawBar(window, { 20.0f, 40.0f }, 220.0f, 18.0f,
			(float)player.hp / PLAYER_MAX_HP, sf::Color::Green, "Bob");
		drawBar(window, { GAME_WIDTH - 320.0f, 40.0f }, 300.0f, 18.0f,
			(float)boss.hp / BOSS_MAX_HP, sf::Color::Red, "Alien");

		// Halo de bouclier autour de Bob.
		if (player.shielding)
		{
			sf::CircleShape halo(46.0f);
			halo.setOrigin({ 46.0f, 46.0f });
			halo.setPosition(playerCenter());
			halo.setFillColor(sf::Color(60, 120, 255, 70));
			halo.setOutlineThickness(2.0f);
			halo.setOutlineColor(sf::Color(60, 120, 255, 180));
			window.draw(halo);
		}
	}
};

int main(void)
{
	sf::RenderWindow window{ sf::VideoMode({ GAME_WIDTH, GAME_HEIGHT }), "Bob can" };
	window.setFramerateLimit(60);

	sf::Texture backgroundTexture("Resources/Background.png");
	sf::Sprite background(backgroundTexture);

	Game game;

	LOG_INFO("BobCan demarre ({}x{})", GAME_WIDTH, GAME_HEIGHT);

	while (window.isOpen())
	{
		while (const std::optional event = window.pollEvent())
		{
			if (event->is<sf::Event::Closed>())
			{
				window.close();
			}
			else if (const auto* kp = event->getIf<sf::Event::KeyPressed>())
			{
				if (kp->code == sf::Keyboard::Key::Escape)
					window.close();
				else
					game.onKeyPressed(kp->code, window);
			}
			else if (const auto* mb = event->getIf<sf::Event::MouseButtonPressed>())
			{
				if (mb->button == sf::Mouse::Button::Left)
					game.onMousePressed(window);
			}
		}

		if (game.state == GameState::Playing)
			game.updatePlaying(window);

		window.clear(sf::Color::White);

		switch (game.state)
		{
		case GameState::Intro:
			game.drawIntro(window);
			break;

		case GameState::Victory:
			game.drawVictory(window);
			break;

		case GameState::SkillTree:
			game.drawSkillTree(window);
			break;

		case GameState::Playing:
			window.draw(background);
			for (const sf::RectangleShape& rectangle : game.world.rectangles)
				window.draw(rectangle);
			window.draw(*game.player.sprite);
			window.draw(*game.boss.sprite);
			for (const Bullet& b : game.player.bullets)
				window.draw(b.rect);
			for (const Bullet& b : game.boss.bullets)
				window.draw(b.rect);
			game.drawHud(window);
			break;
		}

		window.display();
	}

	return 0;
}
