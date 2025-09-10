#include "jobs/job_dependency_graph.hpp"
#include "jobs/fiber.hpp"
#include <algorithm>
#include <sstream>
#include <fstream>
#include <queue>
#include <unordered_set>
#include <chrono>

namespace ecscope::jobs {

//=============================================================================
// Dependency Node Implementation
//=============================================================================

DependencyNode::DependencyNode(JobID job_id)
    : job_id_(job_id)
    , incoming_count_(0)
    , outgoing_count_(0)
    , incoming_edges_(nullptr)
    , outgoing_edges_(nullptr)
    , is_active_(true)
    , is_completed_(false)
    , reference_count_(1)
    , creation_time_(std::chrono::steady_clock::now())
    , dependency_checks_(0) {
}

DependencyNode::~DependencyNode() {
    cleanup_edges();
}

DependencyNode::DependencyNode(DependencyNode&& other) noexcept
    : job_id_(other.job_id_)
    , incoming_count_(other.incoming_count_.load())
    , outgoing_count_(other.outgoing_count_.load())
    , incoming_edges_(other.incoming_edges_.load())
    , outgoing_edges_(other.outgoing_edges_.load())
    , is_active_(other.is_active_.load())
    , is_completed_(other.is_completed_.load())
    , reference_count_(other.reference_count_.load())
    , creation_time_(other.creation_time_)
    , dependency_checks_(other.dependency_checks_.load()) {
    
    // Clear the moved-from object
    other.incoming_edges_.store(nullptr);
    other.outgoing_edges_.store(nullptr);
    other.reference_count_.store(0);
}

DependencyNode& DependencyNode::operator=(DependencyNode&& other) noexcept {
    if (this != &other) {
        cleanup_edges();
        
        job_id_ = other.job_id_;
        incoming_count_.store(other.incoming_count_.load());
        outgoing_count_.store(other.outgoing_count_.load());
        incoming_edges_.store(other.incoming_edges_.load());
        outgoing_edges_.store(other.outgoing_edges_.load());
        is_active_.store(other.is_active_.load());
        is_completed_.store(other.is_completed_.load());
        reference_count_.store(other.reference_count_.load());
        creation_time_ = other.creation_time_;
        dependency_checks_.store(other.dependency_checks_.load());
        
        other.incoming_edges_.store(nullptr);
        other.outgoing_edges_.store(nullptr);
        other.reference_count_.store(0);
    }
    return *this;
}

bool DependencyNode::add_incoming_edge(DependencyEdge* edge) {
    if (!edge) return false;
    
    // Add to the head of the incoming edges list
    DependencyEdge* current_head = incoming_edges_.load(std::memory_order_acquire);
    do {
        edge->set_next_to_edge(current_head);
    } while (!incoming_edges_.compare_exchange_weak(current_head, edge, std::memory_order_release));
    
    incoming_count_.fetch_add(1, std::memory_order_relaxed);
    return true;
}

bool DependencyNode::add_outgoing_edge(DependencyEdge* edge) {
    if (!edge) return false;
    
    // Add to the head of the outgoing edges list
    DependencyEdge* current_head = outgoing_edges_.load(std::memory_order_acquire);
    do {
        edge->set_next_from_edge(current_head);
    } while (!outgoing_edges_.compare_exchange_weak(current_head, edge, std::memory_order_release));
    
    outgoing_count_.fetch_add(1, std::memory_order_relaxed);
    return true;
}

bool DependencyNode::remove_incoming_edge(DependencyEdge* edge) {
    if (!edge) return false;
    
    DependencyEdge* current = incoming_edges_.load(std::memory_order_acquire);
    DependencyEdge* prev = nullptr;
    
    while (current) {
        if (current == edge) {
            DependencyEdge* next = current->next_to_edge();
            
            if (prev) {
                prev->set_next_to_edge(next);
            } else {
                // Removing head
                if (incoming_edges_.compare_exchange_strong(current, next, std::memory_order_release)) {
                    incoming_count_.fetch_sub(1, std::memory_order_relaxed);
                    return true;
                }
                // Retry if CAS failed
                return remove_incoming_edge(edge);
            }
            
            incoming_count_.fetch_sub(1, std::memory_order_relaxed);
            return true;
        }
        
        prev = current;
        current = current->next_to_edge();
    }
    
    return false;
}

bool DependencyNode::remove_outgoing_edge(DependencyEdge* edge) {
    if (!edge) return false;
    
    DependencyEdge* current = outgoing_edges_.load(std::memory_order_acquire);
    DependencyEdge* prev = nullptr;
    
    while (current) {
        if (current == edge) {
            DependencyEdge* next = current->next_from_edge();
            
            if (prev) {
                prev->set_next_from_edge(next);
            } else {
                // Removing head
                if (outgoing_edges_.compare_exchange_strong(current, next, std::memory_order_release)) {
                    outgoing_count_.fetch_sub(1, std::memory_order_relaxed);
                    return true;
                }
                // Retry if CAS failed
                return remove_outgoing_edge(edge);
            }
            
            outgoing_count_.fetch_sub(1, std::memory_order_relaxed);
            return true;
        }
        
        prev = current;
        current = current->next_from_edge();
    }
    
    return false;
}

void DependencyNode::mark_completed() {
    is_completed_.store(true, std::memory_order_release);
}

void DependencyNode::mark_inactive() {
    is_active_.store(false, std::memory_order_release);
}

void DependencyNode::mark_active() {
    is_active_.store(true, std::memory_order_release);
}

void DependencyNode::for_each_incoming_edge(const std::function<void(DependencyEdge*)>& callback) const {
    DependencyEdge* current = incoming_edges_.load(std::memory_order_acquire);
    while (current) {
        DependencyEdge* next = current->next_to_edge();  // Get next before callback
        callback(current);
        current = next;
    }
}

void DependencyNode::for_each_outgoing_edge(const std::function<void(DependencyEdge*)>& callback) const {
    DependencyEdge* current = outgoing_edges_.load(std::memory_order_acquire);
    while (current) {
        DependencyEdge* next = current->next_from_edge();  // Get next before callback
        callback(current);
        current = next;
    }
}

void DependencyNode::release_reference() {
    if (reference_count_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
        delete this;
    }
}

void DependencyNode::cleanup_edges() {
    // This should only be called when the node is being destroyed
    // and no other threads have references to it
    
    DependencyEdge* current = incoming_edges_.load(std::memory_order_relaxed);
    while (current) {
        DependencyEdge* next = current->next_to_edge();
        current->release_reference();
        current = next;
    }
    
    current = outgoing_edges_.load(std::memory_order_relaxed);
    while (current) {
        DependencyEdge* next = current->next_from_edge();
        current->release_reference();
        current = next;
    }
}

//=============================================================================
// Dependency Edge Implementation
//=============================================================================

DependencyEdge::DependencyEdge(const DependencyEdgeInfo& info, DependencyNode* from, DependencyNode* to)
    : info_(info)
    , from_node_(from)
    , to_node_(to)
    , next_from_edge_(nullptr)
    , next_to_edge_(nullptr)
    , is_active_(true)
    , is_resolved_(false)
    , reference_count_(1)
    , evaluation_count_(0)
    , resolution_time_() {
    
    // Add references to the nodes
    if (from_node_) from_node_->add_reference();
    if (to_node_) to_node_->add_reference();
}

DependencyEdge::~DependencyEdge() {
    // Release references to nodes
    if (from_node_) from_node_->release_reference();
    if (to_node_) to_node_->release_reference();
}

bool DependencyEdge::can_be_resolved() const noexcept {
    if (!is_active() || is_resolved()) {
        return false;
    }
    
    // Check if the dependency can be resolved based on its type
    switch (info_.type) {
        case DependencyType::HardDependency:
            return from_node_ && from_node_->is_completed();
            
        case DependencyType::SoftDependency:
            return true;  // Soft dependencies can always be resolved
            
        case DependencyType::AntiDependency:
            return from_node_ && !from_node_->is_active();
            
        case DependencyType::OutputDependency:
            return from_node_ && from_node_->is_completed();
            
        case DependencyType::ResourceDependency:
            return from_node_ && !from_node_->is_active();
            
        default:
            return false;
    }
}

void DependencyEdge::mark_resolved() {
    if (is_resolved_.compare_exchange_strong(false, true, std::memory_order_release)) {
        resolution_time_ = std::chrono::steady_clock::now();
        
        // Call completion callback if set
        if (info_.completion_callback) {
            info_.completion_callback();
        }
    }
}

void DependencyEdge::mark_inactive() {
    is_active_.store(false, std::memory_order_release);
}

void DependencyEdge::release_reference() {
    if (reference_count_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
        delete this;
    }
}

//=============================================================================
// Job Dependency Graph Implementation
//=============================================================================

JobDependencyGraph::JobDependencyGraph(const GraphConfig& config)
    : config_(config)
    , nodes_(std::make_unique<NodeMap>(config_.initial_nodes_capacity))
    , edges_(std::make_unique<EdgeMap>(config_.initial_edges_capacity))
    , node_pool_(config_.node_pool_size)
    , edge_pool_(config_.edge_pool_size)
    , stats_({})
    , last_cycle_check_(std::chrono::steady_clock::now())
    , is_shutting_down_(false)
    , active_operations_(0)
    , cycle_detection_mutex_("DependencyGraph_CycleDetection") {
}

JobDependencyGraph::~JobDependencyGraph() {
    is_shutting_down_.store(true, std::memory_order_release);
    
    // Wait for active operations to complete
    while (active_operations_.load(std::memory_order_acquire) > 0) {
        std::this_thread::yield();
    }
}

bool JobDependencyGraph::add_job(JobID job_id) {
    if (!job_id.is_valid() || is_shutting_down_.load(std::memory_order_acquire)) {
        return false;
    }
    
    active_operations_.fetch_add(1, std::memory_order_acq_rel);
    
    auto node = allocate_node(job_id);
    if (!node) {
        active_operations_.fetch_sub(1, std::memory_order_acq_rel);
        return false;
    }
    
    bool inserted = nodes_->insert(job_id, std::unique_ptr<DependencyNode>(node));
    
    if (inserted) {
        auto current_stats = stats_.load(std::memory_order_relaxed);
        current_stats.total_nodes++;
        stats_.store(current_stats, std::memory_order_relaxed);
    } else {
        deallocate_node(node);
    }
    
    active_operations_.fetch_sub(1, std::memory_order_acq_rel);
    return inserted;
}

bool JobDependencyGraph::remove_job(JobID job_id) {
    if (!job_id.is_valid() || is_shutting_down_.load(std::memory_order_acquire)) {
        return false;
    }
    
    active_operations_.fetch_add(1, std::memory_order_acq_rel);
    
    auto node_ptr = nodes_->find(job_id);
    if (!node_ptr) {
        active_operations_.fetch_sub(1, std::memory_order_acq_rel);
        return false;
    }
    
    DependencyNode* node = node_ptr->get();
    
    // Remove all incoming and outgoing edges
    std::vector<DependencyEdge*> edges_to_remove;
    
    node->for_each_incoming_edge([&](DependencyEdge* edge) {
        edges_to_remove.push_back(edge);
    });
    
    node->for_each_outgoing_edge([&](DependencyEdge* edge) {
        edges_to_remove.push_back(edge);
    });
    
    for (auto* edge : edges_to_remove) {
        remove_dependency(edge->from_job(), edge->to_job());
    }
    
    bool removed = nodes_->erase(job_id);
    
    if (removed) {
        auto current_stats = stats_.load(std::memory_order_relaxed);
        current_stats.total_nodes--;
        stats_.store(current_stats, std::memory_order_relaxed);
    }
    
    active_operations_.fetch_sub(1, std::memory_order_acq_rel);
    return removed;
}

bool JobDependencyGraph::has_job(JobID job_id) const {
    if (!job_id.is_valid()) return false;
    
    return nodes_->find(job_id) != nullptr;
}

DependencyNode* JobDependencyGraph::get_node(JobID job_id) const {
    if (!job_id.is_valid()) return nullptr;
    
    auto node_ptr = nodes_->find(job_id);
    return node_ptr ? node_ptr->get() : nullptr;
}

bool JobDependencyGraph::add_dependency(const DependencyEdgeInfo& edge_info) {
    return add_dependency(edge_info.from_job, edge_info.to_job, edge_info.type, 
                         edge_info.priority, edge_info.description);
}

bool JobDependencyGraph::add_dependency(JobID from_job, JobID to_job, 
                                       DependencyType type, DependencyPriority priority,
                                       const std::string& description) {
    if (!from_job.is_valid() || !to_job.is_valid() || from_job == to_job ||
        is_shutting_down_.load(std::memory_order_acquire)) {
        return false;
    }
    
    active_operations_.fetch_add(1, std::memory_order_acq_rel);
    
    // Check for cycle before adding
    if (config_.enable_cycle_detection && would_create_cycle(from_job, to_job)) {
        auto current_stats = stats_.load(std::memory_order_relaxed);
        current_stats.cycle_prevention_hits++;
        stats_.store(current_stats, std::memory_order_relaxed);
        
        active_operations_.fetch_sub(1, std::memory_order_acq_rel);
        return false;
    }
    
    // Get or create nodes
    auto from_node_ptr = nodes_->find(from_job);
    auto to_node_ptr = nodes_->find(to_job);
    
    if (!from_node_ptr || !to_node_ptr) {
        auto current_stats = stats_.load(std::memory_order_relaxed);
        current_stats.invalid_dependency_attempts++;
        stats_.store(current_stats, std::memory_order_relaxed);
        
        active_operations_.fetch_sub(1, std::memory_order_acq_rel);
        return false;
    }
    
    DependencyNode* from_node = from_node_ptr->get();
    DependencyNode* to_node = to_node_ptr->get();
    
    // Create edge
    DependencyEdgeInfo info(from_job, to_job, type, priority, description);
    auto edge = allocate_edge(info, from_node, to_node);
    
    if (!edge) {
        auto current_stats = stats_.load(std::memory_order_relaxed);
        current_stats.memory_allocation_failures++;
        stats_.store(current_stats, std::memory_order_relaxed);
        
        active_operations_.fetch_sub(1, std::memory_order_acq_rel);
        return false;
    }
    
    // Add edge to nodes
    from_node->add_outgoing_edge(edge);
    to_node->add_incoming_edge(edge);
    
    // Add to edge map
    u64 edge_key = make_edge_key(from_job, to_job);
    bool inserted = edges_->insert(edge_key, std::unique_ptr<DependencyEdge>(edge));
    
    if (inserted) {
        update_statistics_add_dependency();
    } else {
        // Remove from nodes if insertion failed
        from_node->remove_outgoing_edge(edge);
        to_node->remove_incoming_edge(edge);
        deallocate_edge(edge);
    }
    
    active_operations_.fetch_sub(1, std::memory_order_acq_rel);
    return inserted;
}

bool JobDependencyGraph::remove_dependency(JobID from_job, JobID to_job) {
    if (!from_job.is_valid() || !to_job.is_valid() ||
        is_shutting_down_.load(std::memory_order_acquire)) {
        return false;
    }
    
    active_operations_.fetch_add(1, std::memory_order_acq_rel);
    
    u64 edge_key = make_edge_key(from_job, to_job);
    auto edge_ptr = edges_->find(edge_key);
    
    if (!edge_ptr) {
        active_operations_.fetch_sub(1, std::memory_order_acq_rel);
        return false;
    }
    
    DependencyEdge* edge = edge_ptr->get();
    
    // Remove from nodes
    if (edge->from_node()) {
        edge->from_node()->remove_outgoing_edge(edge);
    }
    
    if (edge->to_node()) {
        edge->to_node()->remove_incoming_edge(edge);
    }
    
    // Remove from edge map
    bool removed = edges_->erase(edge_key);
    
    if (removed) {
        update_statistics_remove_dependency();
    }
    
    active_operations_.fetch_sub(1, std::memory_order_acq_rel);
    return removed;
}

bool JobDependencyGraph::has_dependency(JobID from_job, JobID to_job) const {
    if (!from_job.is_valid() || !to_job.is_valid()) {
        return false;
    }
    
    u64 edge_key = make_edge_key(from_job, to_job);
    return edges_->find(edge_key) != nullptr;
}

bool JobDependencyGraph::add_dependencies(const std::vector<DependencyEdgeInfo>& edges) {
    if (edges.empty()) return true;
    
    bool all_successful = true;
    
    for (const auto& edge_info : edges) {
        if (!add_dependency(edge_info)) {
            all_successful = false;
        }
    }
    
    auto current_stats = stats_.load(std::memory_order_relaxed);
    current_stats.batch_operations++;
    stats_.store(current_stats, std::memory_order_relaxed);
    
    return all_successful;
}

bool JobDependencyGraph::remove_dependencies(const std::vector<std::pair<JobID, JobID>>& edges) {
    if (edges.empty()) return true;
    
    bool all_successful = true;
    
    for (const auto& edge_pair : edges) {
        if (!remove_dependency(edge_pair.first, edge_pair.second)) {
            all_successful = false;
        }
    }
    
    auto current_stats = stats_.load(std::memory_order_relaxed);
    current_stats.batch_operations++;
    stats_.store(current_stats, std::memory_order_relaxed);
    
    return all_successful;
}

void JobDependencyGraph::mark_job_completed(JobID job_id) {
    auto node_ptr = nodes_->find(job_id);
    if (node_ptr) {
        DependencyNode* node = node_ptr->get();
        node->mark_completed();
        
        // Mark outgoing edges as resolved if they can be
        node->for_each_outgoing_edge([](DependencyEdge* edge) {
            if (edge->can_be_resolved()) {
                edge->mark_resolved();
            }
        });
    }
}

std::vector<JobID> JobDependencyGraph::get_ready_jobs() const {
    std::vector<JobID> ready_jobs;
    
    nodes_->for_each([&](JobID job_id, const std::unique_ptr<DependencyNode>& node_ptr) {
        DependencyNode* node = node_ptr.get();
        if (node->is_ready() && !node->is_completed()) {
            ready_jobs.push_back(job_id);
        }
    });
    
    return ready_jobs;
}

std::vector<JobID> JobDependencyGraph::get_jobs_ready_after_completion(JobID completed_job) const {
    std::vector<JobID> ready_jobs;
    
    auto completed_node_ptr = nodes_->find(completed_job);
    if (!completed_node_ptr) {
        return ready_jobs;
    }
    
    DependencyNode* completed_node = completed_node_ptr.get();
    
    // Check all jobs that depend on the completed job
    completed_node->for_each_outgoing_edge([&](DependencyEdge* edge) {
        DependencyNode* dependent_node = edge->to_node();
        if (dependent_node && dependent_node->is_ready() && !dependent_node->is_completed()) {
            ready_jobs.push_back(dependent_node->job_id());
        }
    });
    
    return ready_jobs;
}

bool JobDependencyGraph::has_cycle() const {
    FiberLockGuard lock(cycle_detection_mutex_);
    
    std::unordered_set<JobID, JobID::Hash> visited;
    std::unordered_set<JobID, JobID::Hash> recursion_stack;
    
    auto start_time = std::chrono::steady_clock::now();
    
    bool cycle_found = false;
    nodes_->for_each([&](JobID job_id, const std::unique_ptr<DependencyNode>& node_ptr) {
        if (cycle_found) return;  // Early exit if cycle found
        
        if (visited.find(job_id) == visited.end()) {
            if (has_cycle_from_node(job_id, visited, recursion_stack)) {
                cycle_found = true;
            }
        }
    });
    
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    const_cast<JobDependencyGraph*>(this)->update_statistics_cycle_check(duration, cycle_found);
    
    return cycle_found;
}

bool JobDependencyGraph::would_create_cycle(JobID from_job, JobID to_job) const {
    // Check if adding edge from_job -> to_job would create a cycle
    // This means checking if there's already a path from to_job to from_job
    
    std::unordered_set<JobID, JobID::Hash> visited;
    std::vector<JobID> path;
    
    return has_path_between_jobs(to_job, from_job, visited);
}

std::vector<JobID> JobDependencyGraph::find_cycle_path(JobID start_job) const {
    std::unordered_set<JobID, JobID::Hash> visited;
    std::vector<JobID> path;
    
    if (find_cycle_path_from_job(start_job, visited, path)) {
        return path;
    }
    
    return {};
}

std::vector<std::vector<JobID>> JobDependencyGraph::find_all_cycles() const {
    FiberLockGuard lock(cycle_detection_mutex_);
    
    std::vector<std::vector<JobID>> all_cycles;
    std::unordered_set<JobID, JobID::Hash> visited;
    
    nodes_->for_each([&](JobID job_id, const std::unique_ptr<DependencyNode>& node_ptr) {
        if (visited.find(job_id) == visited.end()) {
            std::vector<JobID> current_path;
            find_all_cycles_from_node(job_id, visited, current_path, all_cycles);
        }
    });
    
    return all_cycles;
}

std::vector<JobID> JobDependencyGraph::get_dependencies(JobID job_id) const {
    std::vector<JobID> dependencies;
    
    auto node_ptr = nodes_->find(job_id);
    if (!node_ptr) return dependencies;
    
    DependencyNode* node = node_ptr.get();
    node->for_each_incoming_edge([&](DependencyEdge* edge) {
        dependencies.push_back(edge->from_job());
    });
    
    return dependencies;
}

std::vector<JobID> JobDependencyGraph::get_dependents(JobID job_id) const {
    std::vector<JobID> dependents;
    
    auto node_ptr = nodes_->find(job_id);
    if (!node_ptr) return dependents;
    
    DependencyNode* node = node_ptr.get();
    node->for_each_outgoing_edge([&](DependencyEdge* edge) {
        dependents.push_back(edge->to_job());
    });
    
    return dependents;
}

std::vector<JobID> JobDependencyGraph::topological_sort() const {
    std::vector<JobID> result;
    std::unordered_map<JobID, u32, JobID::Hash> in_degree;
    std::queue<JobID> ready_queue;
    
    // Calculate in-degrees
    nodes_->for_each([&](JobID job_id, const std::unique_ptr<DependencyNode>& node_ptr) {
        in_degree[job_id] = node_ptr->incoming_count();
        if (in_degree[job_id] == 0) {
            ready_queue.push(job_id);
        }
    });
    
    // Process jobs in topological order
    while (!ready_queue.empty()) {
        JobID current_job = ready_queue.front();
        ready_queue.pop();
        result.push_back(current_job);
        
        // Reduce in-degree of dependents
        auto dependents = get_dependents(current_job);
        for (JobID dependent : dependents) {
            in_degree[dependent]--;
            if (in_degree[dependent] == 0) {
                ready_queue.push(dependent);
            }
        }
    }
    
    return result;
}

DependencyStats JobDependencyGraph::get_statistics() const {
    auto current_stats = stats_.load(std::memory_order_relaxed);
    
    // Update real-time statistics
    current_stats.total_nodes = nodes_->size();
    current_stats.total_edges = edges_->size();
    current_stats.memory_used_bytes = current_stats.total_nodes * sizeof(DependencyNode) +
                                     current_stats.total_edges * sizeof(DependencyEdge);
    
    return current_stats;
}

void JobDependencyGraph::reset_statistics() {
    DependencyStats empty_stats = {};
    stats_.store(empty_stats, std::memory_order_relaxed);
}

std::string JobDependencyGraph::export_graphviz() const {
    std::ostringstream dot;
    dot << "digraph JobDependencies {\\n";
    dot << "  rankdir=TB;\\n";
    dot << "  node [shape=box];\\n";
    
    // Add nodes
    nodes_->for_each([&](JobID job_id, const std::unique_ptr<DependencyNode>& node_ptr) {
        const DependencyNode* node = node_ptr.get();
        std::string color = node->is_completed() ? "green" : 
                          (node->is_ready() ? "yellow" : "red");
        
        dot << "  job_" << job_id.index << " [label=\\"Job " << job_id.index 
            << "\\" color=" << color << "];\\n";
    });
    
    // Add edges
    edges_->for_each([&](u64 edge_key, const std::unique_ptr<DependencyEdge>& edge_ptr) {
        (void)edge_key;  // Suppress unused parameter warning
        const DependencyEdge* edge = edge_ptr.get();
        
        std::string style = edge->is_resolved() ? "dashed" : "solid";
        std::string color;
        
        switch (edge->type()) {
            case DependencyType::HardDependency: color = "black"; break;
            case DependencyType::SoftDependency: color = "blue"; break;
            case DependencyType::AntiDependency: color = "red"; break;
            case DependencyType::OutputDependency: color = "green"; break;
            case DependencyType::ResourceDependency: color = "orange"; break;
        }
        
        dot << "  job_" << edge->from_job().index << " -> job_" 
            << edge->to_job().index << " [color=" << color 
            << " style=" << style << "];\\n";
    });
    
    dot << "}\\n";
    return dot.str();
}

usize JobDependencyGraph::node_count() const {
    return nodes_->size();
}

usize JobDependencyGraph::edge_count() const {
    return edges_->size();
}

bool JobDependencyGraph::is_empty() const {
    return nodes_->size() == 0;
}

//=============================================================================
// Private Helper Methods
//=============================================================================

u64 JobDependencyGraph::make_edge_key(JobID from_job, JobID to_job) const {
    return (static_cast<u64>(from_job.index) << 32) | 
           (static_cast<u64>(from_job.generation) << 16) |
           (static_cast<u64>(to_job.index) << 16) | to_job.generation;
}

DependencyEdge* JobDependencyGraph::find_edge(JobID from_job, JobID to_job) const {
    u64 edge_key = make_edge_key(from_job, to_job);
    auto edge_ptr = edges_->find(edge_key);
    return edge_ptr ? edge_ptr->get() : nullptr;
}

bool JobDependencyGraph::has_cycle_from_node(JobID start_node, 
                                            std::unordered_set<JobID, JobID::Hash>& visited,
                                            std::unordered_set<JobID, JobID::Hash>& recursion_stack) const {
    visited.insert(start_node);
    recursion_stack.insert(start_node);
    
    auto dependents = get_dependents(start_node);
    for (JobID dependent : dependents) {
        if (recursion_stack.find(dependent) != recursion_stack.end()) {
            return true;  // Back edge found - cycle detected
        }
        
        if (visited.find(dependent) == visited.end()) {
            if (has_cycle_from_node(dependent, visited, recursion_stack)) {
                return true;
            }
        }
    }
    
    recursion_stack.erase(start_node);
    return false;
}

DependencyNode* JobDependencyGraph::allocate_node(JobID job_id) {
    try {
        return node_pool_.acquire(job_id);
    } catch (...) {
        return nullptr;
    }
}

void JobDependencyGraph::deallocate_node(DependencyNode* node) {
    if (node) {
        node_pool_.release(node);
    }
}

DependencyEdge* JobDependencyGraph::allocate_edge(const DependencyEdgeInfo& info, 
                                                 DependencyNode* from, DependencyNode* to) {
    try {
        return edge_pool_.acquire(info, from, to);
    } catch (...) {
        return nullptr;
    }
}

void JobDependencyGraph::deallocate_edge(DependencyEdge* edge) {
    if (edge) {
        edge_pool_.release(edge);
    }
}

void JobDependencyGraph::update_statistics_add_dependency() {
    auto current_stats = stats_.load(std::memory_order_relaxed);
    current_stats.dependency_additions++;
    current_stats.total_edges++;
    stats_.store(current_stats, std::memory_order_relaxed);
}

void JobDependencyGraph::update_statistics_remove_dependency() {
    auto current_stats = stats_.load(std::memory_order_relaxed);
    current_stats.dependency_removals++;
    if (current_stats.total_edges > 0) {
        current_stats.total_edges--;
    }
    stats_.store(current_stats, std::memory_order_relaxed);
}

void JobDependencyGraph::update_statistics_cycle_check(std::chrono::microseconds duration, bool cycle_found) {
    auto current_stats = stats_.load(std::memory_order_relaxed);
    current_stats.cycle_detections++;
    if (cycle_found) {
        current_stats.cycles_found++;
    }
    
    // Update average cycle check time (simplified exponential moving average)
    f64 alpha = 0.1;  // Smoothing factor
    current_stats.average_cycle_check_time_us = 
        alpha * static_cast<f64>(duration.count()) + 
        (1.0 - alpha) * current_stats.average_cycle_check_time_us;
    
    stats_.store(current_stats, std::memory_order_relaxed);
}

} // namespace ecscope::jobs