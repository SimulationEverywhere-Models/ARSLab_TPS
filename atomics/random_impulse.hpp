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

#include "../data_structures/impulse_message.hpp"
#include "../data_structures/species.hpp"  // TODO: Get this data from a JSON

using namespace cadmium;
using namespace std;

using json = nlohmann::json;

// Port definition
struct RandomImpulse_defs {
    struct impulse_out : public out_port<impulse_message_t> {};
};

template<typename TIME> class RandomImpulse {
    public:
        // temporary assignments
        // TODO: get from particle information (species)
        float tau = 2;
        float shape = 2;
        float mean = 2;

        // ports definition
        using input_ports = tuple<>;
        using output_ports = tuple<typename RandomImpulse_defs::impulse_out>;

        struct state_type {
            //map<int, float> masses;  // particle_id, mass --- initialize via species (should be stored in JSON)
            //map<int, map<string, float>> particle_data;  // <particle_id, <property_id, data>>
            json particle_data;
            int dim;  // specifies the number of dimensions
            TIME next_impulse_time;
            impulse_message_t impulse;
        };
        state_type state;

        RandomImpulse () {
            cout << "RandomImpulse default constructor called" << endl;
            // initialize test particles
        }

        // initialize particles from external file
        // may import straight from file in defautl constructor
        //RandomImpulse (particle data) {}

        RandomImpulse (int test) {
            cout << "RandomImpulse non-default constructor called with value: " << test << endl;
        }

        RandomImpulse (json j, int dim) {  // Do we also want to pass in species information?
            cout << "RandomImpulse constructor received JSON and dim: " << j << " --- " << dim << endl;
            state.particle_data = j;
            state.dim = dim;
        }

        // internal transition
        void internal_transition () {
            cout << "ri internal transition called" << endl;
            exponential_distribution<float> exponential_dist(tau);
            state.next_impulse_time = exponential_dist(generator);
            //state.next_impulse_time = TIME("00:00:05");  //rand() % 10;
            //state.impulse = RI_message_t({1, 2, 3});
            int dim = 3;
            vector<float> next_impulse;
            float momentum_magnitude;
            float factor;
            vector<float> direction;
            float z_component;
            float xy_factor;

            gamma_distribution<float> gamma_dist(mean, shape);  // mean, shape
            momentum_magnitude = gamma_dist(generator);

            // These are created/destroyed every time the internal transition is called
            // Extremely inefficient
            uniform_real_distribution<float> uniform_dist_neg1_1(-1, 1);
            uniform_real_distribution<float> uniform_dist_0_1(0, 1);
            uniform_real_distribution<float> uniform_dist_0_2(0, 2);
            uniform_real_distribution<float> uniform_dist_0_2pi(0, 2 * pi);
            switch (dim) {
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

            state.impulse.impulse = next_impulse;
            state.impulse.particle_id = 0;  // TODO: Use this to identify the particle in question
            cout << "ri internal transition finishing" << endl;
        }

        // external transition
        // receives messages regarding changes to particle mass
        void external_transition (TIME e, typename make_message_bags<input_ports>::type mbs) {
            assert(false && "RI modules must not receive inputs");
        }

        // confluence transition
        // should never happen
        void confluence_transition (TIME e, typename make_message_bags<input_ports>::type mbs) {
            cout << "ri confluence transition called" << endl;
            internal_transition();
            cout << "ri confluence transition finishing" << endl;
        }

        // output function
        typename make_message_bags<output_ports>::type output () const {
            cout << "ri output called" << endl;
            typename make_message_bags<output_ports>::type bags;
            vector<impulse_message_t> bag_port_out;
            bag_port_out.push_back(state.impulse);
            get_messages<typename RandomImpulse_defs::impulse_out>(bags) = bag_port_out;
            cout << "ri output returning" << endl;
            return bags;
        }

        // time advance function
        TIME time_advance () const {
            cout << "ri time advance called/returning" << endl;
            return state.next_impulse_time;
        }

        friend ostringstream& operator<<(ostringstream& os, const typename RandomImpulse<TIME>::state_type& i) {
            cout << "ri << called" << endl;
            string result = "";
            for (auto impulse_comp : i.impulse.impulse) {
                result += to_string(impulse_comp) + " ";
            }
            os << "impulse: " << result;
            cout << "ri << returning" << endl;
            return os;
        }
    
    private:
        default_random_engine generator;  // used to generate numbers from gamma distribution
        const float pi = 3.14159265359;
};

#endif