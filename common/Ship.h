#ifndef SHIP_H_INCLUDED
#define SHIP_H_INCLUDED

#include "Object.h"
#include <string>
#include <cstdint>

class Ship : public Object
{
    std::string name;
    float shield;
    float health;
    uint32_t score;
    float orientation;
    float angularVelocity;
    float torque;

public:
    Ship(std::string name);
    virtual ~Ship() = default;

    const float& GetOrientation() const;
    void ApplyTorque(float torque);
    float GetAngularVelocity() const;
    void ApplyDamage(float dmg);
    void AddScore(unsigned pts);
    bool IsDead() const;
    void Respawn(Vector3f position);
    const float& GetShield() const;
    const float& GetHealth() const;
    const std::string& GetName() const;
    uint32_t GetScore() const;

    virtual void Update(float dt);
};

#endif // SHIP_H_INCLUDED
