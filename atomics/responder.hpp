#ifndef RESPONDER_HPP
#define RESPONDER_HPP

/*
Current functionality:
- Relay impulse messages from the impulse_in port through the impulse_out port
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

#include "../data_structures/message.hpp"

#define DEBUG false

using namespace cadmium;
using namespace std;

using json = nlohmann::json;

// Port definition
struct Responder_defs {
    //struct transition_in : public in_port<___> {};
    struct impulse_in : public in_port<message_t> {};
    //struct collision_in : public in_port<___> {};
    //struct attachment_out : public out_port<___> {};  // not applicable for now (tethering)
    //struct detachment_out : public out_port<___> {};  // not applicable for now (tethering)
    struct impulse_out : public out_port<message_t> {};  // unsure of purpose (message type may be wrong)
    //struct loading_out : public out_port<___> {};
    //struct restitution_out : public out_port<___> {};
    //struct response_out : public out_port<___> {};
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
        using input_ports = tuple<typename Responder_defs::impulse_in>;
        using output_ports = tuple<typename Responder_defs::impulse_out>;

        struct state_type {
            // particle mass, position, velocity
            //map
            json particle_data;
            TIME next_internal;
            message_t impulse;
        };
        state_type state;

        Responder () {
            //
        }

        Responder (json j) {
            if (DEBUG) cout << "Responder constructor received JSON: " << j << endl;
            state.particle_data = j;
        }

        // internal transition
        void internal_transition () {
            if (DEBUG) cout << "resp internal transition called" << endl;
            // TODO: simulate particle decay (possibly in another module)
            state.next_internal = numeric_limits<TIME>::infinity();  // the response module should not have internal events
            if (DEBUG) cout << "resp internal transition finished" << endl;
        }

        // external transition
        void external_transition (TIME e, typename make_message_bags<input_ports>::type mbs) {
            if (DEBUG) cout << "resp external transition called" << endl;
            if (get_messages<typename Responder_defs::impulse_in>(mbs).size() > 1) {
                assert(false && "Responder received more than one concurrent message");
            }
            // Handle impulse messages
            for (const auto &x : get_messages<typename Responder_defs::impulse_in>(mbs)) {
                if (DEBUG) cout << "responder received: " << x << endl;

                // x is a message object

                vector<float> newVelocity;
                for (int i = 0; i < x.data.size(); ++i) {
                    newVelocity.push_back(x.data[i] + (float)state.particle_data[to_string(x.particle_id)]["velocity"][i]);
                }
                
                //cout << "resp: particle " << x.particle_id << " vel before impulse: " << state.particle_data[to_string(x.particle_id)] << endl;
                //cout << "resp: adding impulse: " << vector_string<float>(x.impulse) << endl;
                state.particle_data[to_string(x.particle_id)]["velocity"] = newVelocity;  // record velocity change in resp particle model
                //cout << "resp: particle " << x.particle_id << " vel after impulse: " << state.particle_data[to_string(x.particle_id)] << endl;
                // TODO: send msg so that detector can do the same
            }

            //for (const auto &x : get_messages<typename Responder_defs::collision_in>(mbs)) {
                // TODO: manage incoming collision messages
            //}

            if (DEBUG) cout << "resp external transition finish" << endl;
        }

        // confluence transition
        void confluence_transition (TIME e, typename make_message_bags<input_ports>::type mbs) {
            if (DEBUG) cout << "resp confluence transition called" << endl;
            internal_transition();
            external_transition(TIME(), move(mbs));
            if (DEBUG) cout << "resp confluence transition finishing" << endl;
        }

        // output function
        typename make_message_bags<output_ports>::type output () const {
            if (DEBUG) cout << "resp output called" << endl;
            typename make_message_bags<output_ports>::type bags;
            vector<message_t> bag_port_out;
            bag_port_out.push_back(state.impulse);
            get_messages<typename Responder_defs::impulse_out>(bags) = bag_port_out;
            if (DEBUG) cout << "resp output returning" << endl;
            return bags;
        }

        // time advance function
        TIME time_advance () const {
            if (DEBUG) cout << "resp time advance called/returning" << endl;
            return state.next_internal;
        }

        friend ostringstream& operator<<(ostringstream& os, const typename Responder<TIME>::state_type& i) {
            if (DEBUG) cout << "resp << called" << endl;
            string result = "";
            for (auto impulse_comp : i.impulse.data) {
                result += to_string(impulse_comp) + " ";
            }
            os << "impulse: " << result;
            if (DEBUG) cout << "resp << returning" << endl;
            return os;
        }

    private:

        template <typename T>
        string vector_string (vector<T> v) {
            string result = "";
            for (auto i : v) {
                result += to_string(i) + " ";
            }
            return result;
        }
};

#endif