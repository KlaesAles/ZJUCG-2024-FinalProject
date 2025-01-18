#ifndef CHARACTER_H
#define CHARACTER_H

#include "GameObject.h"

enum class CharacterState {
    Idle,
    Running,
    Jumping
};

class CharacterMove {
public:
    CharacterState state;
    std::shared_ptr<GameObject> obj;
    glm::vec3 toward;

    CharacterMove(std::shared_ptr<GameObject> obj) : obj(obj), state(CharacterState::Idle), toward(glm::vec3(0,0,0)) {}

    void update(bool isRunning,bool isJumping){
        if(isJumping)
            state = CharacterState::Jumping;
        else if(isRunning)
            state = CharacterState::Running;
        else 
            state = CharacterState::Idle;
    }

    void setToward(glm::vec3 toward) {
        obj->setRotation(toward);
        obj->updatePos(0.5f);
    }
    CharacterState getState() {
        return state;
    }
};

#endif