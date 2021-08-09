/*
subV
Responsibilities
- manage sub-volumes of space
- report when the next collision will occur in its volume
*/

#ifndef SUBV_HPP
#define SUBV_HPP

#include <cadmium/modeling/ports.hpp>
#include <cadmium/modeling/message_bag.hpp>

#include <limits>
#include <assert.h>
#include <string>
#include <cmath>  // abs, sqrt
//#include <algorithm>  // max
#include <map>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include <boost/functional/hash.hpp>

#include "../test/tags.hpp"  // debug tags
#include "../utilities/vector_utils.hpp"  // vector functions

#include "../data_structures/message.hpp"

#define DELTA_T_MAX 10000000  // value larger than any reasonable simulation runtime

using namespace cadmium;
using namespace std;

using json = nlohmann::json;

// Port definition
struct SubV_defs {
    struct response_in : public in_port<tracker_message_t> {};
    struct collision_out : public out_port<collision_message_t> {};
    //struct departure_out : public out_port<___> {};
    //struct arrival_out : public out_port<___> {};
    struct logging_out : public out_port<logging_message_t> {};
};

template<typename TIME> class SubV {
    public:
        // ports definition
        // TODO: add adjacency messages
        /*
        using input_ports = tuple<typename SubV_defs::response_in>;
        using output_ports = tuple<typename SubV_defs::collision_out,
                                   typename SubV_defs::departure_out,
                                   typename SubV_defs::arrival_out>;
        */

        using input_ports = tuple<typename SubV_defs::response_in>;
        using output_ports = tuple<typename SubV_defs::collision_out,
                                   typename SubV_defs::logging_out>;

        struct state_type {
            // state information
            json particle_data;  // particle_id, {postition, velocity, radius}  // needs information on last collision
            int subV_id;
            TIME next_internal;
            TIME current_time;  // current time within a subV module
            map<int, float> particle_times;  // particle_id, time  // last event for each particle in a subV module
            collision_message_t next_collision;
            bool awaiting_response;  // whether or not subV has received a response from the responder (if not, do not preform further calculations until received)
            bool sending_collision;  // whether or not to send a collision (stop message sending is receiving an RI or a response message)
            unordered_map<pair<int, int>, float, boost::hash<pair<int, int>>> collisions_cache;  // cache collision times for non-inf times
            vector<logging_message_t> logging_messages;  // messages that store position for logging purposes
        };
        state_type state;

        SubV () {
            if (DEBUG_SV) cout << "SubV default constructor called" << endl;
        }

        SubV (json j) {
            if (DEBUG_SV) cout << "SubV constructor called" << endl;

            state.particle_data = j;

            // initialization
            // TODO: subV_id should be initialized or calculated from arguments
            state.subV_id = 1;
            state.current_time = TIME();
            state.next_internal = TIME();

            // initialize particle times
            for (auto it = state.particle_data.begin(); it != state.particle_data.end(); ++it) {
                state.particle_times[stoi(it.key())] = state.current_time;
            }

            // populate collision cache
            populate_collision_cache();
        }

        // internal transition
        void internal_transition () {
            if (DEBUG_SV) cout << "subV internal transition called" << endl;

            // update the current time before doing work
            state.current_time += state.next_internal;  // next_internal was set by the previous call to int/ext transition

            // reset flag and clear logging messages (can be done here since output is called before internal transition in Cadmium)
            state.sending_collision = true;
            state.logging_messages.clear();

            if (state.awaiting_response) {
                state.next_internal = numeric_limits<TIME>::infinity();
                if (DEBUG_SV) cout << "subV internal transition: passivating (have not received response message)" << endl;
                return;
            }

            if (state.next_collision.positions.size() == 2) {
                // update collision cache to incorporate new velocities from set of messages from the responder (do this before updating next_collision_data)
                update_collision_cache(get_keys(state.next_collision.positions));  // TODO: account for walls (maybe use negative numbers?)
            }

            // get the next collision
            next_collision_t next_collision_data = get_next_collision();
            state.next_collision = next_collision_data.collision;

            if (DEBUG_SV) cout << "subV internal_transition: next collision in: " << next_collision_data.time << endl;

            // calculate positions but don't incorporate them until new velocities received
            // can't set until next_int (rather, when we get the corresponding velocity message back) in case RI sends a message (in which case, we throw away this calculation)
            for (auto it = state.next_collision.positions.begin(); it != state.next_collision.positions.end(); ++it) {
                state.next_collision.positions[it->first] = position(it->first, state.current_time + next_collision_data.time);
                if (DEBUG_SV) cout << "subV internal transition: setting message position: " << VectorUtils::get_string<float>(state.next_collision.positions[it->first]) << endl;
            }
            assert(state.next_collision.positions.size() == 2 || state.next_collision.positions.size() == 0);  // 0 if inital call without receiving first

            // set next_internal
            state.next_internal = next_collision_data.time;

            state.awaiting_response = true;

            if (DEBUG_SV) cout << "subV internal transition finishing" << endl;
        }

        // external transition
        void external_transition (TIME e, typename make_message_bags<input_ports>::type mbs) {
            if (DEBUG_SV) cout << "subV external transition called" << endl;

            state.awaiting_response = false;
            state.sending_collision = false;  // do not output a collision message if an enternal event is called between calls to internal transition and output
            bool applicable_message_processed = false;

            // update the current time before doing work
            state.current_time += e;

            int numMessages = get_messages<typename SubV_defs::response_in>(mbs).size();
            if (numMessages > 1) {
                if (DEBUG_SV) cout << "NOTE: subV received " << numMessages << " concurrent messages" << endl;
            }

            // TODO: try managing both velocity messages simultaneously (if it is not an RI, of course)
            //       as it may be necessary to update the cache ONLY after BOTH velocities are set

            // Handle velocity messages from tracker
            for (const auto &x : get_messages<typename SubV_defs::response_in>(mbs)) {
                // if the message is relevant to this subV
                if (find(x.subV_ids.begin(), x.subV_ids.end(), state.subV_id) != x.subV_ids.end()) {
                    applicable_message_processed = true;

                    // Calculations should not be done here because, for collisions, there will be two associated messages received (one for each particle involved)
                    // - Only particle_data information should be changed

                    if (DEBUG_SV) cout << "subV external transition: handling impulse: " << (x.purpose == "ri" ? "true" : "false") << endl;
                    if (DEBUG_SV) cout << "subV external transition: handling type: " << x.purpose << endl;
                    if (DEBUG_SV) cout << "subV external transition: current time (subV_id: " << state.subV_id << "): " << state.current_time << endl;

                    // process each particle involved in the message
                    for (int particle_id : x.particle_ids) {
                        // set the position
                        state.particle_data[to_string(particle_id)]["position"] = position(particle_id);
                        //state.particle_data[to_string(x.particle_id)]["position"] = state.next_collision.positions[x.particle_id];  // cannot do this (RI messages will break this)
                        if (DEBUG_SV) cout << "subV external transition: received velocity: " << VectorUtils::get_string<float>(x.data)
                                        << ", set position: " << VectorUtils::get_string<float>(state.particle_data[to_string(particle_id)]["position"]) << endl;

                        // update related time value for particle (last time position changed)
                        // must be updated after position is calculated
                        state.particle_times[particle_id] = state.current_time;

                        if (DEBUG_SV && false) {
                            // report position to the command line
                            cout << "subV external transition: subV_id: " << state.subV_id
                                << ", time: " << state.current_time
                                << ", p_id: " << particle_id
                                << ", position: " << VectorUtils::get_string<float>(state.particle_data[to_string(particle_id)]["position"]) <<endl;
                        }

                        // incorporate newly received velocity
                        state.particle_data[to_string(particle_id)]["velocity"] = x.data;

                        // prepare logging messages
                        state.logging_messages.push_back(
                            logging_message_t(state.subV_id, particle_id, state.particle_data[to_string(particle_id)]["velocity"], state.particle_data[to_string(particle_id)]["position"])
                        );

                        if (DEBUG_SV) cout << "subV external transition: new velocity set: (p_id: " << particle_id << ") " << VectorUtils::get_string<float>(x.data) << endl;
                    }

                    // set next_internal to zero to immediately calculate the next collision
                    state.next_internal = 0;
                }
            }

            // if there were no applicable messages, adjust the next internal to remain set for next collision
            if (!applicable_message_processed) {
                state.next_internal -= e;
            }

            if (DEBUG_SV) cout << "subV external transition: next internal (subV_id: " << state.subV_id << "): " << state.next_internal << endl;

            if (DEBUG_SV) cout << "subV external transition finishing" << endl;
        }

        // confluence transition
        void confluence_transition (TIME e, typename make_message_bags<input_ports>::type mbs) {
            if (DEBUG_SV) cout << "subV confluence transition called" << endl;
            internal_transition();
            external_transition(e, move(mbs));
            if (DEBUG_SV) cout << "subV confluence transition finishing" << endl;
        }

        // output function
        typename make_message_bags<output_ports>::type output () const {
            if (DEBUG_SV) cout << "subV output called" << endl;
            typename make_message_bags<output_ports>::type bags;
            vector<collision_message_t> bag_port_out;
            if (state.sending_collision) {
                bag_port_out.push_back(state.next_collision);
                if (DEBUG_SV) cout << "subV output: sending collision: " << state.next_collision << endl;
            }
            else {
                if (DEBUG_SV) cout << "subV output: no collision being sent" << endl;
            }
            get_messages<typename SubV_defs::collision_out>(bags) = bag_port_out;

            if (state.logging_messages.size() > 0) {
                get_messages<typename SubV_defs::logging_out>(bags) = state.logging_messages;
                if (DEBUG_SV) cout << "subV output: sending logging message(s)" << endl;
            }
            else {
                if (DEBUG_SV) cout << "subV output: no logging message(s) sent" << endl;
            }

            if (DEBUG_SV) cout << "subV output returning" << endl;
            return bags;
        }

        // time advance function
        TIME time_advance () const {
            if (DEBUG_SV) cout << "subV time advance called/returning" << endl;
            return state.next_internal;
        }

        friend ostringstream& operator<<(ostringstream& os, const typename SubV<TIME>::state_type& i) {
            if (DEBUG_SV) cout << "subV << called" << endl;
            string result = "(sv_id:" + to_string(i.subV_id) + ") particles: ";
            for (auto p_id = i.particle_data.begin(); p_id != i.particle_data.end(); ++p_id) {
                result += "[(p_id:" + p_id.key() + "): ";
                result += "pos" + VectorUtils::get_string<float>(i.particle_data[p_id.key()]["position"], true) + ", ";
                result += "vel" + VectorUtils::get_string<float>(i.particle_data[p_id.key()]["velocity"], true) + "]";
            }
            os << result;
            if (DEBUG_SV) cout << "subV << returning" << endl;
            return os;
        }

    private:

        // contains information on the collision and the time at which it will happen
        struct next_collision_t {
            collision_message_t collision;
            TIME time;
        };

        // initially populate cache with absolute times (not just time UNTIL)
        void populate_collision_cache () {
            if (DEBUG_SV) cout << "subV populate_collision_cache called" << endl;
            TIME next_collision_time;
            for (auto it1 = state.particle_data.begin(); it1 != state.particle_data.end(); ++it1) {
                for (auto it2 = it1; it2 != state.particle_data.end(); ++it2) {
                    if (*it1 != *it2) {
                        next_collision_time = detect(stoi(it1.key()), stoi(it2.key()));
                        if (next_collision_time < DELTA_T_MAX) {  // effectively checks that the time is not inf
                            state.collisions_cache[make_pair(stoi(it1.key()), stoi(it2.key()))] = next_collision_time;  // do not need to add since this only happens in constructor
                        }
                    }
                }
            }
            if (DEBUG_SV) cout << "subV populate_collision_cache finishing" << endl;
        }

        // update all particles to determine when the next collision will occur and with which particles
        void update_collision_cache (vector<int> p_ids) {
            if (DEBUG_SV) cout << "subV update_collision_cache called" << endl;
            TIME next_collision_time;
            pair<int, int> curr_ids;

            for (auto& it1 : p_ids) {
                for (auto& it2 : state.particle_data.items()) {
                    if (it1 == stoi(it2.key())) continue;
                    next_collision_time = detect(it1, stoi(it2.key()));
                    curr_ids = make_pair(it1, stoi(it2.key()));
                    if (next_collision_time >= 0 && next_collision_time < DELTA_T_MAX) {
                        if (DEBUG_SV) cout << "subV update_collision_cache: adding pair: " << pair_string(curr_ids) << " with time " << state.current_time << " + " << next_collision_time << endl;
                        state.collisions_cache[curr_ids] = state.current_time + next_collision_time;
                    }
                    else {
                        int num_removed = state.collisions_cache.erase(curr_ids);  // curr_ids may not may not exist
                        if (DEBUG_SV) cout << "subV update_collision_cache: removed inf pair " << pair_string(curr_ids) << ": " << (num_removed == 1 ? "true" : "false") << endl;
                    }
                }
            }
            if (CACHE_LOGGING) cout << "subV update_collision_cache:  (subV_id: "
                                    << state.subV_id
                                    << ") number of elements in collision cache (t: "
                                    << state.current_time
                                    << "): "
                                    << state.collisions_cache.size()
                                    << endl;
            if (DEBUG_SV) cout << "subV update_collision_cache finishing" << endl;
        }

        next_collision_t get_next_collision () {
            if (DEBUG_SV) cout << "subV get_next_collision called" << endl;
            next_collision_t next_collision;
            next_collision.time = numeric_limits<TIME>::infinity();
            int p1_id;
            int p2_id;

            for (auto& it : state.collisions_cache) {
                if (it.second >= 0 && it.second - state.current_time < next_collision.time) {
                    next_collision.collision.positions.clear();
                    next_collision.collision.positions[it.first.first] = {};
                    next_collision.collision.positions[it.first.second] = {};
                    if (DEBUG_SV) cout << "subV get_next_collision: next_collision_between: " << it.first.first << ", " << it.first.second << endl;
                    if (DEBUG_SV) cout << "subV get_next_collision: setting next_collision.time to: " << it.second << " - " << state.current_time << endl;
                    next_collision.time = it.second - state.current_time;
                    p1_id = it.first.first;
                    p2_id = it.first.second;
                }
            }
            int num_removed = state.collisions_cache.erase(make_pair(p1_id, p2_id));
            if (DEBUG_SV) cout << "subV get_next_collision: removed pair (" << min(p1_id, p2_id) << ", " << max(p1_id, p2_id) << "): " << (num_removed == 1 ? "true" : "false") << endl;
            if (DEBUG_SV) cout << "subV get_next_collision returning" << endl;
            return next_collision;
        }

        // returns the time until a collision between p1_id and p2_id
        TIME detect (int p1_id, int p2_id) {
            float delta_blocking = (float)state.particle_data[to_string(p1_id)]["radius"] + (float)state.particle_data[to_string(p2_id)]["radius"];
            if (delta_blocking == 0) return -1;  // check that both particles are not points

            vector<float> p1_u = position(p1_id);
            vector<float> p2_u = position(p2_id);
            vector<float> p1_v = state.particle_data[to_string(p1_id)]["velocity"];
            vector<float> p2_v = state.particle_data[to_string(p2_id)]["velocity"];

            vector<float> p2_v_sub_p1_v = VectorUtils::element_op(p2_v, p1_v, VectorUtils::subtract);
            vector<float> p2_u_sub_p1_u = VectorUtils::element_op(p2_u, p1_u, VectorUtils::subtract);

            // assuming vector multiplication per element
            float a = VectorUtils::sum(VectorUtils::element_op(p2_v_sub_p1_v, p2_v_sub_p1_v, VectorUtils::multiply));
            float b = 2 * VectorUtils::sum(VectorUtils::element_op(p2_u_sub_p1_u, p2_v_sub_p1_v, VectorUtils::multiply));
            float c = VectorUtils::sum(VectorUtils::element_op(p2_u_sub_p1_u, p2_u_sub_p1_u, VectorUtils::multiply)) - (delta_blocking * delta_blocking);

            // assuming vector multiplication is the dot product
            //float a = VectorUtils::sum(VectorUtils::dot_prod(p2_v_sub_p1_v, p2_v_sub_p1_v));
            //float b = 2 * VectorUtils::sum(VectorUtils::dot_prod(p2_u_sub_p1_u, p2_v_sub_p1_v));
            //float c = VectorUtils::sum(VectorUtils::dot_prod(p2_u_sub_p1_u, p2_u_sub_p1_u)) - (delta_blocking * delta_blocking);

            if (DEBUG_SV) {
                cout << "detection information:" << endl;
                cout << "| IDs: p1: " << p1_id << ", p2: " << p2_id << endl;
                cout << "| p1_u: " << VectorUtils::get_string<float>(p1_u) << ", p2_u: " << VectorUtils::get_string<float>(p2_u) << endl;
                cout << "| p1_v: " << VectorUtils::get_string<float>(p1_v) << ", p2_v: " << VectorUtils::get_string<float>(p2_v) << endl;
                cout << "| a: " << a << ", b: " << b << ", c: " << c << endl;
            }

            float d = (b * b) - (4 * a * c);

            // postive b indicates the particles are not heading toward each other while a negative d means there are no real solutions indicating the particles never cross paths
            if (b >= 0 || d < 0) return -1;

            float numer = max((-b) - sqrt(d), float(0));
            float denom = 2 * a;

            if (numer >= denom * DELTA_T_MAX) return -1;  // DELTA_T_MAX used to avoid divide-by-zero errors (needs to be larger than any reasonable simulation runtime)

            if (DEBUG_SV) cout << "| next collision between particles: " << numer / denom << endl;

            return numer / denom;  // time until next collision between p1_id and p2_id
        }

        // retrieve the position of a particle at a certain amount of time in the future
        // time is the time at which we want to know the particle's position
        vector<float> position (int p_id, TIME time) {
            TIME desired_time = time - state.particle_times[p_id];
            vector <float> temp = VectorUtils::element_dist(state.particle_data[to_string(p_id)]["velocity"], desired_time, VectorUtils::multiply);
            return VectorUtils::element_op(state.particle_data[to_string(p_id)]["position"], temp, VectorUtils::add);
        }

        // retrieve the position of a particle at the current time
        vector<float> position (int p_id) {
            return position(p_id, state.current_time);
        }

        pair<int, int> make_pair (int p1_id, int p2_id) {
            return pair<int, int>(min(p1_id, p2_id), max(p1_id, p2_id));
        }

        string pair_string (pair<int, int>& p) {
            return "(" + to_string(p.first) + ", " + to_string(p.second) + ")";
        }

        template<typename T, typename S>
        vector<T> get_keys (map<T, S>& test) {
            vector<T> results;
            for (const auto& it : test) {
                results.push_back(it.first);
            }
            return results;
        }
};

#endif