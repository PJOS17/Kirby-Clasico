#ifndef KIRBY_HPP
#define KIRBY_HPP
#include <SFML/Graphics.hpp>
#include "Constants.hpp"
class SharedState;

class Kirby {
public:
    Kirby(float startX, float startY);
    ~Kirby();
    void update(SharedState* state);
    void updateCPU(SharedState* state);
    void render(sf::RenderWindow& window, float camX);
    void moveLeft(); void moveRight();    void stopMoving();
    void jump(SharedState* ss); void floatUp(SharedState* ss);
    void startAbsorb(SharedState* ss);
    void stopAbsorb(SharedState* ss);
    void swallow(SharedState* ss);
    void spitOrUseAbility(SharedState* s);
    float getX() const { return x; } float getY() const { return y; }
    void setState(KirbyState s) { state = s; }
    int getDirection() const { return direction; }
    KirbyState getState() const { return state; }
    Ability getAbility() const { return ability; }
    int getLives() const { return lives; } int getHealth() const { return health; }
    int getFloatsLeft() const { return floatsLeft; }
    bool isDead() const { return lives<=0; }
    bool hasEnemyInMouth() const { return state==KirbyState::HAS_ENEMY; }
    void setAbility(Ability a) { ability=a; }
    void setLivesAndHealth(int l, int h) { lives = l; health = h; }
    void takeDamage(SharedState* ss);
    void resetState();
    sf::FloatRect getBounds() const;
    sf::FloatRect getAbsorbBounds() const;
    Ability getPendingAbility() const { return pendingAbility; }
private:
    void applyGravity(); void checkPlatformCollisions(SharedState* ss);
    float x,y,vx,vy; int direction; KirbyState state; Ability ability;
    Ability pendingAbility;
    int lives,health,floatsLeft; float absorbTimer,hurtTimer; bool onGround;
    void resetAnimation();
    sf::Texture texIdle[2], texWalkLeft[2], texWalkRight[2], texJump, texFall;
    sf::Texture texFloat[4], texAbsorb, texHurt;
    sf::Texture texPuffed[5], texSpit[3];
    sf::Sprite sprite;
    bool textureLoaded; int animFrame; float animTimer;
    float attackCooldown;
};
#endif
