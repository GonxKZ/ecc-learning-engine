#pragma once

#include "../core/ai_types.hpp"
#include "../core/blackboard.hpp"
#include <memory>
#include <vector>
#include <string>
#include <functional>
#include <queue>

namespace ecscope::ai {

/**
 * @brief Behavior Tree System - Modern AI behavior architecture
 * 
 * Provides a complete behavior tree implementation with composite nodes,
 * decorator nodes, leaf nodes, and advanced execution control. Designed
 * for complex, reusable AI behaviors.
 */

// Forward declarations
class BehaviorNode;
class BehaviorTree;

/**
 * @brief Behavior Tree Node Status
 */
enum class NodeStatus : u8 {
    INVALID = 0,    // Node not initialized
    SUCCESS,        // Node completed successfully
    FAILURE,        // Node failed
    RUNNING,        // Node is still executing
    SUSPENDED       // Node execution suspended
};

/**
 * @brief Execution Context for behavior tree nodes
 */
struct ExecutionContext {
    std::shared_ptr<Blackboard> blackboard;
    ecs::Entity entity = ecs::Entity::invalid();
    f64 delta_time = 0.0;
    f64 total_time = 0.0;
    u32 execution_count = 0;
    
    // Context stack for nested executions
    std::vector<std::string> execution_stack;
    
    // Runtime data
    std::unordered_map<std::string, f32> runtime_variables;
    std::unordered_map<std::string, bool> flags;
};

/**
 * @brief Base Behavior Node
 */
class BehaviorNode {
public:
    explicit BehaviorNode(const std::string& name = "Node") 
        : name_(name), status_(NodeStatus::INVALID), execution_count_(0) {}
    
    virtual ~BehaviorNode() = default;
    
    // Core execution interface
    NodeStatus execute(ExecutionContext& context) {
        context.execution_stack.push_back(name_);
        
        // Initialize node if not done yet
        if (status_ == NodeStatus::INVALID) {
            initialize(context);
        }
        
        // Execute node logic
        NodeStatus result = update(context);
        status_ = result;
        execution_count_++;
        
        // Handle completion
        if (result == NodeStatus::SUCCESS || result == NodeStatus::FAILURE) {
            terminate(context);
        }
        
        context.execution_stack.pop_back();
        return result;
    }
    
    // Node lifecycle
    virtual void initialize(ExecutionContext& context) {}
    virtual NodeStatus update(ExecutionContext& context) = 0;
    virtual void terminate(ExecutionContext& context) {}
    
    // Node management
    virtual void reset() { 
        status_ = NodeStatus::INVALID; 
        execution_count_ = 0;
    }
    
    virtual void abort() {
        status_ = NodeStatus::FAILURE;
    }
    
    // Node information
    const std::string& name() const { return name_; }
    NodeStatus status() const { return status_; }
    u32 execution_count() const { return execution_count_; }
    
    // Node hierarchy
    virtual bool is_composite() const { return false; }
    virtual bool is_decorator() const { return false; }
    virtual bool is_leaf() const { return true; }
    
    // Child management (for composite nodes)
    virtual void add_child(std::shared_ptr<BehaviorNode> child) {}
    virtual void remove_child(std::shared_ptr<BehaviorNode> child) {}
    virtual const std::vector<std::shared_ptr<BehaviorNode>>& get_children() const {
        static std::vector<std::shared_ptr<BehaviorNode>> empty;
        return empty;
    }
    
    // Debug and visualization
    virtual std::string describe() const { return name_; }
    virtual void print_tree(u32 depth = 0) const {
        for (u32 i = 0; i < depth; ++i) std::cout << "  ";
        std::cout << name_ << " [" << status_to_string(status_) << "]\n";
    }
    
protected:
    std::string name_;
    NodeStatus status_;
    u32 execution_count_;
    
    static std::string status_to_string(NodeStatus status) {
        switch (status) {
            case NodeStatus::INVALID: return "INVALID";
            case NodeStatus::SUCCESS: return "SUCCESS";
            case NodeStatus::FAILURE: return "FAILURE";
            case NodeStatus::RUNNING: return "RUNNING";
            case NodeStatus::SUSPENDED: return "SUSPENDED";
        }
        return "UNKNOWN";
    }
};

/**
 * @brief Composite Nodes - Nodes with multiple children
 */

// Base composite node
class CompositeNode : public BehaviorNode {
public:
    explicit CompositeNode(const std::string& name) : BehaviorNode(name) {}
    
    bool is_composite() const override { return true; }
    bool is_leaf() const override { return false; }
    
    void add_child(std::shared_ptr<BehaviorNode> child) override {
        children_.push_back(child);
    }
    
    void remove_child(std::shared_ptr<BehaviorNode> child) override {
        children_.erase(
            std::remove(children_.begin(), children_.end(), child),
            children_.end()
        );
    }
    
    const std::vector<std::shared_ptr<BehaviorNode>>& get_children() const override {
        return children_;
    }
    
    void reset() override {
        BehaviorNode::reset();
        current_child_index_ = 0;
        for (auto& child : children_) {
            child->reset();
        }
    }
    
    void print_tree(u32 depth = 0) const override {
        BehaviorNode::print_tree(depth);
        for (const auto& child : children_) {
            child->print_tree(depth + 1);
        }
    }
    
protected:
    std::vector<std::shared_ptr<BehaviorNode>> children_;
    usize current_child_index_ = 0;
};

// Sequence Node - Executes children in order, fails if any child fails
class SequenceNode : public CompositeNode {
public:
    explicit SequenceNode(const std::string& name = "Sequence") 
        : CompositeNode(name) {}
    
    NodeStatus update(ExecutionContext& context) override {
        if (children_.empty()) {
            return NodeStatus::SUCCESS;
        }
        
        while (current_child_index_ < children_.size()) {
            auto& current_child = children_[current_child_index_];
            NodeStatus child_status = current_child->execute(context);
            
            if (child_status == NodeStatus::RUNNING) {
                return NodeStatus::RUNNING;
            } else if (child_status == NodeStatus::FAILURE) {
                return NodeStatus::FAILURE;
            } else if (child_status == NodeStatus::SUCCESS) {
                current_child_index_++;
            }
        }
        
        return NodeStatus::SUCCESS;
    }
    
    void reset() override {
        CompositeNode::reset();
    }
};

// Selector Node - Executes children until one succeeds
class SelectorNode : public CompositeNode {
public:
    explicit SelectorNode(const std::string& name = "Selector") 
        : CompositeNode(name) {}
    
    NodeStatus update(ExecutionContext& context) override {
        if (children_.empty()) {
            return NodeStatus::FAILURE;
        }
        
        while (current_child_index_ < children_.size()) {
            auto& current_child = children_[current_child_index_];
            NodeStatus child_status = current_child->execute(context);
            
            if (child_status == NodeStatus::RUNNING) {
                return NodeStatus::RUNNING;
            } else if (child_status == NodeStatus::SUCCESS) {
                return NodeStatus::SUCCESS;
            } else if (child_status == NodeStatus::FAILURE) {
                current_child_index_++;
            }
        }
        
        return NodeStatus::FAILURE;
    }
};

// Parallel Node - Executes all children simultaneously
class ParallelNode : public CompositeNode {
public:
    enum class Policy : u8 {
        REQUIRE_ALL,    // All children must succeed
        REQUIRE_ONE,    // At least one child must succeed
        REQUIRE_N       // Exactly N children must succeed
    };
    
    explicit ParallelNode(const std::string& name = "Parallel", 
                         Policy policy = Policy::REQUIRE_ALL, u32 required_count = 0)
        : CompositeNode(name), policy_(policy), required_count_(required_count) {}
    
    NodeStatus update(ExecutionContext& context) override {
        if (children_.empty()) {
            return NodeStatus::SUCCESS;
        }
        
        u32 success_count = 0;
        u32 failure_count = 0;
        u32 running_count = 0;
        
        // Execute all children
        for (auto& child : children_) {
            NodeStatus child_status = child->execute(context);
            
            switch (child_status) {
                case NodeStatus::SUCCESS:
                    success_count++;
                    break;
                case NodeStatus::FAILURE:
                    failure_count++;
                    break;
                case NodeStatus::RUNNING:
                    running_count++;
                    break;
                default:
                    break;
            }
        }
        
        // Evaluate based on policy
        switch (policy_) {
            case Policy::REQUIRE_ALL:
                if (failure_count > 0) return NodeStatus::FAILURE;
                if (success_count == children_.size()) return NodeStatus::SUCCESS;
                return NodeStatus::RUNNING;
                
            case Policy::REQUIRE_ONE:
                if (success_count > 0) return NodeStatus::SUCCESS;
                if (failure_count == children_.size()) return NodeStatus::FAILURE;
                return NodeStatus::RUNNING;
                
            case Policy::REQUIRE_N:
                if (success_count >= required_count_) return NodeStatus::SUCCESS;
                if (success_count + running_count < required_count_) return NodeStatus::FAILURE;
                return NodeStatus::RUNNING;
        }
        
        return NodeStatus::RUNNING;
    }
    
private:
    Policy policy_;
    u32 required_count_;
};

/**
 * @brief Decorator Nodes - Nodes that modify child behavior
 */

// Base decorator node
class DecoratorNode : public BehaviorNode {
public:
    explicit DecoratorNode(const std::string& name, std::shared_ptr<BehaviorNode> child = nullptr)
        : BehaviorNode(name), child_(child) {}
    
    bool is_decorator() const override { return true; }
    bool is_leaf() const override { return false; }
    
    void set_child(std::shared_ptr<BehaviorNode> child) {
        child_ = child;
    }
    
    std::shared_ptr<BehaviorNode> get_child() const { return child_; }
    
    void reset() override {
        BehaviorNode::reset();
        if (child_) {
            child_->reset();
        }
    }
    
    void print_tree(u32 depth = 0) const override {
        BehaviorNode::print_tree(depth);
        if (child_) {
            child_->print_tree(depth + 1);
        }
    }
    
protected:
    std::shared_ptr<BehaviorNode> child_;
};

// Inverter Node - Inverts child result (success becomes failure and vice versa)
class InverterNode : public DecoratorNode {
public:
    explicit InverterNode(const std::string& name = "Inverter", 
                         std::shared_ptr<BehaviorNode> child = nullptr)
        : DecoratorNode(name, child) {}
    
    NodeStatus update(ExecutionContext& context) override {
        if (!child_) {
            return NodeStatus::FAILURE;
        }
        
        NodeStatus child_status = child_->execute(context);
        
        switch (child_status) {
            case NodeStatus::SUCCESS:
                return NodeStatus::FAILURE;
            case NodeStatus::FAILURE:
                return NodeStatus::SUCCESS;
            case NodeStatus::RUNNING:
                return NodeStatus::RUNNING;
            default:
                return NodeStatus::FAILURE;
        }
    }
};

// Repeat Node - Repeats child execution N times or until failure
class RepeatNode : public DecoratorNode {
public:
    explicit RepeatNode(const std::string& name = "Repeat", u32 count = 1,
                       std::shared_ptr<BehaviorNode> child = nullptr)
        : DecoratorNode(name, child), max_count_(count), current_count_(0) {}
    
    NodeStatus update(ExecutionContext& context) override {
        if (!child_) {
            return NodeStatus::FAILURE;
        }
        
        while (current_count_ < max_count_) {
            NodeStatus child_status = child_->execute(context);
            
            if (child_status == NodeStatus::RUNNING) {
                return NodeStatus::RUNNING;
            } else if (child_status == NodeStatus::FAILURE) {
                return NodeStatus::FAILURE;
            } else if (child_status == NodeStatus::SUCCESS) {
                current_count_++;
                child_->reset(); // Reset for next iteration
            }
        }
        
        return NodeStatus::SUCCESS;
    }
    
    void reset() override {
        DecoratorNode::reset();
        current_count_ = 0;
    }
    
private:
    u32 max_count_;
    u32 current_count_;
};

// Cooldown Node - Prevents child execution until cooldown expires
class CooldownNode : public DecoratorNode {
public:
    explicit CooldownNode(const std::string& name = "Cooldown", f64 cooldown_duration = 1.0,
                         std::shared_ptr<BehaviorNode> child = nullptr)
        : DecoratorNode(name, child), cooldown_duration_(cooldown_duration), last_execution_time_(0.0) {}
    
    NodeStatus update(ExecutionContext& context) override {
        if (!child_) {
            return NodeStatus::FAILURE;
        }
        
        f64 current_time = context.total_time;
        
        // Check if cooldown has expired
        if (current_time - last_execution_time_ < cooldown_duration_) {
            return NodeStatus::FAILURE;
        }
        
        NodeStatus child_status = child_->execute(context);
        
        // Update last execution time on completion
        if (child_status == NodeStatus::SUCCESS || child_status == NodeStatus::FAILURE) {
            last_execution_time_ = current_time;
        }
        
        return child_status;
    }
    
    void reset() override {
        DecoratorNode::reset();
        last_execution_time_ = 0.0;
    }
    
private:
    f64 cooldown_duration_;
    f64 last_execution_time_;
};

// Timeout Node - Fails child if it runs too long
class TimeoutNode : public DecoratorNode {
public:
    explicit TimeoutNode(const std::string& name = "Timeout", f64 timeout_duration = 5.0,
                        std::shared_ptr<BehaviorNode> child = nullptr)
        : DecoratorNode(name, child), timeout_duration_(timeout_duration), start_time_(-1.0) {}
    
    NodeStatus update(ExecutionContext& context) override {
        if (!child_) {
            return NodeStatus::FAILURE;
        }
        
        f64 current_time = context.total_time;
        
        // Initialize start time
        if (start_time_ < 0) {
            start_time_ = current_time;
        }
        
        // Check timeout
        if (current_time - start_time_ > timeout_duration_) {
            child_->abort();
            return NodeStatus::FAILURE;
        }
        
        NodeStatus child_status = child_->execute(context);
        
        // Reset start time on completion
        if (child_status == NodeStatus::SUCCESS || child_status == NodeStatus::FAILURE) {
            start_time_ = -1.0;
        }
        
        return child_status;
    }
    
    void reset() override {
        DecoratorNode::reset();
        start_time_ = -1.0;
    }
    
private:
    f64 timeout_duration_;
    f64 start_time_;
};

/**
 * @brief Leaf Nodes - Action and condition nodes
 */

// Base action node
class ActionNode : public BehaviorNode {
public:
    using ActionFunc = std::function<NodeStatus(ExecutionContext&)>;
    
    explicit ActionNode(const std::string& name, ActionFunc action = nullptr)
        : BehaviorNode(name), action_func_(action) {}
    
    NodeStatus update(ExecutionContext& context) override {
        if (action_func_) {
            return action_func_(context);
        }
        return NodeStatus::FAILURE;
    }
    
    void set_action(ActionFunc action) {
        action_func_ = action;
    }
    
private:
    ActionFunc action_func_;
};

// Base condition node
class ConditionNode : public BehaviorNode {
public:
    using ConditionFunc = std::function<bool(ExecutionContext&)>;
    
    explicit ConditionNode(const std::string& name, ConditionFunc condition = nullptr)
        : BehaviorNode(name), condition_func_(condition) {}
    
    NodeStatus update(ExecutionContext& context) override {
        if (condition_func_) {
            return condition_func_(context) ? NodeStatus::SUCCESS : NodeStatus::FAILURE;
        }
        return NodeStatus::FAILURE;
    }
    
    void set_condition(ConditionFunc condition) {
        condition_func_ = condition;
    }
    
private:
    ConditionFunc condition_func_;
};

// Wait node - Succeeds after specified duration
class WaitNode : public BehaviorNode {
public:
    explicit WaitNode(const std::string& name = "Wait", f64 duration = 1.0)
        : BehaviorNode(name), duration_(duration), start_time_(-1.0) {}
    
    NodeStatus update(ExecutionContext& context) override {
        f64 current_time = context.total_time;
        
        if (start_time_ < 0) {
            start_time_ = current_time;
        }
        
        if (current_time - start_time_ >= duration_) {
            return NodeStatus::SUCCESS;
        }
        
        return NodeStatus::RUNNING;
    }
    
    void reset() override {
        BehaviorNode::reset();
        start_time_ = -1.0;
    }
    
private:
    f64 duration_;
    f64 start_time_;
};

/**
 * @brief Behavior Tree
 */
class BehaviorTree {
public:
    explicit BehaviorTree(const std::string& name = "BehaviorTree")
        : name_(name), is_running_(false) {}
    
    // Tree management
    void set_root(std::shared_ptr<BehaviorNode> root) {
        root_ = root;
    }
    
    std::shared_ptr<BehaviorNode> get_root() const {
        return root_;
    }
    
    // Execution
    NodeStatus execute(ExecutionContext& context) {
        if (!root_) {
            return NodeStatus::FAILURE;
        }
        
        is_running_ = true;
        context.execution_count++;
        
        NodeStatus result = root_->execute(context);
        
        if (result != NodeStatus::RUNNING) {
            is_running_ = false;
        }
        
        return result;
    }
    
    // Control
    void reset() {
        if (root_) {
            root_->reset();
        }
        is_running_ = false;
    }
    
    void abort() {
        if (root_) {
            root_->abort();
        }
        is_running_ = false;
    }
    
    // Status
    bool is_running() const { return is_running_; }
    const std::string& name() const { return name_; }
    
    // Debug
    void print_tree() const {
        std::cout << "Behavior Tree: " << name_ << "\n";
        if (root_) {
            root_->print_tree();
        }
    }
    
private:
    std::string name_;
    std::shared_ptr<BehaviorNode> root_;
    bool is_running_;
};

/**
 * @brief Behavior Tree Builder - Fluent interface for constructing trees
 */
class BehaviorTreeBuilder {
public:
    explicit BehaviorTreeBuilder(const std::string& name = "BehaviorTree")
        : tree_(std::make_shared<BehaviorTree>(name)) {}
    
    // Root node
    BehaviorTreeBuilder& root(std::shared_ptr<BehaviorNode> node) {
        tree_->set_root(node);
        current_parent_ = node;
        return *this;
    }
    
    // Composite nodes
    BehaviorTreeBuilder& sequence(const std::string& name = "Sequence") {
        auto node = std::make_shared<SequenceNode>(name);
        add_child(node);
        current_parent_ = node;
        return *this;
    }
    
    BehaviorTreeBuilder& selector(const std::string& name = "Selector") {
        auto node = std::make_shared<SelectorNode>(name);
        add_child(node);
        current_parent_ = node;
        return *this;
    }
    
    BehaviorTreeBuilder& parallel(const std::string& name = "Parallel",
                                 ParallelNode::Policy policy = ParallelNode::Policy::REQUIRE_ALL,
                                 u32 required_count = 0) {
        auto node = std::make_shared<ParallelNode>(name, policy, required_count);
        add_child(node);
        current_parent_ = node;
        return *this;
    }
    
    // Decorator nodes
    BehaviorTreeBuilder& inverter(const std::string& name = "Inverter") {
        auto node = std::make_shared<InverterNode>(name);
        add_child(node);
        current_parent_ = node;
        return *this;
    }
    
    BehaviorTreeBuilder& repeat(u32 count, const std::string& name = "Repeat") {
        auto node = std::make_shared<RepeatNode>(name, count);
        add_child(node);
        current_parent_ = node;
        return *this;
    }
    
    BehaviorTreeBuilder& cooldown(f64 duration, const std::string& name = "Cooldown") {
        auto node = std::make_shared<CooldownNode>(name, duration);
        add_child(node);
        current_parent_ = node;
        return *this;
    }
    
    BehaviorTreeBuilder& timeout(f64 duration, const std::string& name = "Timeout") {
        auto node = std::make_shared<TimeoutNode>(name, duration);
        add_child(node);
        current_parent_ = node;
        return *this;
    }
    
    // Leaf nodes
    BehaviorTreeBuilder& action(const std::string& name, ActionNode::ActionFunc action) {
        auto node = std::make_shared<ActionNode>(name, action);
        add_child(node);
        return *this;
    }
    
    BehaviorTreeBuilder& condition(const std::string& name, ConditionNode::ConditionFunc condition) {
        auto node = std::make_shared<ConditionNode>(name, condition);
        add_child(node);
        return *this;
    }
    
    BehaviorTreeBuilder& wait(f64 duration, const std::string& name = "Wait") {
        auto node = std::make_shared<WaitNode>(name, duration);
        add_child(node);
        return *this;
    }
    
    // Navigation
    BehaviorTreeBuilder& end() {
        if (!parent_stack_.empty()) {
            current_parent_ = parent_stack_.top();
            parent_stack_.pop();
        }
        return *this;
    }
    
    // Build
    std::shared_ptr<BehaviorTree> build() {
        return tree_;
    }
    
private:
    std::shared_ptr<BehaviorTree> tree_;
    std::shared_ptr<BehaviorNode> current_parent_;
    std::stack<std::shared_ptr<BehaviorNode>> parent_stack_;
    
    void add_child(std::shared_ptr<BehaviorNode> child) {
        if (current_parent_) {
            if (current_parent_->is_composite()) {
                current_parent_->add_child(child);
            } else if (current_parent_->is_decorator()) {
                auto decorator = std::dynamic_pointer_cast<DecoratorNode>(current_parent_);
                if (decorator) {
                    decorator->set_child(child);
                }
            }
            parent_stack_.push(current_parent_);
        } else {
            tree_->set_root(child);
        }
    }
};

} // namespace ecscope::ai