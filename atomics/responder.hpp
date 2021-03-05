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

#include "../data_structures/impulse_message.hpp"

using namespace cadmium;
using namespace std;

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
            TIME next_internal;
            impulse_message_t impulse;
        };
        state_type state;

        Responder () {
            //
        }

        // internal transition
        void internal_transition () {
            state.next_internal = numeric_limits<TIME>::infinity();  // the response module should not have internal events
        }

        // external transition
        void external_transition (TIME e, typename make_message_bags<input_ports>::type mbs) {
            if (get_messages<typename Responder_defs::impulse_in>(mbs).size() > 1) {
                assert(false && "Responder received more than one concurrent message");
            }
            for (const auto &x : get_messages<typename Responder_defs::impulse_in>(mbs)) {
                state.impulse = x;
                // TODO:
                // set response message to change the trajectory of affected particles
                // is this the new trajectory or an impulse to make that happen?
            }
        }

        // confluence transition
        void confluence_transition (TIME e, typename make_message_bags<input_ports>::type mbs) {
            internal_transition();
            external_transition(TIME(), move(mbs));
        }

        // output function
        typename make_message_bags<output_ports>::type output () const {
            typename make_message_bags<output_ports>::type bags;
            vector<impulse_message_t> bag_port_out;
            bag_port_out.push_back(state.impulse);
            get_messages<typename Responder_defs::impulse_out>(bags) = bag_port_out;
            return bags;
        }

        // time advance function
        TIME time_advance () const {
            return state.next_internal;
        }

        friend ostringstream& operator<<(ostringstream& os, const typename Responder<TIME>::state_type& i) {
            string result = "";
            for (auto impulse_comp : i.impulse.impulse) {
                result += to_string(impulse_comp) + " ";
            }
            os << "Responder impulse: " << result;
            return os;
        }
};

#endif