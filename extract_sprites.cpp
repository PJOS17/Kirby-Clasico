// Extrae sprites individuales como PNGs transparentes
#include <SFML/Graphics.hpp>
#include <iostream>
#include <sys/stat.h>

sf::Image extractFrame(sf::Image& sheet, sf::Color bg, int fx, int fy, int fw, int fh) {
    sf::Image frame;
    frame.create(fw, fh, sf::Color::Transparent);
    for (int y = 0; y < fh; y++) {
        for (int x = 0; x < fw; x++) {
            int sx = fx+x, sy = fy+y;
            if (sx < (int)sheet.getSize().x && sy < (int)sheet.getSize().y) {
                sf::Color c = sheet.getPixel(sx, sy);
                int d = abs(c.r-bg.r)+abs(c.g-bg.g)+abs(c.b-bg.b);
                if (d > 25) frame.setPixel(x, y, c);
            }
        }
    }
    return frame;
}

int main() {
    mkdir("sprites", 0755);

    // === KIRBY (Playable Characters, bg=0,219,255) ===
    sf::Image kirby;
    if (!kirby.loadFromFile("Playable Characters y Pooderes Absorvidos/Game Boy Advance - Kirby_ Nightmare in Dream Land - Playable Characters - Kirby.png")) return 1;
    sf::Color kbg(0,219,255);

    // Idle (row y=5-26)
    extractFrame(kirby,kbg,5,5,20,22).saveToFile("sprites/k_idle0.png");
    extractFrame(kirby,kbg,30,5,20,22).saveToFile("sprites/k_idle1.png");
    // Walk (row y=73-92)
    extractFrame(kirby,kbg,5,73,20,20).saveToFile("sprites/k_walk0.png");
    extractFrame(kirby,kbg,30,73,21,20).saveToFile("sprites/k_walk1.png");
    extractFrame(kirby,kbg,56,73,20,20).saveToFile("sprites/k_walk2.png");
    extractFrame(kirby,kbg,81,73,21,20).saveToFile("sprites/k_walk3.png");
    extractFrame(kirby,kbg,107,73,20,20).saveToFile("sprites/k_walk4.png");
    extractFrame(kirby,kbg,132,73,21,20).saveToFile("sprites/k_walk5.png");
    // Jump (row y=147-170)
    extractFrame(kirby,kbg,5,147,22,24).saveToFile("sprites/k_jump.png");
    extractFrame(kirby,kbg,32,147,22,24).saveToFile("sprites/k_fall.png");
    // Float (row y=176-200)
    extractFrame(kirby,kbg,5,176,24,25).saveToFile("sprites/k_float0.png");
    extractFrame(kirby,kbg,34,176,24,25).saveToFile("sprites/k_float1.png");
    // Absorb (row y=99-117)
    extractFrame(kirby,kbg,5,99,20,19).saveToFile("sprites/k_absorb.png");
    // Hurt (row y=123-142)
    extractFrame(kirby,kbg,5,123,24,20).saveToFile("sprites/k_hurt.png");
    std::cout << "Kirby: 14 sprites extraidos\n";

    // === WADDLE DEE (bg=82,158,158) ===
    sf::Image waddle;
    if (waddle.loadFromFile("Enemigos/Game Boy Advance - Kirby_ Nightmare in Dream Land - Enemies - Waddle Dee.png")) {
        sf::Color wbg = waddle.getPixel(0,0);
        extractFrame(waddle,wbg,8,8,24,24).saveToFile("sprites/waddle0.png");
        extractFrame(waddle,wbg,34,8,24,24).saveToFile("sprites/waddle1.png");
        std::cout << "Waddle Dee: 2 sprites extraidos\n";
    }

    // === BRONTO BURT ===
    sf::Image bronto;
    if (bronto.loadFromFile("Enemigos/Game Boy Advance - Kirby_ Nightmare in Dream Land - Enemies - Bronto Burt.png")) {
        sf::Color bbg = bronto.getPixel(0,0);
        extractFrame(bronto,bbg,8,8,24,24).saveToFile("sprites/bronto0.png");
        extractFrame(bronto,bbg,34,8,24,24).saveToFile("sprites/bronto1.png");
        std::cout << "Bronto Burt: 2 sprites extraidos\n";
    }

    // === CAPPY ===
    sf::Image cappy;
    if (cappy.loadFromFile("Enemigos/Game Boy Advance - Kirby_ Nightmare in Dream Land - Enemies - Cappy.png")) {
        sf::Color cbg = cappy.getPixel(0,0);
        extractFrame(cappy,cbg,8,6,24,28).saveToFile("sprites/cappy0.png");
        extractFrame(cappy,cbg,40,6,24,28).saveToFile("sprites/cappy1.png");
        std::cout << "Cappy: 2 sprites extraidos\n";
    }

    std::cout << "Extraccion completada!\n";
    return 0;
}
