#ifndef RANDOMIMPULSE_HPP
#define RANDOMIMPULSE_HPP

/*
TODO:
- Stop RI from sending invalid messages (instead of having the responder catch and ignore them).
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
#include <utility>  // contains pair
#include <queue>  // contains priority queue

#include "../test/tags.hpp"  // debug tags
#include "../utilities/vector_utils.hpp"  // vector functions

#include "../data_structures/message.hpp"
//#include "../data_structures/species.hpp"  // TODO: Get this data from a JSON

using namespace cadmium;
using namespace std;

using json = nlohmann::json;

// Port definition
struct RandomImpulse_defs {
    struct impulse_out : public out_port<message_t> {};
};

template<typename TIME> class RandomImpulse {
    public:
        // temporary assignments
        // TODO: get from particle information (species)
        //float tau = 2;  // used in exp dist for next time
        //float shape = 2;
        //float mean = 2;

        // ports definition
        using input_ports = tuple<>;
        using output_ports = tuple<typename RandomImpulse_defs::impulse_out>;

        // comparator for int, TIME pair
        struct ComparePair {
            bool operator() (pair<int, TIME> const& p1, pair<int, TIME> const& p2) {
                return p2.second < p1.second;
            }
        };

        struct state_type {
            json particle_data;
            int dim;  // specifies the number of dimensions
            bool do_ri;  // whether or not to generate random impulses
            TIME next_internal;
            message_t impulse;
            priority_queue<pair<int, TIME>, vector<pair<int, TIME>>, ComparePair> particle_times;
            TIME current_time;
        };
        state_type state;

        RandomImpulse () {
            if (DEBUG_RI) cout << "RandomImpulse default constructor called" << endl;
            // initialize test particles
        }

        RandomImpulse (int test) {
            if (DEBUG_RI) cout << "RandomImpulse non-default constructor called with value: " << test << endl;
        }

        RandomImpulse (json j, int dim, bool do_ri) {
            if (DEBUG_RI) cout << "RandomImpulse constructor received JSON and dim: " << j << " --- " << dim << endl;
            state.particle_data = j;
            state.dim = dim;
            state.do_ri = do_ri;
            state.impulse.purpose = "ri";
            state.next_internal = TIME();
            state.current_time = TIME();

            // go through particles and get the times at which they should receive RIs
            for (auto it = state.particle_data.begin(); it != state.particle_data.end(); ++it) {
                state.particle_times.push(pair<int, TIME>(stoi(it.key()), generate_next_time(state.particle_data[it.key()]["tau"])));
            }
        }

        // internal transition
        void internal_transition () {
            if (DEBUG_RI) cout << "ri internal transition called" << endl;
            state.current_time += state.next_internal;
            if (DEBUG_RI) cout << "ri internal transition: current_time set: " << state.current_time << endl;

            if (!state.do_ri) {
                state.next_internal = numeric_limits<TIME>::infinity();
                if (DEBUG_RI) cout << "ri internal transition finishing (permanately passivated)" << endl;
                return;
            }

            state.particle_times.pop();  // remove current particle from queue

            int currId = state.particle_times.top().first;  // note the current particle's ID
            state.next_internal = state.particle_times.top().second - state.current_time;  // note the current particle's impulse time
            if (DEBUG_RI) cout << "ri internal transition: next_internal set: " << state.particle_times.top().second << " - " << state.current_time << " = " << state.next_internal << endl;

            json currParticle = state.particle_data[to_string(state.particle_times.top().first)];

            state.particle_times.push(pair<int, TIME>(currId, generate_next_time(state.particle_data[to_string(currId)]["tau"]) + state.current_time));  // add current particle back with new time

            vector<float> next_impulse;
            float momentum_magnitude;
            float factor;
            vector<float> direction;
            float z_component;
            float xy_factor;

            gamma_distribution<float> gamma_dist(currParticle["mean"], currParticle["shape"]);  // mean, shape
            momentum_magnitude = gamma_dist(generator);

            // TODO: attempt to correct the below inefficiencies (possibly move them to the state)
            // These are created/destroyed every time the internal transition is called
            uniform_real_distribution<float> uniform_dist_neg1_1(-1, 1);
            uniform_real_distribution<float> uniform_dist_0_1(0, 1);
            uniform_real_distribution<float> uniform_dist_0_2(0, 2);
            uniform_real_distribution<float> uniform_dist_0_2pi(0, 2 * pi);
            switch (state.dim) {
                case 1:
                    factor = 1;
                    direction.push_back(2 * (int)uniform_dist_0_2(generator) - 1);
                    break;
                case 2:
                    factor = sqrt(pow(1 - uniform_dist_0_1(generator), 2));
                    direction.push_back(cos(uniform_dist_0_2pi(generator)));
                    direction.push_back(sin(uniform_dist_0_2pi(generator)));
                    break;
                case 3:
                    factor = sqrt(uniform_dist_0_1(generator));
                    z_component = uniform_dist_neg1_1(generator);
                    xy_factor = sqrt(1 - pow(z_component, 2));
                    direction.push_back(xy_factor * cos(uniform_dist_0_2pi(generator)));
                    direction.push_back(xy_factor * sin(uniform_dist_0_2pi(generator)));
                    direction.push_back(z_component);
                    break;
                default:
                    assert(false && "random_impulse: unsupported number of dimensions");
                    break;
            }

            for (auto i : direction) {
                next_impulse.push_back(i * momentum_magnitude * factor);
            }

            // finish the impulse message
            state.impulse.data = next_impulse;
            state.impulse.particle_ids = {currId};
            if (DEBUG_RI) cout << "ri internal transition finishing" << endl;
        }

        // external transition
        // receives messages regarding changes to particle mass
        void external_transition (TIME e, typename make_message_bags<input_ports>::type mbs) {
            assert(false && "RI module must not receive inputs");
        }

        // confluence transition
        // should never happen
        void confluence_transition (TIME e, typename make_message_bags<input_ports>::type mbs) {
            if (DEBUG_RI) cout << "ri confluence transition called" << endl;
            internal_transition();
            if (DEBUG_RI) cout << "ri confluence transition finishing" << endl;
        }

        // output function
        typename make_message_bags<output_ports>::type output () const {
            if (DEBUG_RI) cout << "ri output called" << endl;
            typename make_message_bags<output_ports>::type bags;
            vector<message_t> bag_port_out;
            if (state.impulse.particle_ids.size() != 0) {
                bag_port_out.push_back(state.impulse);
                if (DEBUG_RI) cout << "ri output sending impulse (p_id: " << VectorUtils::get_string<int>(state.impulse.particle_ids) << "): " << VectorUtils::get_string<float>(state.impulse.data) << endl;
            }
            else {
                if (DEBUG_RI) cout << "ri output not sending impulse" << endl;
            }
            get_messages<typename RandomImpulse_defs::impulse_out>(bags) = bag_port_out;
            if (DEBUG_RI) cout << "ri output returning" << endl;
            return bags;
        }

        // time advance function
        TIME time_advance () const {
            if (DEBUG_RI) cout << "ri time advance called/returning" << endl;
            return state.next_internal < 0 ? 0 : state.next_internal;
        }

        friend ostringstream& operator<<(ostringstream& os, const typename RandomImpulse<TIME>::state_type& i) {
            if (DEBUG_RI) cout << "ri << called" << endl;
            os << "impulse [(p_id:" << VectorUtils::get_string<int>(i.impulse.particle_ids) << "): imp" << VectorUtils::get_string<float>(i.impulse.data, true) << "]";
            if (DEBUG_RI) cout << "ri << returning" << endl;
            return os;
        }

    private:
        default_random_engine generator;  // used to generate numbers from gamma distribution
        const float pi = 3.14159265359;

        float generate_next_time (float tau) {
            exponential_distribution<float> exponential_dist(tau);
            return exponential_dist(generator);
        }
};

#endif