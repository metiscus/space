#include "Ship.h"
#include <functional>

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

float Ship::GetTorque() const
{
    return torque;
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
     if(!GetIsStatic())
     {
         float a = torque / DefaultInertia;
         orientation += angularVelocity * dt + 0.5 * a * dt * dt;
         angularVelocity += dt * a;
     }

     torque = 0.0f;
     force[0] = force[1] = force[2] = 0.f;
 }

 void Ship::GetServerUpdateMessage(ServerShipUpdate& message) const
 {
     const std::hash<std::string> player_hash_fn;
     message.ship_id     = player_hash_fn(GetName());
     Vector3f vec = GetPosition();
     message.position[0] = vec[0];
     message.position[1] = vec[1];
     message.position[2] = vec[2];
     vec = GetVelocity();
     message.velocity[0] = vec[0];
     message.velocity[1] = vec[1];
     message.velocity[2] = vec[2];
     message.orientation     = GetOrientation();
     message.angularVelocity = GetAngularVelocity();
     message.health = health;
     message.shield = shield;
 }

 void Ship::GetClientUpdateMessage(ClientShipUpdate& message) const
 {
     message.fired = false; //TODO: implement
     strcpy(message.name, GetName().c_str());
     Vector3f vec = GetForce();
     message.force[0] = vec[0];
     message.force[1] = vec[1];
     message.force[2] = vec[2];
     message.torque = GetTorque();
 }
