#ifndef FSM_H
#define FSM_H

#include <memory>
#include <unordered_map>
#include <string>
#include <functional>
#include "Character.h"
#include <chrono>
#include "AnimationInfo.h"

class Character;

// Base State class
template<typename T>
class State
{
public:
    State() { static_assert(std::is_base_of<Character, T>::value, "StateType must inherit from State<T>"); }
    virtual ~State() = default;
    virtual void OnEnter(T* owner) = 0;
    virtual void OnExit(T* owner) = 0;
    virtual void Update(T* owner, float dt) = 0;
    virtual std::string GetName() const = 0;
};

// Infinite State Machine
template<typename T>
class StateMachine
{

public:
    StateMachine() : m_owner(nullptr), m_currentState(nullptr) {}
    StateMachine(T* owner) : m_owner(owner), m_currentState(nullptr) {}

	// state management
    template<typename StateType>
    void AddState(const std::string& stateName, std::unique_ptr<StateType> state)   // add state
    {
        static_assert(std::is_base_of<State<T>, StateType>::value, "StateType must inherit from State<T>");
        m_states[stateName] = std::move(state);
    }
	void ChangeState(const std::string& stateName)   // change state
    {
        auto stateNext = m_states.find(stateName);
        if (stateNext != m_states.end())
        {
            if (m_currentState)
                m_currentState->OnExit(m_owner);

            m_currentState = stateNext->second.get();
            m_currentStateName = stateName;
            m_currentState->OnEnter(m_owner);
        }
    }

	// state information
    std::string GetCurrentStateName() const { return m_currentStateName; }                                    // Check the current state
    bool IsInState(const std::string& stateName) const { return m_currentStateName == stateName; }            // Determine if we are in the specified state
    bool HasState(const std::string& stateName) const { return m_states.find(stateName) != m_states.end(); }  // Determine if the specified state exists

	// state update and draw
    void Update(float dt)
    {
        if (m_currentState)
        {
            CheckTransitions(); // check state transitions before updating
            m_currentState->Update(m_owner, dt);
        }
    }

    // increase the state transition conditions
    void AddTransition(const std::string& fromState, const std::string& toState, std::function<bool(T*)> condition)
    {
        m_transitions[fromState].push_back({ toState, condition });
    }
private:
    // check state transitions
    void CheckTransitions()
    {
        auto it = m_transitions.find(m_currentStateName);
        if (it != m_transitions.end())
            for (const auto& transition : it->second)
                if (transition.condition(m_owner))
                {
                    ChangeState(transition.toState);
                    break;
                }
    }
private:
    T* m_owner;
    std::unordered_map<std::string, std::unique_ptr<State<T>>> m_states;
    State<T>* m_currentState;
    std::string m_currentStateName;
    // structure of checking state transitions
    struct Transition
    {
        std::string toState;
        std::function<bool(T*)> condition;
    };
    std::unordered_map<std::string, std::vector<Transition>> m_transitions; // manages state transition conditions
};
#endif