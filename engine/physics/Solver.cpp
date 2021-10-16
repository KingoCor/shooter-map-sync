//
// Created by Иван Ильин on 10.03.2021.
//

#include "Solver.h"
#include "../utils/Log.h"

void Solver::solveCollision(std::shared_ptr<RigidBody> obj1, std::shared_ptr<RigidBody> obj2, const CollisionPoint& collision) {

    Vec3D obj1_velocity_parallel = collision.normal * obj1->velocity().dot(collision.normal);
    Vec3D obj1_velocity_perpendicular = obj1->velocity() - obj1_velocity_parallel;

    Vec3D obj2_velocity_parallel = collision.normal * obj2->velocity().dot(collision.normal);
    Vec3D obj2_velocity_perpendicular = obj2->velocity() - obj2_velocity_parallel;

    if(obj1->isCollision()) {
        if(obj1->velocity().dot(collision.normal) > 0) {
            obj1->setVelocity(obj1_velocity_perpendicular);
        }
    }
    if(obj2->isCollision()) {
        obj2->setVelocity(obj2_velocity_perpendicular - obj2_velocity_parallel * 0.8);
    }

    if(obj1->isCollision() && obj2->isCollision()) {
        obj1->translate(-collision.normal * collision.depth/2.0);
        obj2->translate(collision.normal * collision.depth/2.0);
    } else if(obj1->isCollision()) {
        obj1->translate(-collision.normal * collision.depth);
    }
    else
        obj2->translate(collision.normal * collision.depth);
}

