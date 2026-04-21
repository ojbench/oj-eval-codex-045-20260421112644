#include <iostream>
#include <cstdio>

using namespace std;

/*
We implement an elevator with operations:
 - add x: add request at floor x (x != pos), unique per floor
 - cancel x: cancel request at floor x (guaranteed exists)
 - action: move to next request by rules
 - locate: print current position

Rules:
 - Keep direction if there exists request in current direction; go to the closest in that direction
 - Else reverse direction and go to the closest in the new direction
 - If no requests, do nothing

Constraints: n up to 5e5; floors up to 1e9; locate <= 1000 per test point.

We must restrict headers to cstdio/iostream only. Hence implement our own structures.

Approach: Maintain two binary heaps with lazy deletion and a presence map via open addressing hash set.
 - up-heap: min-heap of floors >= pos (requests above)
 - down-heap: max-heap of floors <= pos (requests below)
 But direction selection requires the nearest in direction relative to current position, not globally since pos moves.

Observation: At any moment, to select next in current direction, we need the closest request on that side:
 - up side: minimal x > pos
 - down side: maximal x < pos

If we keep all requests in two heaps partitioned relative to current pos, when pos changes we need to rebalance heaps.
To avoid O(k) rebalancing, we will maintain a balanced BST. But we cannot include <set>. So we roll our own Treap.

Treap with keys = floor numbers. Operations: insert, erase, predecessor, successor.
All O(log n) expected.

Implementation details:
 - Treap nodes allocated from static pool to avoid new/delete overhead.
 - Random priority via simple xorshift.
 - For each action:
    * If treap empty: do nothing
    * Else find next:
        - If dir = +1: find successor(pos). If exists -> go there
          else dir = -1, find predecessor(pos) and go there (if exists)
        - If dir = -1: predecessor first; else reverse and successor
    * After moving, erase that floor from treap and set pos to it. Direction updated as per rule: remains same if moved in that direction; otherwise reversed.

Edge cases:
 - add x guarantees x != pos and no duplicate present
 - cancel x guarantees exists
 - Requests can be at floor 0..1e9
*/

struct RNG {
    unsigned int x;
    RNG(): x(2463534242u) {}
    unsigned int next() {
        // xorshift32
        unsigned int y = x;
        y ^= y << 13;
        y ^= y >> 17;
        y ^= y << 5;
        x = y;
        return y;
    }
};

struct Node {
    int key;
    unsigned int pri;
    int l, r;
};

static const int MAXN = 600000 + 100000; // buffer
Node pool[MAXN];
int tot = 0;
int root = 0;
RNG rng;

int new_node(int key){
    ++tot;
    pool[tot].key = key;
    pool[tot].pri = rng.next();
    pool[tot].l = pool[tot].r = 0;
    return tot;
}

void rotate_left(int &t){
    int r = pool[t].r;
    pool[t].r = pool[r].l;
    pool[r].l = t;
    t = r;
}

void rotate_right(int &t){
    int l = pool[t].l;
    pool[t].l = pool[l].r;
    pool[l].r = t;
    t = l;
}

void treap_insert(int &t, int key){
    if(!t){
        t = new_node(key);
        return;
    }
    if(key < pool[t].key){
        treap_insert(pool[t].l, key);
        if(pool[pool[t].l].pri > pool[t].pri) rotate_right(t);
    } else {
        treap_insert(pool[t].r, key);
        if(pool[pool[t].r].pri > pool[t].pri) rotate_left(t);
    }
}

void treap_erase(int &t, int key){
    if(!t) return;
    if(key < pool[t].key) treap_erase(pool[t].l, key);
    else if(key > pool[t].key) treap_erase(pool[t].r, key);
    else {
        if(!pool[t].l || !pool[t].r){
            t = pool[t].l ? pool[t].l : pool[t].r;
        } else {
            if(pool[pool[t].l].pri > pool[pool[t].r].pri){
                rotate_right(t);
                treap_erase(pool[t].r, key);
            } else {
                rotate_left(t);
                treap_erase(pool[t].l, key);
            }
        }
    }
}

// find successor strictly greater than x
int treap_successor(int t, int x){
    int ans = -1;
    while(t){
        if(pool[t].key > x){
            ans = pool[t].key;
            t = pool[t].l;
        } else t = pool[t].r;
    }
    return ans;
}

// find predecessor strictly less than x
int treap_predecessor(int t, int x){
    int ans = -1;
    while(t){
        if(pool[t].key < x){
            ans = pool[t].key;
            t = pool[t].r;
        } else t = pool[t].l;
    }
    return ans;
}

int main(){
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int n;
    if(!(cin >> n)) return 0;

    long long pos = 0; // current floor
    int dir = 1; // +1 up, -1 down. starts up

    for(int i=0;i<n;i++){
        string op;
        cin >> op;
        if(op == "add"){
            long long x; cin >> x;
            // x != pos guaranteed and unique
            treap_insert(root, (int)x);
        } else if(op == "cancel"){
            long long x; cin >> x;
            treap_erase(root, (int)x);
        } else if(op == "action"){
            if(!root) {
                // nothing
            } else {
                if(dir == 1){
                    int nxt = treap_successor(root, (int)pos);
                    if(nxt != -1){
                        pos = nxt;
                        treap_erase(root, nxt);
                    } else {
                        dir = -1;
                        int prv = treap_predecessor(root, (int)pos);
                        if(prv != -1){
                            pos = prv;
                            treap_erase(root, prv);
                        }
                    }
                } else { // dir == -1
                    int prv = treap_predecessor(root, (int)pos);
                    if(prv != -1){
                        pos = prv;
                        treap_erase(root, prv);
                    } else {
                        dir = 1;
                        int nxt = treap_successor(root, (int)pos);
                        if(nxt != -1){
                            pos = nxt;
                            treap_erase(root, nxt);
                        }
                    }
                }
            }
        } else if(op == "locate"){
            cout << pos << '\n';
        }
    }

    return 0;
}

