#include "Ship.h"

const float DefaultShield = 100.0f;
const float DefaultHealth = 100.0f;
const float DefaultInertia = 1.0f;

Ship::Ship(std::string name)
{
    this->name = name;
    shield = DefaultShield;
    health = DefaultHealth;
    orientation = 0.0;
    angularVelocity = 0.0f;
    torque = 0.0f;
}

const float& Ship::GetOrientation() const
{
    return orientation;
}

void Ship::AddTorque(float torque)
{
    torque += torque;
}

float Ship::GetAngularVelocity() const
{
    return angularVelocity;
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

uint32_t Ship::GetScore() const
{
    return score;
}

 void Ship::Update(float dt)
 {
     Object::Update(dt);

     // add in angular mechanics
     if(GetIsStatic())
     {
         torque = 0.0f;
         angularVelocity = 0.0f;
     }
     else
     {
         float a = torque / DefaultInertia;
         orientation += angularVelocity * dt + 0.5 * a * dt * dt;
         angularVelocity += dt * a;
     }
 }
