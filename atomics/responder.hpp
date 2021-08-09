#ifndef RESPONDER_HPP
#define RESPONDER_HPP

/*
Current functionality:
- Relay impulse messages from the impulse_in port through the responder_out port.

Notes:
- Positions in this module are not guarenteed to be up to date if the RI module is active since the
  responder cannot calculate positions on its own and subV does not report back to this module.
  - May be wise to remove position data from the responder all together and just pass the position
    data from collision messages to where they are needed
    - IMPLEMENTED
*/

#include <cadmium/modeling/ports.hpp>
#include <cadmium/modeling/message_bag.hpp>

#include <limits>
#include <assert.h>
#include <string>
#include <stdlib.h>
#include <vector>
#include <random>
#include <math.h>
#include <map>
#include <nlohmann/json.hpp>
#include <set>

#include "../test/tags.hpp"  // debug tags
#include "../utilities/vector_utils.hpp"  // vector functions

#include "../data_structures/message.hpp"
#include "../data_structures/node.hpp"

using namespace cadmium;
using namespace std;

using json = nlohmann::json;

// Port definition
struct Responder_defs {
    //struct transition_in : public in_port<___> {};
    struct impulse_in : public in_port<message_t> {};
    struct collision_in : public in_port<collision_message_t> {};
    //struct attachment_out : public out_port<___> {};  // not applicable for now (tethering)
    //struct detachment_out : public out_port<___> {};  // not applicable for now (tethering)
    //struct impulse_out : public out_port<message_t> {};  // unsure of purpose (message type may be wrong)
    //struct loading_out : public out_port<___> {};
    //struct restitution_out : public out_port<___> {};
    struct response_out : public out_port<message_t> {};
};

// comparator for sorting node pointers
// sort by time first (this is what we really want)
// to prevent nodes with the same time being considered duplicates, sort by the first collider then the second collider as well
struct NodePtrComp {
    bool operator() (const Node* lhs, const Node* rhs) const {
        if (lhs->getRest() == rhs->getRest()) {
            if (lhs->getColliders().first == rhs->getColliders().first) {
                return lhs->getColliders().second < rhs->getColliders().second;
            }
            else {
                return lhs->getColliders().first < rhs->getColliders().first;
            }
        }
        else {
            return lhs->getRest() < rhs->getRest();
        }
    }
};

template<typename TIME> class Responder {
    public:
        // temporary assignments
        //

        // ports definition
        /*
        using input_ports = tuple<typename Responder_defs::transition_in,
                                  typename Responder_defs::impulse_in,
                                  typename Responder_defs::collision_in>;
        using output_ports = tuple<typename Responder_defs::attachment_out,
                                   typename Responder_defs::detachment_out,
                                   typename Responder_defs::impulse_out,
                                   typename Responder_defs::loading_out,
                                   typename Responder_defs::restitution_out,
                                   typename Responder_defs::response_out>;
        */
        using input_ports = tuple<typename Responder_defs::impulse_in,
                                  typename Responder_defs::collision_in>;
        using output_ports = tuple<typename Responder_defs::response_out>;

        struct state_type {
            // particle mass, position, velocity
            //map
            json particle_data;
            TIME next_internal;
            vector<message_t> collision_messages;
            vector<message_t> ri_messages;
            TIME current_time;
            //bool received_collision;  // whether or not the last event was a collision being received
            bool sending_ri;
            Node* buffer;  // storage for the node being restituted (will be released in the int that follows restitution velocities being sent)

            // ID_loaded
            // this is a property of each particle in the thesis
            unordered_map<int, unordered_set<int>> id_loaded;  // formatted: {pID, {directly loaded particles}}

            // loading order tree storage (also stores impulses and restitution times)
            set<Node*, NodePtrComp> loading_trees;

            // restitution impulses ("loaded" particle property)
            //unordered_map<int, vector<int>> restitution_impulses;  // formatted: {pID, {impulses of directly loaded particles}}
        };
        state_type state;

        Responder () {
            //
        }

        Responder (json j) {
            if (DEBUG_RE) cout << "Responder constructor received JSON: " << j << endl;
            state.particle_data = j;
            state.next_internal = TIME();
            state.current_time = TIME();
            state.sending_ri = false;
            state.buffer = NULL;
            //cout << "finished ctor" << endl;
        }

        // internal transition
        void internal_transition () {
            if (DEBUG_RE) cout << "resp internal transition called" << endl;
            state.current_time += state.next_internal;
            // TODO: simulate particle decay (possibly in another module)
            state.ri_messages.clear();

            // set this before the following check (to make sure that it gets set before returning)
            if (state.loading_trees.size() > 0) {
                state.next_internal = (*state.loading_trees.begin())->getRest() - state.current_time;
                if (DEBUG_RI) cout << "resp internal_transition: setting next_internal to: " << state.next_internal << endl;
            }
            else {
                if (DEBUG_RI) cout << "resp internal_transition: passivating (no restitutions to perform)" << endl;
                state.next_internal = numeric_limits<TIME>::infinity();
            }

            // if we are here because of an RI, we just need to set next_internal and change the flag
            if (state.sending_ri) {
                if (DEBUG_RI) cout << "resp internal_transition: prematurely returning (received RI)" << endl;
                state.sending_ri = false;
                return;
            }

            // if the buffer is not empty, preform destructive operations on the loading tree and set buffer to NULL
            // also change the velocities of particles to be the post-restitution velocities
            // this is where restitution happens in the responder
            if (state.buffer != NULL) {
                if (DEBUG_RI) cout << "resp internal_transition: buffer is not NULL (perform restitution from previous internal transition)" << endl;

                // add children back into tree (they have the same restitutionTime as parent (set in Node.addChild))
                for (Node* child : state.buffer->getChildren()) {
                    state.loading_trees.insert(child);
                }

                // remove buffer node from tree
                state.loading_trees.erase(state.buffer);

                // change velocities of involved particles (from messages)
                for (message_t message : state.collision_messages) {
                    for (int p_id : message.particle_ids) {
                        state.particle_data[to_string(p_id)]["velocity"] = message.data;
                    }
                }

                // remove associations in state.id_loaded
                state.id_loaded[state.buffer->getColliders().first].erase(state.buffer->getColliders().second);
                state.id_loaded[state.buffer->getColliders().second].erase(state.buffer->getColliders().first);

                // remove now restituted node
                delete state.buffer;

                // clear buffer
                state.buffer = NULL;
            }

            // this must be done after the above tree/velocity operations as the messages as used
            // this will not happen if state.sending_ri is true (since, if true, the function returns earlier)
            state.collision_messages.clear();  // can be done here since output is called before internal transition in Cadmium

            // print state of state.loading_trees before additions or removals
            if (DEBUG_RI) display_loading_trees_debug("internal_transition");

            // if there are no restitutions in the future, resp is passivated (earlier in the function)
            // otherwise, prepare the next restitution
            if (state.loading_trees.size() > 0) {
                // the following should happen on the internal transition AFTER output sends the messages pertaining to the node in the buffer
                // here, we find the next node to be restituted and store that information (messages and buffer)

                if (DEBUG_RI) cout << "resp internal_transition: calculating/preparing next restitution" << endl;

                Node* curr_node = *state.loading_trees.begin();  // get the node with the nearest restitution time
                state.buffer = curr_node;  // store so that modifications can be made to the tree after the messages are sent
                if (DEBUG_RI) cout << "resp internal_transition: messages being prepared for node: " << *curr_node << endl;

                // get loading group data
                vector<int> p_ids = {curr_node->getColliders().second, curr_node->getColliders().first};
                vector<scan_result_t> group_data;
                group_data.push_back(scan_branch(p_ids[0], p_ids[1]));
                group_data.push_back(scan_branch(p_ids[1], p_ids[0]));

                // calculate new velocities for children
                vector<vector<float>> velocities;
                velocities.push_back(VectorUtils::element_op(state.particle_data[to_string(p_ids[0])]["velocity"],
                                                             VectorUtils::element_dist(curr_node->getImpulse(), group_data[0].mass, VectorUtils::divide),
                                                             VectorUtils::add));
                velocities.push_back(VectorUtils::element_op(state.particle_data[to_string(p_ids[1])]["velocity"],
                                                             VectorUtils::element_dist(curr_node->getImpulse(), group_data[1].mass, VectorUtils::divide),
                                                             VectorUtils::subtract));

                // at this point, we just need to prepare messages
                // no velocities should be changed until restitution occurs
                // these messages will be sent to the detector and also used to set the velocities in the responder
                for (unsigned int i = 0; i < velocities.size(); ++i) {
                    state.collision_messages.push_back(message_t(velocities[i], group_data[i].ids, "rest"));  // string is for the message purpose (mainly logging purposes)
                }

                // next_internal is set earlier (before RI check to make sure that it gets set)
            }
            if (DEBUG_RE) cout << "resp internal transition finished" << endl;
        }

        // external transition
        void external_transition (TIME e, typename make_message_bags<input_ports>::type mbs) {
            if (DEBUG_RE) cout << "resp external transition called" << endl;

            state.current_time += e;

            if (get_messages<typename Responder_defs::impulse_in>(mbs).size() > 1) {
                assert(false && "Responder received more than one concurrent message");
            }
            // Handle impulse messages from RI
            for (const auto &x : get_messages<typename Responder_defs::impulse_in>(mbs)) {
                if (DEBUG_RE) cout << "NOTE: responder received impulse: " << x << endl;

                if (x.particle_ids.size() == 0) continue;  // received an uninitialized message

                // get data on the particles that are loaded with the particle receiving the impulse (they must all be changed as well)
                scan_result_t group_data = scan_branch(x.particle_ids[0], -1);

                // calculate the new velocity (since all particles effected have the same velocity, we can use any of them for velocity)
                vector<float> newVelocity;
                //float particle_mass = state.particle_data[to_string(x.particle_ids[0])]["mass"];
                newVelocity = VectorUtils::element_op(state.particle_data[to_string(x.particle_ids[0])]["velocity"],
                                                      VectorUtils::element_dist(x.data, group_data.mass, VectorUtils::divide),
                                                      VectorUtils::add);

                for (int id : group_data.ids) {
                    state.particle_data[to_string(id)]["velocity"] = newVelocity;  // record velocity change in resp particle model
                }
                state.ri_messages.push_back(message_t(newVelocity, group_data.ids, "ri"));  // string is for the message purpose (mainly logging purposes)

                state.sending_ri = true;

                // set next internal (adjust it to keep stay accurate for the next restitution)
                // if there is a collision message, this value may be overridden
                //state.next_internal -= e;
                state.next_internal = 0;  // immediately respond to impulse
            }

            // Handle collision messages
            for (const auto &x : get_messages<typename Responder_defs::collision_in>(mbs)) {
                if (DEBUG_RE) cout << "resp external transition: responder received collision: " << x << endl;

                state.buffer = NULL;  // do not manipulate the tree for this node
                state.collision_messages.clear();  // prepare messages buffer for next set of velocities

                vector<vector<float>> msg_positions;
                vector<int> p_ids;
                vector<scan_result_t> group_data;

                // grab the particle positions (must happen before impulses are calculated)
                // grab the particle IDs
                // calculate branch data
                for (const auto& [key, val] : x.positions) {
                    msg_positions.push_back(val);
                    p_ids.push_back(key);  // store particle IDs
                    group_data.push_back(scan_branch(key, -1));
                }

                if (p_ids.size() != 2) continue;  // received an uninitialized or malformed message

                // calculate full impulse
                // we keep this impulse in order to calculate the resitution impulse
                vector<float> full_impulse = calc_impulse(p_ids[0], p_ids[1], group_data[0].mass, group_data[1].mass, msg_positions[0], msg_positions[1]);

                // calculate loading impulse
                vector<float> loading_impulse = calc_loading_impulse(p_ids[0], p_ids[1], group_data[0].mass, group_data[1].mass);

                // calculate loading velocity
                vector<float> loading_velocity = calc_loading_velocity(p_ids[0], p_ids[1], group_data[0].mass, group_data[1].mass);

                // calculate restitution impulse
                vector<float> restitution_impulse = VectorUtils::element_op(full_impulse, loading_impulse, VectorUtils::subtract);

                // calculate restitution time
                float restitution_time = state.current_time + 10;  // TODO: THIS NEEDS TO BE CALCULATED (probably from some particle species value(s))

                if (DEBUG_RI) {
                    cout << "resp external_transition: impulses/velocities calculated:" << endl;
                    cout << "|        full impulse: " << VectorUtils::get_string<float>(full_impulse) << endl;
                    cout << "|     loading impulse: " << VectorUtils::get_string<float>(loading_impulse) << endl;
                    cout << "|    loading velocity: " << VectorUtils::get_string<float>(loading_velocity) << endl;
                    cout << "| restitution impulse: " << VectorUtils::get_string<float>(restitution_impulse) << endl;
                }

                // manage loading trees
                // get the trees of the colliding nodes (if there are any)
                vector<Node*> child_trees;
                for (int i = 0; i <= 1; ++i) {
                    if (group_data[i].ids.size() > 1) {
                        for (Node* node : state.loading_trees) {
                            auto result = node->getParticles().find(p_ids[i]);
                            if (result != node->getParticles().end()) {
                                child_trees.push_back(node);
                            }
                        }
                    }
                }

                // print state of state.loading_trees before additions or removals
                if (DEBUG_RI) display_loading_trees_debug("external_transition");

                // create new node and remove older pointers
                Node* newNode = new Node(p_ids[0], p_ids[1], group_data[0].mass + group_data[1].mass, restitution_time, restitution_impulse);
                newNode->addChildren(child_trees);
                state.loading_trees.insert(newNode);
                for (Node* child : child_trees) {
                    state.loading_trees.erase(child);
                }

                if (DEBUG_RI) cout << "resp external_transition: added node to state.loading_trees: " << *newNode << endl;

                // print state of state.loading_trees before additions or removals
                if (DEBUG_RI) display_loading_trees_debug("external_transition");

                // update loading relations
                state.id_loaded[p_ids[0]].insert(p_ids[1]);
                state.id_loaded[p_ids[1]].insert(p_ids[0]);

                if (DEBUG_RE) {
                    cout << "impulse calculation and application:" << endl;
                    cout << "| resp external transition: before setting calculated velocities: (p_id: " << p_ids[0] << ") " << VectorUtils::get_string<float>(state.particle_data[to_string(p_ids[0])]["velocity"]) << endl;
                    cout << "| resp external transition: before setting calculated velocities: (p_id: " << p_ids[1] << ") " << VectorUtils::get_string<float>(state.particle_data[to_string(p_ids[1])]["velocity"]) << endl;
                    cout << "| resp external transition: calculated full impulse: " << VectorUtils::get_string<float>(full_impulse) << endl;
                    //cout << "| resp external transition: new velocity: (p_id: " << p_ids[0] << ") " << VectorUtils::get_string<float>(p1_vel) << endl;
                    //cout << "| resp external transition: new velocity: (p_id: " << p_ids[1] << ") " << VectorUtils::get_string<float>(p2_vel) << endl;
                }

                // all involved particles
                vector<int> involved_ids = VectorUtils::concat<int>(group_data[0].ids, group_data[1].ids);

                // set velocities
                for (int id : involved_ids) {
                    state.particle_data[to_string(id)]["velocity"] = loading_velocity;
                }

                // prepare messages
                state.collision_messages.push_back(message_t(loading_velocity, involved_ids, "load", x.positions));  // string is for the message purpose (mainly logging purposes)

                // set next internal to the smallest restitution time
                state.next_internal = 0;  // immediately send new velocities to subVs
                //state.next_internal = (*state.loading_trees.begin())->getRest() - state.current_time;
            }

            // set next internal
            //state.next_internal = 0;

            if (DEBUG_RE) cout << "resp external transition finish" << endl;
        }

        // confluence transition
        void confluence_transition (TIME e, typename make_message_bags<input_ports>::type mbs) {
            if (DEBUG_RE) cout << "resp confluence transition called" << endl;
            internal_transition();
            external_transition(e, move(mbs));
            if (DEBUG_RE) cout << "resp confluence transition finishing" << endl;
        }

        // output function
        typename make_message_bags<output_ports>::type output () const {
            if (DEBUG_RE) cout << "resp output called" << endl;
            typename make_message_bags<output_ports>::type bags;
            vector<message_t> bag_port_out;
            if (state.sending_ri) {
                bag_port_out = state.ri_messages;
            }
            else {
                bag_port_out = state.collision_messages;
            }
            get_messages<typename Responder_defs::response_out>(bags) = bag_port_out;
            if (DEBUG_RE) {
                cout << "resp output sending: ";
                for (auto i : state.collision_messages) {
                    cout << "{" << i << "}";
                }
                cout << endl;
            }
            if (DEBUG_RE) cout << "resp output returning" << endl;
            return bags;
        }

        // time advance function
        TIME time_advance () const {
            if (DEBUG_RE) cout << "resp time advance called/returning" << endl;
            return state.next_internal;
        }

        /*
        // Temporarily replaced with less verbose version to reduce file size
        friend ostringstream& operator<<(ostringstream& os, const typename Responder<TIME>::state_type& i) {
            if (DEBUG_RE) cout << "resp << called" << endl;
            string result = "particles: ";
            for (auto p_id = i.particle_data.begin(); p_id != i.particle_data.end(); ++p_id) {
                result += "[(p_id:" + p_id.key() + "): ";
                result += "pos" + VectorUtils::get_string<float>(i.particle_data[p_id.key()]["position"], true) + ", ";
                result += "vel" + VectorUtils::get_string<float>(i.particle_data[p_id.key()]["velocity"], true) + "]";
            }
            result += ", velocity messages: ";
            for (auto message : i.messages) {
                result += "[(p_id:" + to_string(message.particle_id) + "): ";
                result += "vel" + VectorUtils::get_string<float>(message.data, true) + "] ";
            }
            os << result;
            if (DEBUG_RE) cout << "resp << returning" << endl;
            return os;
        }
        */

        friend ostringstream& operator<<(ostringstream& os, const typename Responder<TIME>::state_type& i) {
            if (DEBUG_RE) cout << "resp << called" << endl;
            string result = "num particles: " + i.particle_data.size();
            result += ", num collision velocity messages: " + i.collision_messages.size();
            os << result;
            if (DEBUG_RE) cout << "resp << returning" << endl;
            return os;
        }

    private:

        struct scan_result_t {
            float mass;
            vector<int> ids;
            vector<pair<int, int>> pairs;
            scan_result_t (float i_mass, vector<int> i_ids, vector<pair<int, int>> i_pairs) :
                mass(i_mass), ids(i_ids), pairs(i_pairs) {}
        };

        // get the impulse after a collision
        // masses are arguments to allow for combined masses (loading)
        // this function calculates the full collision impulse
        // used to get the restitution impulse
        vector<float> calc_impulse (int p1_id, int p2_id, float p1_mass, float p2_mass, vector<float>& p1_pos, vector<float>& p2_pos) const {
            if (DEBUG_RE) cout << "resp calc_impulse called" << endl;
            vector<float> result;

            float c_restitute = 0.9;  // [0, 1]

            // relative velocity of p1 and p2
            vector<float> v_1_2 = VectorUtils::element_op(state.particle_data[to_string(p2_id)]["velocity"],
                                                          state.particle_data[to_string(p1_id)]["velocity"],
                                                          VectorUtils::subtract);

            // unit vector pointing from p1 to p2
            vector<float> u = VectorUtils::make_unit(VectorUtils::get_vect(p2_pos, p1_pos));
            /*
            vector<float> u = VectorUtils::make_unit(VectorUtils::get_vect(state.particle_data[to_string(p2_id)]["position"],
                                                                           state.particle_data[to_string(p1_id)]["position"]));
            */

            // calculate the component of the relative velocity which is along the vector u
            vector<float> v_u = VectorUtils::get_proj(v_1_2, u);

            // get impulse
            result = VectorUtils::element_dist(v_u, (1 / ((1 / p1_mass) + (1 / p2_mass))) * (1 + c_restitute), VectorUtils::multiply);

            if (DEBUG_RE) cout << "resp calc_impulse returning" << endl;
            return result;
        }

        // calculate loading velocity
        vector<float> calc_loading_velocity (int p1_id, int p2_id, float p1_mass, float p2_mass) {
            return VectorUtils::element_op(
                VectorUtils::element_dist(
                    state.particle_data[to_string(p1_id)]["velocity"],
                    (1 / (1 + (p2_mass / p1_mass))),
                    VectorUtils::multiply
                ),
                VectorUtils::element_dist(
                    state.particle_data[to_string(p2_id)]["velocity"],
                    (1 / (1 + (p1_mass / p2_mass))),
                    VectorUtils::multiply
                ),
                VectorUtils::add
            );
        }

        // calculate loading impulse (used to calculate restitution impulse)
        vector<float> calc_loading_impulse (int p1_id, int p2_id, float p1_mass, float p2_mass) {
            return VectorUtils::element_dist(
                VectorUtils::element_op(
                    state.particle_data[to_string(p2_id)]["velocity"],
                    state.particle_data[to_string(p1_id)]["velocity"],
                    VectorUtils::subtract
                ),
                p1_mass + p2_mass,
                VectorUtils::multiply
            );
        }

        // scan branch
        // adapted from original TPS code (v1.4)
        scan_result_t scan_branch (int id_a, int id_src) {
            float mass_a = state.particle_data[to_string(id_a)]["mass"];
            vector<int> ids_a = {id_a};
            vector<pair<int, int>> pairs_a;
            for (int id_b : state.id_loaded[id_a]) {
                if (id_b != id_src) {
                    scan_result_t result = scan_branch(id_b, id_a);
                    mass_a = mass_a + result.mass;
                    for (int id : result.ids) {
                        ids_a.push_back(id);
                    }
                    for (pair<int, int> id_pair : result.pairs) {
                        pairs_a.push_back(id_pair);
                    }
                    pairs_a.push_back({id_a, id_b});
                }
            }
            return scan_result_t(mass_a, ids_a, pairs_a);
        }

        void display_loading_trees_debug (string function_name) {
            cout << "resp " << function_name << ": nodes in state.loading_trees:" << endl;
            for (Node* node : state.loading_trees) {
                cout << "| " << *node << endl;
            }
            if (state.loading_trees.size() == 0) {
                cout << "| state.loading_trees reported size=0" << endl;
            }
        }
};

#endif