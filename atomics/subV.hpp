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
#include <nlohmann/json.hpp>

#include "../test/tags.hpp"  // debug tags
#include "../utilities/vector_utils.hpp"  // vector functions

#include "../data_structures/message.hpp"

using namespace cadmium;
using namespace std;

using json = nlohmann::json;

// Port definition
struct SubV_defs {
    struct response_in : public in_port<tracker_message_t> {};
    struct collision_out : public out_port<collision_message_t> {};
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
            TIME next_internal;
            TIME current_time;  // current time within a subV module
            map<int, float> particle_times;  // particle_id, time  // last event for each particle in a subV module
            collision_message_t next_collision;
            bool awaiting_response;  // whether or not subV has received a response from the responder (if not, do not preform further calculations until received)
            bool sending_collision;  // whether or not to send a collision (stop message sending is receiving an RI or a response message)
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
        }

        // internal transition
        void internal_transition () {
            if (DEBUG_SV) cout << "subV internal transition called" << endl;

            // update the current time before doing work
            state.current_time += state.next_internal;  // next_internal was set by the previous call to int/ext transition

            // reset flag (can be done here since output is called before internal transition in Cadmium)
            state.sending_collision = true;

            if (state.awaiting_response) {
                state.next_internal = numeric_limits<TIME>::infinity();
                if (DEBUG_SV) cout << "subV internal transition: passivating (have not received response message)" << endl;
                return;
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

            if (get_messages<typename SubV_defs::response_in>(mbs).size() > 1) {
                if (DEBUG_SV) cout << "NOTE: subV received more than one concurrent message" << endl;
            }

            // Handle velocity messages from tracker
            for (const auto &x : get_messages<typename SubV_defs::response_in>(mbs)) {
                // if the message is relevant to this subV
                if (find(x.subV_ids.begin(), x.subV_ids.end(), state.subV_id) != x.subV_ids.end()) {
                    applicable_message_processed = true;

                    // Calculations should not be done here because, for collisions, there will be two associated messages received (one for each particle involved)
                    // - Only particle_data information should be changed

                    // update related time value for particle (last time position changed)
                    state.particle_times[x.particle_id] = state.current_time;

                    if (DEBUG_SV) cout << "subV external transition: handling impulse: " << (x.is_ri ? "true" : "false") << endl;
                    if (DEBUG_SV) cout << "subV external transition: current time (subV_id: " << state.subV_id << "): " << state.current_time << endl;

                    //state.particle_data[to_string(x.particle_id)]["position"] = position(x.particle_id);
                    state.particle_data[to_string(x.particle_id)]["position"] = state.next_collision.positions[x.particle_id];
                    if (DEBUG_SV) cout << "subV external transition: received velocity: " << VectorUtils::get_string<float>(x.data)
                                       << ", set position: " << VectorUtils::get_string<float>(state.particle_data[to_string(x.particle_id)]["position"]) << endl;

                    if (DEBUG_SV && false) {
                        // report position to the command line
                        cout << "subV external transition: subV_id: " << state.subV_id
                             << ", time: " << state.current_time
                             << ", p_id: " << x.particle_id
                             << ", position: " << VectorUtils::get_string<float>(state.particle_data[to_string(x.particle_id)]["position"]) <<endl;
                    }

                    // incorporate newly received velocity
                    state.particle_data[to_string(x.particle_id)]["velocity"] = x.data;
                    if (DEBUG_SV) cout << "subV external transition: new velocity set: (p_id: " << x.particle_id << ") " << VectorUtils::get_string<float>(x.data) << endl;

                    // set next_internal to zero to immediately calculate the next collision
                    state.next_internal = 0;

                    // invalidate the message
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
            string result = "particles: ";
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

        const int delta_t_max = 10000000;  // used to avoid divide-by-zero errors (needs to be larger than any reasonable simulation runtime)

        // contains information on the collision and the time at which it will happen
        struct next_collision_t {
            collision_message_t collision;
            TIME time;
        };

        // check all particles to determine when the next collision will occur and with which particles
        next_collision_t get_next_collision () {
            next_collision_t next_collision;
            next_collision.time = numeric_limits<TIME>::infinity();
            TIME next_collision_time;
            for (auto it1 = state.particle_data.begin(); it1 != state.particle_data.end(); ++it1) {
                for (auto it2 = it1; it2 != state.particle_data.end(); ++it2) {
                    if (it1 != it2) {
                        next_collision_time = detect(stoi(it1.key()), stoi(it2.key()));
                        if (next_collision_time >= 0 && next_collision_time < next_collision.time) {
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
            float delta_blocking = (float)state.particle_data[to_string(p1_id)]["radius"] + (float)state.particle_data[to_string(p2_id)]["radius"];
            if (delta_blocking == 0) return -1;

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

            if (b >= 0 || d < 0) return -1;

            float numer = max((-b) - sqrt(d), float(0));
            float denom = 2 * a;

            if (numer >= denom * delta_t_max) return -1;

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
};

#endif