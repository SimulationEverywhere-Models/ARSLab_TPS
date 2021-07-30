#include "node.hpp"

Node::Node() {
    //
}

Node::Node(int p1, int p2, float mass, float time, vector<float> impulse) {
    //colliders = minmax(p1, p2);
    colliders = {p1, p2};
    restitutionTime = time;
    this->impulse = impulse;
    particles.insert({p1, p2});
    this->mass = mass;
}

Node::~Node() {
    // we do not want to deallocate child nodes (they must do it themselves)
}

void Node::addChild(Node* node) {
    children.push_back(node);
    adjustTime(this->restitutionTime, node);  // adjust the child and all of its descendants
    for (int pID : node->particles) {
        particles.insert(pID);
    }
}

void Node::addChildren(vector<Node*> nodes) {
    for (Node* node : nodes) {
        addChild(node);
    }
}

vector<Node*> Node::getChildren() {
    return children;
}

// associate each child node with its collider
unordered_map<int, Node*> Node::getChildAssociations() {
    unordered_map<int, Node*> associations;
    vector<int> colliders_list = {colliders.first, colliders.second};

    for (int curr_collider : colliders_list) {
        for (Node* child : children) {
            if (associations.find(curr_collider) == associations.end()) {
                for (int pID : child->particles) {
                    if (pID == curr_collider) {
                        associations[curr_collider] = child;
                        break;
                    }
                }
            }
        }

        if (associations.find(curr_collider) == associations.end()) {
            associations[curr_collider] = NULL;
        }
    }

    return associations;
}

int Node::numChildren() {
    return children.size();
}

void Node::setRest(float time) {
    restitutionTime = time;
}

float Node::getRest() const {
    return restitutionTime;
}

unordered_set<int> Node::getParticles() const {
    return particles;
}

vector<float> Node::getImpulse() const {
    return impulse;
}

float Node::getMass() const {
    return mass;
}

pair<int, int> Node::getColliders() const {
    return colliders;
}

void Node::adjustTime(float time, Node* node) {
    node->restitutionTime = time;
    for (Node* curr_node : node->getChildren()) {
        adjustTime(time, curr_node);
    }
}

bool operator< (const Node& lhs, const Node& rhs) {
    return lhs.restitutionTime < rhs.restitutionTime;
}

ostream& operator<< (ostream& os, const Node& node) {
    os << "head=(" + to_string(node.colliders.first) + "," + to_string(node.colliders.second);
    os << "), mass=" + to_string(node.mass);
    os << ", time=" + to_string(node.restitutionTime);
    os << ", child nodes={";
    for (Node* child : node.children) {
        os << "(" + to_string(child->colliders.first) + "," + to_string(child->colliders.second) + ")";
        os << " ";
    }
    os << "}, all particles={";
    for (int id : node.particles) {
        os << id;
        os << " ";
    }
    os << "}";
    return os;
}