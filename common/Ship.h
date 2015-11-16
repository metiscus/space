#ifndef SHIP_H_INCLUDED
#define SHIP_H_INCLUDED

#include "Object.h"
#include <string>

class Ship : public Object
{
    std::string name;
    float shield;
    float health;
    unsigned score;

public:
    Ship(std::string name);
    virtual ~Ship() = default;

    void ApplyDamage(float dmg);
    void AddScore(unsigned pts);
    bool IsDead() const;
    void Respawn(Vector3f position);
    const float& GetShield() const;
    const float& GetHealth() const;
    const std::string& GetName() const;
    unsigned GetScore() const;
};

#endif // SHIP_H_INCLUDED
