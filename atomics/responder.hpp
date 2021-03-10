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

#include "../data_structures/impulse_message.hpp"

using namespace cadmium;
using namespace std;

using json = nlohmann::json;

// Port definition
struct Responder_defs {
    //struct transition_in : public in_port<___> {};
    struct impulse_in : public in_port<impulse_message_t> {};
    //struct collision_in : public in_port<___> {};
    //struct attachment_out : public out_port<___> {};  // not applicable for now (tethering)
    //struct detachment_out : public out_port<___> {};  // not applicable for now (tethering)
    struct impulse_out : public out_port<impulse_message_t> {};  // unsure of purpose (message type may be wrong)
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
                                  typename Responder_defs::impulse_in>;
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
            impulse_message_t impulse;
        };
        state_type state;

        Responder () {
            //
        }

        Responder (json j) {
            cout << "Responder constructor received JSON: " << j << endl;
            state.particle_data = j;
        }

        // internal transition
        void internal_transition () {
            cout << "resp internal transition called" << endl;
            // TODO: simulate particle decay (possibly in another module)
            state.next_internal = numeric_limits<TIME>::infinity();  // the response module should not have internal events
            cout << "resp internal transition finished" << endl;
        }

        // external transition
        void external_transition (TIME e, typename make_message_bags<input_ports>::type mbs) {
            cout << "resp external transition called" << endl;
            if (get_messages<typename Responder_defs::impulse_in>(mbs).size() > 1) {
                assert(false && "Responder received more than one concurrent message");
            }
            for (const auto &x : get_messages<typename Responder_defs::impulse_in>(mbs)) {
                state.impulse = x;
                // TODO:
                // set response message to change the trajectory of affected particles
                // is this the new trajectory or an impulse to make that happen?
            }
            cout << "resp external transition finish" << endl;
        }

        // confluence transition
        void confluence_transition (TIME e, typename make_message_bags<input_ports>::type mbs) {
            cout << "resp confluence transition called" << endl;
            internal_transition();
            external_transition(TIME(), move(mbs));
            cout << "resp confluence transition finishing" << endl;
        }

        // output function
        typename make_message_bags<output_ports>::type output () const {
            cout << "resp output called" << endl;
            typename make_message_bags<output_ports>::type bags;
            vector<impulse_message_t> bag_port_out;
            bag_port_out.push_back(state.impulse);
            get_messages<typename Responder_defs::impulse_out>(bags) = bag_port_out;
            cout << "resp output returning" << endl;
            return bags;
        }

        // time advance function
        TIME time_advance () const {
            cout << "resp time advance called/returning" << endl;
            return state.next_internal;
        }

        friend ostringstream& operator<<(ostringstream& os, const typename Responder<TIME>::state_type& i) {
            cout << "resp << called" << endl;
            string result = "";
            for (auto impulse_comp : i.impulse.impulse) {
                result += to_string(impulse_comp) + " ";
            }
            os << "Responder impulse: " << result;
            cout << "resp << returning" << endl;
            return os;
        }
};

#endif