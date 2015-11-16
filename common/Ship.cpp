#include "Ship.h"

const float DefaultShield = 100.0f;
const float DefaultHealth = 100.0f;

Ship::Ship(std::string name)
{
    this->name = name;
    shield = DefaultShield;
    health = DefaultHealth;
}

void Ship::ApplyDamage(float dmg)
{
    shield -= dmg;
    if(shield<0.f)
    {
        health += shield;
        shield = 0.f;
    }
}

void Ship::AddScore(unsigned pts)
{
    score += pts;
}

bool Ship::IsDead() const
{
    return health <= 0.f;
}

void Ship::Respawn(Vector3f position)
{
    health = DefaultHealth;
    shield = DefaultShield;
    SetPosition(position);
    SetVelocity(Vector3f(0.0, 0.0, 0.0));
}

const float& Ship::GetShield() const
{
    return shield;
}

const float& Ship::GetHealth() const
{
    return health;
}

const std::string& Ship::GetName() const
{
    return name;
}

unsigned Ship::GetScore() const
{
    return score;
}
