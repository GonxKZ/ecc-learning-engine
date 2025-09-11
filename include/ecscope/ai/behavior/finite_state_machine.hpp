#pragma once

#include "../core/ai_types.hpp"
#include "../core/blackboard.hpp"
#include <unordered_map>
#include <vector>
#include <functional>
#include <memory>
#include <queue>
#include <string>

namespace ecscope::ai {

/**
 * @brief Finite State Machine - Classical AI behavior system
 * 
 * Provides hierarchical finite state machines with transition conditions,
 * entry/exit actions, and nested state support. Designed for complex
 * AI behaviors with clear state management.
 */

// Forward declarations
class FSMState;
class FSMTransition;
class FSM;

/**
 * @brief FSM Transition Condition
 */
class TransitionCondition {
public:
    virtual ~TransitionCondition() = default;
    virtual bool evaluate(const Blackboard& blackboard, ecs::Entity entity, f64 delta_time) = 0;
    virtual std::string describe() const = 0;
    virtual std::unique_ptr<TransitionCondition> clone() const = 0;
};

/**
 * @brief Concrete transition conditions
 */

// Timer-based condition
class TimerCondition : public TransitionCondition {
public:
    explicit TimerCondition(f64 duration) : duration_(duration), start_time_(-1.0) {}
    
    bool evaluate(const Blackboard& blackboard, ecs::Entity entity, f64 delta_time) override {
        f64 current_time = get_current_time();
        if (start_time_ < 0) {
            start_time_ = current_time;
        }
        return (current_time - start_time_) >= duration_;
    }
    
    std::string describe() const override {
        return "Timer(" + std::to_string(duration_) + "s)";
    }
    
    std::unique_ptr<TransitionCondition> clone() const override {
        return std::make_unique<TimerCondition>(duration_);
    }
    
    void reset() { start_time_ = -1.0; }
    
private:
    f64 duration_;
    f64 start_time_;
    
    static f64 get_current_time() {
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<f64>(now.time_since_epoch()).count();
    }
};

// Blackboard value condition
template<typename T>
class BlackboardCondition : public TransitionCondition {
public:
    using Comparator = std::function<bool(const T&, const T&)>;
    
    BlackboardCondition(const std::string& key, const T& value, Comparator comp)
        : key_(key), value_(value), comparator_(comp) {}
    
    bool evaluate(const Blackboard& blackboard, ecs::Entity entity, f64 delta_time) override {
        auto bb_value = blackboard.get<T>(key_);
        return bb_value && comparator_(*bb_value, value_);
    }
    
    std::string describe() const override {
        return "Blackboard(" + key_ + ")";
    }
    
    std::unique_ptr<TransitionCondition> clone() const override {
        return std::make_unique<BlackboardCondition<T>>(key_, value_, comparator_);
    }
    
private:
    std::string key_;
    T value_;
    Comparator comparator_;
};

// Lambda-based custom condition
class LambdaCondition : public TransitionCondition {
public:
    using ConditionFunc = std::function<bool(const Blackboard&, ecs::Entity, f64)>;
    
    LambdaCondition(ConditionFunc func, const std::string& description)
        : condition_func_(func), description_(description) {}
    
    bool evaluate(const Blackboard& blackboard, ecs::Entity entity, f64 delta_time) override {
        return condition_func_(blackboard, entity, delta_time);
    }
    
    std::string describe() const override {
        return description_;
    }
    
    std::unique_ptr<TransitionCondition> clone() const override {
        return std::make_unique<LambdaCondition>(condition_func_, description_);
    }
    
private:
    ConditionFunc condition_func_;
    std::string description_;
};

// Composite conditions (AND, OR, NOT)
class CompositeCondition : public TransitionCondition {
public:
    enum class Type { AND, OR, NOT };
    
    explicit CompositeCondition(Type type) : type_(type) {}
    
    void add_condition(std::unique_ptr<TransitionCondition> condition) {
        conditions_.push_back(std::move(condition));
    }
    
    bool evaluate(const Blackboard& blackboard, ecs::Entity entity, f64 delta_time) override {
        switch (type_) {
            case Type::AND:
                for (auto& condition : conditions_) {
                    if (!condition->evaluate(blackboard, entity, delta_time)) {
                        return false;
                    }
                }
                return !conditions_.empty();
                
            case Type::OR:
                for (auto& condition : conditions_) {
                    if (condition->evaluate(blackboard, entity, delta_time)) {
                        return true;
                    }
                }
                return false;
                
            case Type::NOT:
                if (conditions_.size() == 1) {
                    return !conditions_[0]->evaluate(blackboard, entity, delta_time);
                }
                return false;
        }
        return false;
    }
    
    std::string describe() const override {
        std::string result;
        switch (type_) {
            case Type::AND: result = "AND("; break;
            case Type::OR: result = "OR("; break;
            case Type::NOT: result = "NOT("; break;
        }
        for (size_t i = 0; i < conditions_.size(); ++i) {
            if (i > 0) result += ", ";
            result += conditions_[i]->describe();
        }
        result += ")";
        return result;
    }
    
    std::unique_ptr<TransitionCondition> clone() const override {
        auto clone = std::make_unique<CompositeCondition>(type_);
        for (const auto& condition : conditions_) {
            clone->add_condition(condition->clone());
        }
        return clone;
    }
    
private:
    Type type_;
    std::vector<std::unique_ptr<TransitionCondition>> conditions_;
};

/**
 * @brief FSM Transition
 */
class FSMTransition {
public:
    FSMTransition(const std::string& from_state, const std::string& to_state,
                  std::unique_ptr<TransitionCondition> condition)
        : from_state_(from_state)
        , to_state_(to_state)
        , condition_(std::move(condition))
        , priority_(Priority::NORMAL) {}
    
    // Check if transition should occur
    bool should_transition(const Blackboard& blackboard, ecs::Entity entity, f64 delta_time) {
        return condition_->evaluate(blackboard, entity, delta_time);
    }
    
    // Getters
    const std::string& from_state() const { return from_state_; }
    const std::string& to_state() const { return to_state_; }
    Priority priority() const { return priority_; }
    
    void set_priority(Priority priority) { priority_ = priority; }
    
    // Transition actions
    void set_action(std::function<void(const Blackboard&, ecs::Entity)> action) {
        action_ = action;
    }
    
    void execute_action(const Blackboard& blackboard, ecs::Entity entity) {
        if (action_) {
            action_(blackboard, entity);
        }
    }
    
    std::string describe() const {
        return from_state_ + " -> " + to_state_ + " [" + condition_->describe() + "]";
    }
    
private:
    std::string from_state_;
    std::string to_state_;
    std::unique_ptr<TransitionCondition> condition_;
    Priority priority_;
    std::function<void(const Blackboard&, ecs::Entity)> action_;
};

/**
 * @brief FSM State
 */
class FSMState {
public:
    using StateAction = std::function<void(const Blackboard&, ecs::Entity, f64)>;
    
    explicit FSMState(const std::string& name) 
        : name_(name), is_active_(false), enter_time_(0.0) {}
    
    // State lifecycle
    void enter(const Blackboard& blackboard, ecs::Entity entity, f64 current_time) {
        is_active_ = true;
        enter_time_ = current_time;
        if (on_enter_) {
            on_enter_(blackboard, entity, 0.0);
        }
    }
    
    void update(const Blackboard& blackboard, ecs::Entity entity, f64 delta_time) {
        if (is_active_ && on_update_) {
            on_update_(blackboard, entity, delta_time);
        }
    }
    
    void exit(const Blackboard& blackboard, ecs::Entity entity, f64 current_time) {
        if (is_active_ && on_exit_) {
            on_exit_(blackboard, entity, current_time - enter_time_);
        }
        is_active_ = false;
    }
    
    // Action assignment
    void set_on_enter(StateAction action) { on_enter_ = action; }
    void set_on_update(StateAction action) { on_update_ = action; }
    void set_on_exit(StateAction action) { on_exit_ = action; }
    
    // State properties
    const std::string& name() const { return name_; }
    bool is_active() const { return is_active_; }
    f64 duration() const { return is_active_ ? get_current_time() - enter_time_ : 0.0; }
    
    // Nested FSM support
    void set_nested_fsm(std::shared_ptr<FSM> nested_fsm) {
        nested_fsm_ = nested_fsm;
    }
    
    std::shared_ptr<FSM> get_nested_fsm() const { return nested_fsm_; }
    bool has_nested_fsm() const { return nested_fsm_ != nullptr; }
    
private:
    std::string name_;
    bool is_active_;
    f64 enter_time_;
    
    StateAction on_enter_;
    StateAction on_update_;
    StateAction on_exit_;
    
    // Hierarchical FSM support
    std::shared_ptr<FSM> nested_fsm_;
    
    static f64 get_current_time() {
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<f64>(now.time_since_epoch()).count();
    }
};

/**
 * @brief Finite State Machine
 */
class FSM {
public:
    explicit FSM(const std::string& name = "FSM") 
        : name_(name), is_active_(false), current_state_(nullptr) {}
    
    // State management
    void add_state(const std::string& name) {
        if (states_.find(name) == states_.end()) {
            states_[name] = std::make_unique<FSMState>(name);
        }
    }
    
    void remove_state(const std::string& name) {
        // Don't remove if it's the current state
        if (current_state_ && current_state_->name() == name) {
            return;
        }
        
        // Remove transitions involving this state
        auto it = transitions_.begin();
        while (it != transitions_.end()) {
            if (it->from_state() == name || it->to_state() == name) {
                it = transitions_.erase(it);
            } else {
                ++it;
            }
        }
        
        states_.erase(name);
    }
    
    FSMState* get_state(const std::string& name) {
        auto it = states_.find(name);
        return it != states_.end() ? it->second.get() : nullptr;
    }
    
    // Transition management
    void add_transition(const std::string& from_state, const std::string& to_state,
                       std::unique_ptr<TransitionCondition> condition) {
        // Ensure states exist
        add_state(from_state);
        add_state(to_state);
        
        transitions_.emplace_back(from_state, to_state, std::move(condition));
    }
    
    void add_transition_with_action(const std::string& from_state, const std::string& to_state,
                                   std::unique_ptr<TransitionCondition> condition,
                                   std::function<void(const Blackboard&, ecs::Entity)> action) {
        add_transition(from_state, to_state, std::move(condition));
        transitions_.back().set_action(action);
    }
    
    // FSM lifecycle
    void start(const std::string& initial_state, const Blackboard& blackboard, 
               ecs::Entity entity) {
        auto initial = get_state(initial_state);
        if (!initial) {
            return;
        }
        
        is_active_ = true;
        current_state_ = initial;
        current_state_->enter(blackboard, entity, get_current_time());
        
        // Start nested FSM if present
        if (current_state_->has_nested_fsm()) {
            auto nested = current_state_->get_nested_fsm();
            // Nested FSM needs its own initial state logic
        }
    }
    
    void update(const Blackboard& blackboard, ecs::Entity entity, f64 delta_time) {
        if (!is_active_ || !current_state_) {
            return;
        }
        
        // Update current state
        current_state_->update(blackboard, entity, delta_time);
        
        // Update nested FSM if present
        if (current_state_->has_nested_fsm()) {
            current_state_->get_nested_fsm()->update(blackboard, entity, delta_time);
        }
        
        // Check transitions
        check_transitions(blackboard, entity, delta_time);
    }
    
    void stop(const Blackboard& blackboard, ecs::Entity entity) {
        if (is_active_ && current_state_) {
            current_state_->exit(blackboard, entity, get_current_time());
            
            // Stop nested FSM
            if (current_state_->has_nested_fsm()) {
                current_state_->get_nested_fsm()->stop(blackboard, entity);
            }
        }
        
        is_active_ = false;
        current_state_ = nullptr;
    }
    
    // Force state change
    void force_transition(const std::string& state_name, const Blackboard& blackboard,
                         ecs::Entity entity) {
        auto new_state = get_state(state_name);
        if (!new_state || !is_active_) {
            return;
        }
        
        if (current_state_) {
            current_state_->exit(blackboard, entity, get_current_time());
        }
        
        current_state_ = new_state;
        current_state_->enter(blackboard, entity, get_current_time());
    }
    
    // State queries
    std::string get_current_state_name() const {
        return current_state_ ? current_state_->name() : "";
    }
    
    bool is_in_state(const std::string& state_name) const {
        return current_state_ && current_state_->name() == state_name;
    }
    
    bool is_active() const { return is_active_; }
    
    // Get available states
    std::vector<std::string> get_state_names() const {
        std::vector<std::string> names;
        for (const auto& [name, state] : states_) {
            names.push_back(name);
        }
        return names;
    }
    
    // Get transition information
    std::vector<std::string> get_possible_transitions() const {
        std::vector<std::string> transitions;
        if (!current_state_) return transitions;
        
        for (const auto& transition : transitions_) {
            if (transition.from_state() == current_state_->name()) {
                transitions.push_back(transition.describe());
            }
        }
        return transitions;
    }
    
    // FSM statistics
    struct Statistics {
        std::string name;
        std::string current_state;
        usize total_states;
        usize total_transitions;
        f64 current_state_duration;
        bool is_active;
        std::unordered_map<std::string, u32> state_visit_counts;
        std::unordered_map<std::string, f64> state_total_durations;
    };
    
    Statistics get_statistics() const {
        Statistics stats;
        stats.name = name_;
        stats.current_state = get_current_state_name();
        stats.total_states = states_.size();
        stats.total_transitions = transitions_.size();
        stats.current_state_duration = current_state_ ? current_state_->duration() : 0.0;
        stats.is_active = is_active_;
        stats.state_visit_counts = state_visit_counts_;
        stats.state_total_durations = state_total_durations_;
        return stats;
    }
    
    // Debug and visualization
    void print_state_machine() const {
        std::cout << "FSM: " << name_ << "\n";
        std::cout << "States: ";
        for (const auto& [name, state] : states_) {
            std::cout << name;
            if (current_state_ && current_state_->name() == name) {
                std::cout << "*";
            }
            std::cout << " ";
        }
        std::cout << "\nTransitions:\n";
        for (const auto& transition : transitions_) {
            std::cout << "  " << transition.describe() << "\n";
        }
    }
    
    const std::string& get_name() const { return name_; }
    
private:
    std::string name_;
    bool is_active_;
    FSMState* current_state_;
    
    std::unordered_map<std::string, std::unique_ptr<FSMState>> states_;
    std::vector<FSMTransition> transitions_;
    
    // Statistics
    mutable std::unordered_map<std::string, u32> state_visit_counts_;
    mutable std::unordered_map<std::string, f64> state_total_durations_;
    mutable std::unordered_map<std::string, f64> state_enter_times_;
    
    void check_transitions(const Blackboard& blackboard, ecs::Entity entity, f64 delta_time) {
        if (!current_state_) return;
        
        // Sort transitions by priority
        std::vector<FSMTransition*> applicable_transitions;
        for (auto& transition : transitions_) {
            if (transition.from_state() == current_state_->name()) {
                applicable_transitions.push_back(&transition);
            }
        }
        
        std::sort(applicable_transitions.begin(), applicable_transitions.end(),
                 [](const FSMTransition* a, const FSMTransition* b) {
                     return a->priority() > b->priority();
                 });
        
        // Check transitions in priority order
        for (auto* transition : applicable_transitions) {
            if (transition->should_transition(blackboard, entity, delta_time)) {
                // Execute transition
                transition->execute_action(blackboard, entity);
                
                // Update statistics
                update_state_statistics();
                
                // Change state
                std::string old_state = current_state_->name();
                current_state_->exit(blackboard, entity, get_current_time());
                
                current_state_ = get_state(transition->to_state());
                if (current_state_) {
                    current_state_->enter(blackboard, entity, get_current_time());
                    
                    // Update visit count
                    state_visit_counts_[current_state_->name()]++;
                    state_enter_times_[current_state_->name()] = get_current_time();
                }
                
                break; // Only execute first matching transition
            }
        }
    }
    
    void update_state_statistics() const {
        if (!current_state_) return;
        
        const std::string& state_name = current_state_->name();
        f64 current_time = get_current_time();
        
        auto it = state_enter_times_.find(state_name);
        if (it != state_enter_times_.end()) {
            f64 duration = current_time - it->second;
            state_total_durations_[state_name] += duration;
        }
    }
    
    static f64 get_current_time() {
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<f64>(now.time_since_epoch()).count();
    }
};

/**
 * @brief FSM Builder - Fluent interface for constructing FSMs
 */
class FSMBuilder {
public:
    explicit FSMBuilder(const std::string& name = "FSM") 
        : fsm_(std::make_shared<FSM>(name)) {}
    
    FSMBuilder& add_state(const std::string& name) {
        fsm_->add_state(name);
        current_state_name_ = name;
        return *this;
    }
    
    FSMBuilder& on_enter(FSMState::StateAction action) {
        if (auto* state = fsm_->get_state(current_state_name_)) {
            state->set_on_enter(action);
        }
        return *this;
    }
    
    FSMBuilder& on_update(FSMState::StateAction action) {
        if (auto* state = fsm_->get_state(current_state_name_)) {
            state->set_on_update(action);
        }
        return *this;
    }
    
    FSMBuilder& on_exit(FSMState::StateAction action) {
        if (auto* state = fsm_->get_state(current_state_name_)) {
            state->set_on_exit(action);
        }
        return *this;
    }
    
    FSMBuilder& transition_to(const std::string& to_state) {
        transition_to_state_ = to_state;
        return *this;
    }
    
    FSMBuilder& when(std::unique_ptr<TransitionCondition> condition) {
        if (!transition_to_state_.empty()) {
            fsm_->add_transition(current_state_name_, transition_to_state_, std::move(condition));
            transition_to_state_.clear();
        }
        return *this;
    }
    
    FSMBuilder& after(f64 seconds) {
        return when(std::make_unique<TimerCondition>(seconds));
    }
    
    template<typename T>
    FSMBuilder& when_blackboard(const std::string& key, const T& value) {
        auto condition = std::make_unique<BlackboardCondition<T>>(
            key, value, [](const T& a, const T& b) { return a == b; });
        return when(std::move(condition));
    }
    
    FSMBuilder& when_custom(std::function<bool(const Blackboard&, ecs::Entity, f64)> func,
                           const std::string& description = "custom") {
        return when(std::make_unique<LambdaCondition>(func, description));
    }
    
    std::shared_ptr<FSM> build() {
        return fsm_;
    }
    
private:
    std::shared_ptr<FSM> fsm_;
    std::string current_state_name_;
    std::string transition_to_state_;
};

} // namespace ecscope::ai