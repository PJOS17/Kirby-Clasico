#ifndef ENEMY_HPP
#define ENEMY_HPP
#include <SFML/Graphics.hpp>
#include "Constants.hpp"
class SharedState;

class Enemy {
public:
    Enemy(float startX, float startY, float patrolDist, Ability dropAbility, EnemyType type);
    ~Enemy();
    void update(SharedState* state);
    void render(sf::RenderWindow& window, float camX);
    sf::FloatRect getBounds() const;
    bool isDead() const { return dead; }
    Ability getDropAbility() const { return dropAbility; }
    float getX() const { return x; } float getY() const { return y; }
    void die();
    bool isBoss() const { return enemyType == EnemyType::BOSS || enemyType == EnemyType::DANCER_BOSS; }
    bool isDancerDefeated() const { return enemyType == EnemyType::DANCER_BOSS && dancerState == 3; }
    void takeDamage();
private:
    float x,y,originX,patrolDistance; int direction; float speed;
    bool dead; Ability dropAbility; EnemyType enemyType;
    sf::Texture frameTex[10]; sf::Sprite sprite;
    bool textureLoaded; int animFrame; float animTimer;
    int bossHealth;
    float stateTimer;
    int dancerState;
    float vy;
    float bombCooldown;
    float hitTimer;
    bool onPlatform;
};
#endif
