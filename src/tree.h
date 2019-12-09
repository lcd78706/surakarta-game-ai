#pragma once
#include <vector>
#include <mutex>
#include <atomic>
#include "board.h"

class TreeNode {
public:
    TreeNode() {}
    TreeNode(const Board &b) : 
        parent(NULL),
        board(b),
        win_count(1),
        visit_count(2),
        // win_rate(0.0f),
        state_value(0.0f),
        softmax_value(1.0f),
        player(0),
        explore(false),
        prev_action(std::make_pair("none", 0)) { child.clear(); }
    TreeNode(const Board &b,
             float state_value,
             float softmax_value,
             int player,
             TreeNode* parent,
             std::pair<std::string, unsigned> prev_action) : 
        parent(parent),
        board(b),
        win_count(1),
        visit_count(2),
        // win_rate(0.0f),
        state_value(state_value),
        softmax_value(softmax_value),
        player(player),
        explore(false),
        prev_action(prev_action) { child.clear(); }

    TreeNode(const TreeNode& node) {std::cerr << "===aaa==="<<std::endl;};
    // TreeNode& operator =(const TreeNode& node) = default;

public:
    TreeNode* get_parent() { return parent; }
    void set_parent(TreeNode *parent) { this->parent = parent; }

    Board& get_board() { return board; }
    const Board& get_board() const { return board; }

    int get_win_count() { return win_count; }
    void add_win_count() { win_count++; }

    int get_visit_count() { return visit_count; }
    void add_visit_count() { visit_count++; }

    // float get_win_rate() { return win_rate; }
    // void update_win_rate(float win_rate) {
    //     this->win_rate += (win_rate - this->win_rate) / (visit_count + 1);
    // }

    float get_state_value() { return state_value; }
    void set_state_value(float state_value) { this->state_value = state_value; }

    float get_softmax_value() { return softmax_value; }
    void set_softmax_value(float softmax_value) { this->softmax_value = softmax_value; }

    float get_child_softmax_total() { return child_softmax_total; }
    void set_child_softmax_total(float child_softmax_total) { this->child_softmax_total = child_softmax_total; }

    int get_player() { return player; }
    void set_player(int player) { this->player = player; }

    std::vector<TreeNode>& get_all_child() { return child; }
    TreeNode& get_child(int index) { return child.at(index); }

    bool is_explore() { return explore; }
    void set_explore() { explore = true; }

    TreeNode& get_best_child_node() {
        // for(auto& i: child) std::cerr << i.visit_count<< std::endl;
        return *std::max_element(child.begin(), child.end(),
                                 [](const TreeNode& A, const TreeNode& B) { return A.visit_count < B.visit_count; });
    }
    TreeNode& get_child_with_temperature(double rd) {
        int total = visit_count;
        int chosen = total * rd;
        for(auto& tmp : child){
            if((chosen -= (tmp.get_visit_count() - 2)) <= 0) return tmp;
        }
        return child.back();
    }

    std::pair<std::string, unsigned> get_prev_action() { return prev_action; }
    std::string get_prev_action_type () { return prev_action.first; }
    unsigned get_prev_action_code () { return prev_action.second; }
public:
    void lock_mutex () {
        while( lmtx.test_and_set(std::memory_order_acquire)) {
            asm volatile ("rep; nop" ::: "memory");
        }
    }
    void unlock_mutex () {
        lmtx.clear(std::memory_order_release);
    }

private:
    TreeNode *parent;
    Board board;
    std::atomic_int win_count;
    std::atomic_int visit_count;
    // float win_rate;
    float state_value; // tuple value
    float softmax_value;
    float child_softmax_total;
    bool player; // current player
    bool explore;
    // std::mutex lmtx;
    std::atomic_flag lmtx = ATOMIC_FLAG_INIT;
    std::vector<TreeNode> child;
    std::pair<std::string, unsigned> prev_action;
};

class Tree {
public:
    Tree(const Board& b) : root(TreeNode(b)) {}
    TreeNode& get_root() { return root; }
public:
    void lock_mutex () {
        gmtx.lock();
    }
    void unlock_mutex () {
        gmtx.unlock();
    }
private:
    TreeNode root;
    std::mutex gmtx;
};
