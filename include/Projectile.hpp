#ifndef PROJECTILE_HPP
#define PROJECTILE_HPP
#include <SFML/Graphics.hpp>
#include "Constants.hpp"
class SharedState;

class Projectile {
public:
    Projectile(float startX, float startY, int dir, bool fromKirby, Ability abilityType = Ability::NONE);
    ~Projectile();
    void update(SharedState* state);
    void render(sf::RenderWindow& window, float camX);
    sf::FloatRect getBounds() const;
    bool isInactive() const { return inactive; }
    bool isFromKirby() const { return fromKirby; }
    Ability getAbility() const { return ability; }
    void deactivate() { inactive = true; }
private:
    float x,y; int direction; float speed;
    bool inactive, fromKirby;
    sf::CircleShape shape; float animTimer;
    Ability ability;
    float lifeTime;
    sf::Texture tex;
    sf::Sprite sprite;
    bool useSprite;
    float vy;
};
#endif
