#include "Projectile.hpp"
#include "SharedState.hpp"
#include "Kirby.hpp"
#include "Enemy.hpp"
#include <cmath>

Projectile::Projectile(float sx,float sy,int d,bool fk, Ability abilityType)
    :x(sx),y(sy),direction(d),speed(7.0f),inactive(false),fromKirby(fk),animTimer(0), ability(abilityType), lifeTime(0.0f), useSprite(false), vy(0)
{
    shape.setRadius(8);shape.setOrigin(8,8);shape.setPointCount(5);
    if(fk){
        shape.setFillColor(sf::Color(255,255,100));shape.setOutlineColor(sf::Color(255,200,50));
        if (ability == Ability::NONE) {
            speed = 8.0f;
            if (tex.loadFromFile("Sprys Nuevos/Animaciones Kirby/sprite_193.png")) {
                sprite.setTexture(tex); sprite.setScale(3.0f, 3.0f); useSprite = true;
            }
            shape.setRadius(12); shape.setOrigin(12,12); shape.setPointCount(10);
            shape.setFillColor(sf::Color(255, 255, 100));
        } else if (ability == Ability::FIRE) {
            speed = 9.0f;
            shape.setFillColor(sf::Color(255, 100, 50));
            shape.setRadius(16); shape.setOrigin(16,16);
            shape.setOutlineColor(sf::Color(255, 200, 50));
            shape.setOutlineThickness(2);
        } else if (ability == Ability::SWORD) {
            speed = 0.0f;
            shape.setRadius(32); shape.setOrigin(32,32); shape.setPointCount(3);
            shape.setFillColor(sf::Color(150, 255, 150, 220));
            shape.setOutlineColor(sf::Color(255, 255, 255));
            shape.setOutlineThickness(2);
        } else if (ability == Ability::SPARK) {
            speed = 0.0f;
            shape.setRadius(50); shape.setOrigin(50,50); shape.setPointCount(16);
            shape.setFillColor(sf::Color(255, 255, 100, 180));
            shape.setOutlineColor(sf::Color(100, 200, 255));
            shape.setOutlineThickness(3);
        } else if (ability == Ability::BEAM) {
            speed = 13.0f;
            shape.setFillColor(sf::Color(100, 200, 255));
            shape.setRadius(10); shape.setOrigin(10,10);
            shape.setOutlineColor(sf::Color(255, 255, 255));
            shape.setOutlineThickness(2);
        } else if (ability == Ability::BOMB_RETURN) {
            speed = 10.0f;
            vy = -6.0f;
            shape.setRadius(12); shape.setOrigin(12,12);
            shape.setFillColor(sf::Color(255, 100, 50));
            shape.setOutlineColor(sf::Color(255, 200, 100));
            shape.setOutlineThickness(2);
        }
    } else if (ability == Ability::BOMB) {
        speed = 4.0f;
        vy = -8.0f;
        if (tex.loadFromFile("Sprys Nuevos/Bosses/sprite_019.png")) {
            sprite.setTexture(tex);
            sprite.setScale(2.0f, 2.0f);
            useSprite = true;
        } else {
            shape.setRadius(12); shape.setOrigin(12,12);
            shape.setFillColor(sf::Color(80, 80, 80));
            shape.setOutlineColor(sf::Color(200, 200, 200));
            shape.setOutlineThickness(2);
        }
    } else {
        shape.setFillColor(sf::Color(255,80,80));shape.setOutlineColor(sf::Color(200,50,50));shape.setOutlineThickness(2);
    }
}
Projectile::~Projectile(){}

void Projectile::update(SharedState* s){
    if(inactive)return;
    lifeTime += 0.016f;
    x+=speed*direction; animTimer+=0.016f; shape.setRotation(animTimer*360);

    if (ability == Ability::BOMB || ability == Ability::BOMB_RETURN) {
        y += vy;
        vy += 0.3f;
        if (y > 600) { inactive = true; return; }
    }

    if (ability == Ability::SWORD) {
        if (s->kirby) {
            x = s->kirby->getX() + 24 + direction * 35;
            y = s->kirby->getY() + 24;
        }
        if (lifeTime > 0.5f) { inactive = true; return; }
    } else if (ability == Ability::SPARK) {
        if (s->kirby) {
            x = s->kirby->getX() + 24;
            y = s->kirby->getY() + 24;
        }
        if (lifeTime > 0.6f) { inactive = true; return; }
    } else if (ability == Ability::FIRE) {
        if (lifeTime > 0.8f) { inactive = true; return; }
    } else if (ability == Ability::BEAM) {
        if (lifeTime > 1.2f) { inactive = true; return; }
    }

    if(x<-20||x>5000||y<-20||y>700){inactive=true;return;}
    sf::FloatRect pb=getBounds();
    if(fromKirby){
        for(size_t i=0;i<s->enemies.size();i++){
            Enemy* e=s->enemies[i];
            if(e&&!e->isDead()&&pb.intersects(e->getBounds())){
                e->die();s->score+=50;s->playSoundEnemyDie=true;
                if (ability != Ability::SWORD && ability != Ability::SPARK && ability != Ability::BOMB_RETURN) inactive=true;
                return;
            }
        }
    } else {
        if(s->kirby&&!s->kirby->isDead()&&pb.intersects(s->kirby->getBounds())){
            s->kirby->takeDamage(s);inactive=true;return;}
    }
    for(size_t i=0;i<s->platforms.size();i++){
        if(s->platforms[i].type==TileType::GROUND){
            sf::FloatRect pr(s->platforms[i].x,s->platforms[i].y,s->platforms[i].w,s->platforms[i].h);
            if(pb.intersects(pr)){inactive=true;return;}
        }
    }
}

void Projectile::render(sf::RenderWindow& win, float camX){
    if(inactive)return;
    if(fromKirby){
        if (ability == Ability::SPARK || ability == Ability::SWORD) {
            shape.setPosition(x-camX, y);
            win.draw(shape);
        } else {
            sf::ConvexShape star(10);
            float size = 8.0f;
            float out = size, in = size/2.0f;
            for(int i=0; i<10; i++){
                float r = (i%2==0) ? out : in;
                float angle = i * 3.14159f / 5.0f - 3.14159f/2.0f;
                star.setPoint(i, sf::Vector2f(r*cos(angle), r*sin(angle)));
            }
            if (ability == Ability::BEAM) star.setFillColor(sf::Color(100,200,255));
            else if (ability == Ability::FIRE) star.setFillColor(sf::Color(255,100,50));
            else star.setFillColor(sf::Color(255,255,100));
            star.setPosition(x-camX, y);
            star.setRotation(animTimer*360);
            star.setOutlineColor(sf::Color(255,200,50));
            star.setOutlineThickness(2);
            win.draw(star);
        }
    } else {
        if (useSprite) {
            sprite.setPosition(x-camX, y);
            win.draw(sprite);
        } else {
            shape.setPosition(x-camX,y); win.draw(shape);
        }
    }
}

sf::FloatRect Projectile::getBounds() const {
    if (ability == Ability::SPARK) return sf::FloatRect(x-50,y-50,100,100);
    if (ability == Ability::SWORD) return sf::FloatRect(x-32,y-32,64,64);
    if (ability == Ability::FIRE) return sf::FloatRect(x-16,y-16,32,32);
    if (ability == Ability::BEAM) return sf::FloatRect(x-10,y-10,20,20);
    if (ability == Ability::BOMB || ability == Ability::BOMB_RETURN) return sf::FloatRect(x-12,y-12,24,24);
    return sf::FloatRect(x-8,y-8,16,16);
}
