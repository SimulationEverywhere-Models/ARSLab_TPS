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
#include <algorithm>  // max
#include <map>

#include "../test/tags.hpp"  // debug tags
#include "../utilities/vector_utils.hpp"  // vector functions

#include "../data_structures/message.hpp"

using namespace cadmium;
using namespace std;

// Port definition
struct SubV_defs {
    struct response_in : public in_port<tracker_message_t> {};
    struct collision_out : public out_port<collision_message_t> {};  // collision message should contain: 2 particle IDs and the time
    //struct departure_out : public out_port<___> {};
    //struct arrival_out : public out_port<___> {};
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
        using output_ports = tuple<typename SubV_defs::collision_out>;

        struct state_type {
            // state information
            json particle_data;  // particle_id, {postition, velocity, radius}  // needs information on last collision
            int subV_id;
            TIME next_internal;  // next collision
            TIME current_time;  // current time within a subV module
            map<int, float> particle_times;  // particle_id, time  // last event for each particle in a subV module
            collision_message_t next_collision;
        };
        state_type state;

        SubV () {
            if (DEBUG_SV) cout << "SubV default constructor called" << endl;

            // initialization
            // TODO: subV_id should be initialized or calculated from arguments
            state.subV_id = 1;
            state.current_time = TIME();
            state.next_internal = TIME();

            // initialize particle times
            for (auto it = particle_data.begin(); it += particle_data.end(); ++it) {
                particle_times[stoi(it.key())] = state.current_time;
            }
        }

        // internal transition
        void internal_transition () {
            if (DEBUG_SV) cout << "subV internal transition called" << endl;

            // update the current time before doing work
            state.current_time += state.next_internal;  // next_internal was set by the previous call to int/ext transition

            // get the next collision
            next_collision_t next_collision = get_next_collision();

            // calculate positions but don't incorporate them until new velocities received
            // can't set until next_int (rather, when we get the corresponding velocity message back) in case RI sends a message (in which case, we throw away this calculation)
            //state.next_collision.p1_pos = position(state.next_collision.p1_id);
            //state.next_collision.p2_pos = position(state.next_collision.p2_id);
            for (auto it = state.next_collision.begin(); it != state.next_collision.end(); ++it) {
                state.next_collision.positions[it->first] = position(it->first);
            }
            assert(state.next_collision.positions.size() == 2);

            // set next_internal
            state.next_internal = state.next_collision.time;

            if (DEBUG_SV) cout << "subV internal transition finishing" << endl;
        }

        // external transition
        void external_transition (TIME e, typename make_message_bags<input_ports>::type mbs) {
            if (DEBUG_SV) cout << "subV external transition called" << endl;
            
            // update the current time before doing work
            state.current_time += e;

            if (get_messages<typename SubV_defs::response_in>(mbs).size() > 1) {
                cout << "NOTE: subV received more than one concurrent message" << endl;
            }

            // Handle velocity messages from tracker
            for (const auto &x : get_messages<typename SubV_defs::response_in>(mbs)) {
                // if the message is relevant to this subV
                if (find(x.subV_ids.begin(), x.subV_ids.end(), state.subV_id) != x.subV_ids.end()) {
                    // Maybe have a flag that determines if the message is an RI message (so no positions (or velocities?) are set until a non-RI message is received)
                    // - No, RI messages should be treated the same

                    // Calculations should not be done here because, for collisions, there will be two associated messages received (one for each particle involved)
                    // - Only particle_data information should be changed

                    // update related time value for particle (last time position changed)
                    state.particle_times[x.particle_id] = state.current_time;

                    // incorporate positions calculated in internal transition or before RI data is used (position must be called before velocity change)
                    // if position was calculated in the internal transition (ie. a particle involved in a collision)
                    if (state.next_collision.positions.find(x.particle_id) != state.next_collision.positions.end()) {
                        state.particle_data[to_string(x.particle_id)]["position"] = state.next_collision.positions[x.particle_id];
                    }
                    // if an impulse is received from the RI module (the stored data will need to be recalculated, so it cannot be used)
                    else {
                        state.particle_data[to_string(x.particle_id)]["position"] = position(x.particle_id);
                    }

                    // report position to the command line
                    cout << "subV: " << state.subV_id << ", time: " << state.current_time << ", p_id: " << x.particle_id << ", position:" << endl;

                    // incorporate newly received velocity
                    state.particle_data[to_string(x.particle_id)]["velocity"] = x.data;

                    // set next_internal to zero to immediately calculate the next collision
                    state.next_internal = 0;
                }
            }

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
            bag_port_out = state.next_collision;
            get_messages<typename SubV_defs::collision_out>(bags) = bag_port_out;
            if (DEBUG_SV) cout << "subV output returning" << endl;
            return bags;
        }

        // time advance function
        TIME time_advance () const {
            if (DEBUG_SV) cout << "subV time advance called/returning" << endl;
            return state.current_time + state.next_internal;
        }

        friend ostringstream& operator<<(ostringstream& os, const typename SubV<TIME>::state_type& i) {
            if (DEBUG_SV) cout << "subV << called" << endl;
            string result = "";
            for (auto p_id = i.particle_data.begin(); p_id != i.particle_data.end(); ++p_id) {
                result += "[(p_id:" + p_id.key();
                for (auto position_comp : i.particle_data[p_id.key()]["position"]) {
                    result += to_string(position_comp) + " ";
                }
                result += "] ";
            }
            os << "positions: " << result;
            if (DEBUG_SV) cout << "subV << returning" << endl;
            return os;
        }

    private:

        const int delta_t_max = 10000000;  // used to avoid divide-by-zero errors (needs to be larger than any reasonable simulation runtime)

        // contains information on the collision and the time at which it will happen
        struct next_collision_t {
            collision_message_t collision;
            TIME time;
        };

        // check all particles to determine when the next collision will occur and with which particles
        next_collision_t get_next_collision () {
            collision_message_t next_collision;
            next_collision.time = numeric_limits<TIME>::infinity();
            TIME next_collision_time;
            for (auto it1 = particle_data.begin(); it1 != particle_data.end(); ++it1) {
                for (auto it2 = it1; it2 != particle_data.end(); ++it2) {
                    if (it1 != it2) {
                        next_collision_time = detect(stoi(it1.key()), stoi(it2.key()));
                        if (next_collision_time >= 0 && next_collision_time < next_collision.time) {
                            //next_collision.p1_id = stoi(it1.key());
                            //next_collision.p2_id = stoi(it2.key());
                            next_collision.collision.positions[stoi(it1.key())] = {};
                            next_collision.collision.positions[stoi(it2.key())] = {};
                            next_collision.time = next_collision_time;
                        }
                    }
                }
            }
            return next_collision;
        }

        // returns the time until a collision between p1_id and p2_id
        TIME detect (int p1_id, int p2_id) {
            delta_blocking = abs(state.particle_data[to_string(p1_id)]["radius"] - state.particle_data[to_string(p2_id)]["radius"]);
            if (delta_blocking == 0) return -1;

            float p1_u = position(p1_id);
            float p2_u = position(p2_id);
            float p1_v = state.particle_data[to_string(p1_id)]["velocity"];
            float p2_v = state.particle_data[to_string(p2_id)]["velocity"];

            float p2_v_sub_p1_v = VectorUtils::element_op(p2_v, p1_v, subtract);
            float p2_u_sub_p1_u = VectorUtils::element_op(p2_u, p1_u, subtract);

            // assuming vector multiplication is the dot product
            float a = VectorUtils::sum(VectorUtils::dot_prod(p2_v_sub_p1_v, p2_v_sub_p1_v));
            float b = 2 * VectorUtils::sum(VectorUtils::dot_prod(p2_u_sub_p1_u, p2_v_sub_p1_v));
            float c = VectorUtils::sum(VectorUtils::dot_prod(p2_u_sub_p1_u, p2_u_sub_p1_u)) - (delta_blocking * delta_blocking);
            float d = (b * b) - (4 * a * c);

            if (b >= 0 || d < 0) return -1;

            float numer = max((-b) - sqrt(d), 0);
            float denom = 2 * a;

            if (numer >= denom * delta_t_max) return -1;

            return numer / denom;  // time until next collision between p1_id and p2_id
        }

        // retrieve the position of a particle at the current time
        vector<float> position (int p_id) {]
            time = state.current_time - state.particle_times[p_id];
            vector <float> temp;
            for (auto i : state.particle_data[to_string(p_id)]["velocity"]) {
                temp.push_back(i * time);
            }
            return vect_op(state.particle_data[to_string(p_id)]["position"], temp, add);
        }
};

#endif