#include "Object.h"

Object::Object()
    : position(0.0, 0.0, 0.0)
    , velocity(0.0, 0.0, 0.0)
    , force(0.0, 0.0, 0.0)
    , mass(1.0)
    , radius(1.0)
    , isStatic(false)
{

}

const Vector3f& Object::GetPosition() const
{
    return position;
}

const Vector3f& Object::GetVelocity() const
{
    return velocity;
}

const float& Object::GetMass() const
{
    return mass;
}

bool Object::GetIsStatic() const
{
    return isStatic;
}

const float& Object::GetRadius() const
{
    return radius;
}

const Vector3f& Object::GetForce() const
{
    return force;
}

void Object::SetPosition(const Vector3f& position)
{
    this->position = position;
}

void Object::SetVelocity(const Vector3f& velocity)
{
    this->velocity = velocity;
}

void Object::SetMass(const float& mass)
{
    this->mass = mass;
}

void Object::SetIsStatic(bool isStatic)
{
    this->isStatic = isStatic;
}

void Object::SetRadius(const float& radius)
{
    this->radius = radius;
}

void Object::SetForce(const Vector3f& force)
{
    this->force = force;
}

void Object::AddForce(const Vector3f& force)
{
    this->force += force;
}

void Object::Update(float dt)
{
    if(this->isStatic)
    {
        this->force = Vector3f(0, 0, 0);
    }
    else
    {
        Vector3f a = this->force / this->mass;
        this->position += this->velocity * dt + 0.5f * a * dt*dt;
        this->velocity += a * dt;
    }
}
