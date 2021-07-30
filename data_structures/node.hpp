#ifndef NODE
#define NODE

#include <iostream>
#include <vector>
#include <algorithm>
#include <unordered_set>
#include <unordered_map>

using namespace std;

class Node {
    public:
        Node ();
        Node (int p1, int p2, float mass, float time, vector<float> impulse);
        ~Node ();
        void addChild (Node* node);
        void addChildren (vector<Node*> nodes);
        vector<Node*> getChildren ();
        unordered_map<int, Node*> getChildAssociations ();
        int numChildren ();
        void setRest (float time);
        float getRest () const;
        unordered_set<int> getParticles () const;
        vector<float> getImpulse () const;
        float getMass () const;
        pair<int, int> getColliders () const;
        friend bool operator< (const Node& lhs, const Node& rhs);
        friend ostream& operator<< (ostream& os, const Node& node);
    private:
        pair<int, int> colliders;  // the two particles that collided (min, max)
        vector<Node*> children;  // child nodes
        unordered_set<int> particles;  // particles related to this node and its descendants
        vector<float> impulse;  // impulse associated with the collision that created the node
        float restitutionTime;  // the time that resitution happens at (according to the responder)
        float mass;  // mass of this node and its descendants
        void adjustTime (float time, Node* node);  // adjust time of all descendant nodes
};

#endif