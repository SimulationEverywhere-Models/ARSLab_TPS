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

using namespace cadmium;
using namespace std;

// Port definition
struct SubV_defs {
    struct response_in : public in_port<tracker_message_t> {};
    struct collision_out : public out_port<___> {};  // collision message should contain: 2 particle IDs and the time (?)
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

        struct state_type {
            // state information
            json particle_data;  // particle_id, {postition, velocity, radius}  // needs information on last collision
            int subV_id;
            TIME next_internal;  // next collision
            TIME current_time;  // current time within a subV module
            map<int, float> particle_times;  // particle_id, time  // last event for each particle in a subV module
            collision_type next_collision;
        };
        state_type state;

        SubV () {
            // initialization
            // TODO: subV_id should be initialized from arguments
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
            // update the current time before doing work
            state.current_time += state.next_internal;  // next_internal was set by the previous call to int/ext transition

            // get the next collision
            state.next_collision = get_next_collision();

            // set next_internal
            state.next_internal = state.next_collision.time;
        }

        // external transition
        void external_transition (TIME e, typename make_message_bags<input_ports>::type mbs) {
            // update the current time before doing work
            state.current_time += e;

            // get the next collision
            state.next_collision = get_next_collision();

            // set next_internal
            state.next_internal = state.next_collision.time;

        }

        // confluence transition
        void confluence_transition (TIME e, typename make_message_bags<input_ports>::type mbs) {
            //
        }

        // output function
        typename make_message_bags<output_ports>::type output () const {
            typename make_message_bags<output_ports>::type bags;
            //
            return bags;
        }

        // time advance function
        TIME time_advance () const {
            return state.current_time + state.next_internal;
        }

    private:

        struct collision_type {
            TIME time;
            int p1_id;
            int p2_id;
        };

        const int delta_t_max = 10000000;  // used to avoid divide-by-zero errors (needs to be larger than any reasonable simulation runtime)

        // check all particles to determine when the next collision will occur and with which particles
        collision_type get_next_collision () {
            collision_type next_collision;
            next_collision.time = numeric_limits<TIME>::infinity();
            TIME next_collision_time;
            for (auto it1 = particle_data.begin(); it1 != particle_data.end(); ++it1) {
                for (auto it2 = it1; it2 != particle_data.end(); ++it2) {
                    if (it1 != it2) {
                        next_collision_time = detect(stoi(it1.key()), stoi(it2.key()));
                        if (next_collision_time < next_collision.time) {
                            next_collision.time = next_collision_time;
                            next_collision.p1_id = stoi(it1.key());
                            next_collision.p2_id = stoi(it2.key());
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
            float p2_v = state.particle_data[to_string(p1_id)]["velocity"];

            float p2_v_sub_p1_v = vect_op(p2_v, p1_v, subtract);
            float p2_u_sub_p1_u = vect_op(p2_u, p1_u, subtract);

            // assuming vector multiplication is the dot product
            float a = sum_vect(dot_prod(p2_v_sub_p1_v, p2_v_sub_p1_v));
            float b = 2 * sum_vect(dot_prod(p2_u_sub_p1_u, p2_v_sub_p1_v));
            float c = sum_vect(dot_prod(p2_u_sub_p1_u, p2_u_sub_p1_u)) - (delta_blocking * delta_blocking);
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
            velocity = state.particle_data[to_string(p_id)]["velocity"];
            position = state.particle_data[to_string(p_id)]["position"];
            vector <float> temp;
            for (auto i : velocity) {
                temp.push_back(i * time);
            }
            return vect_op(position, temp, add);
        }

        /*** HELPER FUCNTIONS ***/

        // sums all of the elements in a vector
        float sum_vect (vector<float> v) {
            float result = 0;
            for (auto i : v) {
                result += i;
            }
            return result;
        }

        // calculates the dot product of two vectors
        float dot_prod (vector<float> v1, vector<float> v2) {
            return sum_vect(vect_op(v1, v2, multiply));
        }

        // preform an operation on each of the elements in a vector
        vector<float> vect_op (vector<float> v1, vector<float> v2, float (*operation)(float, float)) {
            if (v1.size() != v2.size()) return {};
            vector<float> result;
            for (int i = 0; i < v1.size(); ++i) {
                result.push_back(operation(v1[i], v2[i]));
            }
            return result;
        }

        // basic operations
        float add      (float i, float j) { return i + j };
        float subtract (float i, float j) { return i - j; }
        float multiply (float i, float j) { return i * j; }
}

#endif