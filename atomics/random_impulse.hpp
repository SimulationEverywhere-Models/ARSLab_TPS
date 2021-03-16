#ifndef RANDOMIMPULSE_HPP
#define RANDOMIMPULSE_HPP

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

#include "../data_structures/message.hpp"
//#include "../data_structures/species.hpp"  // TODO: Get this data from a JSON

#define DEBUG false

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

        struct ComparePair {
            bool operator() (pair<int, TIME> const& p1, pair<int, TIME> const& p2) {
                return p2.second < p1.second;
            }
        };

        struct state_type {
            json particle_data;d
            int dim;  // specifies the number of dimensions
            TIME next_impulse_time;
            message_t impulse;
            priority_queue<pair<int, TIME>, vector<pair<int, TIME>>, ComparePair> particle_times;
        };
        state_type state;

        RandomImpulse () {
            if (DEBUG) cout << "RandomImpulse default constructor called" << endl;
            // initialize test particles
        }

        RandomImpulse (int test) {
            if (DEBUG) cout << "RandomImpulse non-default constructor called with value: " << test << endl;
        }

        RandomImpulse (json j, int dim) {
            if (DEBUG) cout << "RandomImpulse constructor received JSON and dim: " << j << " --- " << dim << endl;
            state.particle_data = j;
            state.dim = dim;

            // go through particles and get the times at which they should receive RIs
            for (auto it = state.particle_data.begin(); it != state.particle_data.end(); ++it) {
                state.particle_times.push(pair<int, TIME>(stoi(it.key()), generate_next_time(state.particle_data[it.key()]["tau"])));
            }
        }

        // internal transition
        void internal_transition () {
            if (DEBUG) cout << "ri internal transition called" << endl;
            int currId = state.particle_times.top().first;  // note the current particle's ID
            state.next_impulse_time = state.particle_times.top().second;  // note the current particle's impulse time
            state.particle_times.pop();  // remove current particle from queue

            // modify all other particle times
            priority_queue<pair<int, TIME>, vector<pair<int, TIME>>, ComparePair> particle_times_temp;
            while (!state.particle_times.empty()) {
                particle_times_temp.push(pair<int, TIME>(
                    state.particle_times.top().first,
                    state.particle_times.top().second - state.next_impulse_time
                ));
                state.particle_times.pop();
            }
            particle_times_temp.push(pair<int, TIME>(currId, generate_next_time(state.particle_data[to_string(currId)]["tau"])));  // add current
            state.particle_times = move(particle_times_temp);

            json currParticle = state.particle_data[to_string(state.particle_times.top().first)];

            vector<float> next_impulse;
            float momentum_magnitude;
            float factor;
            vector<float> direction;
            float z_component;
            float xy_factor;

            gamma_distribution<float> gamma_dist(currParticle["mean"], currParticle["shape"]);  // mean, shape
            momentum_magnitude = gamma_dist(generator);

            // TODO: attempt to correct the below inefficiencies
            // These are created/destroyed every time the internal transition is called
            // Extremely inefficient
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
                    assert(false && "Unsupported number of dimensions");
                    break;
            }

            for (auto i : direction) {
                next_impulse.push_back(i * momentum_magnitude * factor);
            }

            // finish the impulse message
            state.impulse.data = next_impulse;
            state.impulse.particle_id = currId;
            if (DEBUG) cout << "ri internal transition finishing" << endl;
        }

        // external transition
        // receives messages regarding changes to particle mass
        void external_transition (TIME e, typename make_message_bags<input_ports>::type mbs) {
            assert(false && "RI modules must not receive inputs");
        }

        // confluence transition
        // should never happen
        void confluence_transition (TIME e, typename make_message_bags<input_ports>::type mbs) {
            if (DEBUG) cout << "ri confluence transition called" << endl;
            internal_transition();
            if (DEBUG) cout << "ri confluence transition finishing" << endl;
        }

        // output function
        typename make_message_bags<output_ports>::type output () const {
            if (DEBUG) cout << "ri output called" << endl;
            typename make_message_bags<output_ports>::type bags;
            vector<message_t> bag_port_out;
            bag_port_out.push_back(state.impulse);
            get_messages<typename RandomImpulse_defs::impulse_out>(bags) = bag_port_out;
            if (DEBUG) cout << "ri output returning" << endl;
            return bags;
        }

        // time advance function
        TIME time_advance () const {
            if (DEBUG) cout << "ri time advance called/returning" << endl;
            return state.next_impulse_time;
        }

        friend ostringstream& operator<<(ostringstream& os, const typename RandomImpulse<TIME>::state_type& i) {
            if (DEBUG) cout << "ri << called" << endl;
            string result = "";
            for (auto impulse_comp : i.impulse.data) {
                result += to_string(impulse_comp) + " ";
            }
            os << "impulse: " << result;
            if (DEBUG) cout << "ri << returning" << endl;
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