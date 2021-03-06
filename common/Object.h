#ifndef OBJECT_H_INCLUDED
#define OBJECT_H_INCLUDED

#include <Eigen/Core>

using Eigen::Vector3f;

class Object
{
protected:
    Vector3f position;
    Vector3f velocity;
    Vector3f force;
    float mass;
    float radius;
    bool isStatic;
    float linear_dampening;

public:
    Object();
    virtual ~Object() = default;

    const Vector3f& GetPosition() const;
    const Vector3f& GetVelocity() const;
    const float&    GetMass() const;
    bool            GetIsStatic() const;
    const float&    GetRadius() const;
    const Vector3f& GetForce() const;
    void            SetPosition(const Vector3f& position);
    void            SetVelocity(const Vector3f& velocity);
    void            SetMass(const float& mass);
    void            SetIsStatic(bool isStatic);
    void            SetRadius(const float& radius);
    void            SetForce(const Vector3f& force);
    virtual void    AddForce(const Vector3f& force);
    void            SetLinearDampening(const float& dampening);
    virtual void    Update(float dt) = 0;
};

#endif // OBJECT_H_INCLUDED
