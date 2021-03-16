#ifndef TRACKER_HPP
#define TRACKER_HPP

#include <cadmium/modeling/ports.hpp>
#include <cadmium/modeling/message_bag.hpp>

#include <limits>
#include <assert.h>
#include <string>
#include <map>
#include <vector>

#include "../data_structures/message.hpp"

using namespace cadmium;
using namespace std;

/*
Functionality:
- Currently just relays reponse messages
- Does not worry about various subV modules as there will be only one for now
- Communicates with lattice coupled model (which, for now, only contains one subV module)

TODO:
- Add support for multiple subV modules
*/

// Port definition
struct Tracker_defs {
    struct response_in : public in_port<message_t> {};
    struct response_out : public out_port<message_t> {};
    //struct departure_in : public in_port<___> {};
    //struct arrival_in : public in_port<___> {};
};

template<typename TIME> class Tracker {
    public:
        // ports definition
        /*
        using input_ports = tuple<typename Tracker_defs::response_in,
                                  typename Tracker_defs::departure_in,
                                  typename Tracker_defs::arrival_in>;
        using output_ports = tuple<typename Tracker_defs::response_out>;
        */

        using input_ports = tuple<typename Tracker_defs::response_in>;
        using output_ports = tuple<typename Tracker_defs::response_out>;

        struct state_type {
            // state information
            TIME next_internal;
            map<int, vector<int>> particle_data;  // particle_id, {subV_id}
            vector<float> velocity;
        };
        state_type state;

        Tracker () {
            // initialization
            // TODO: should be initialized from arguments
            state.particle_data = {
                {1, {1, 2}}
            };
        }

        // internal transition
        void internal_transition () {
            // tracker module does not have internal events
            state.next_internal = numeric_limits<TIME>::infinity();
        }

        // external transition
        void external_transition (TIME e, typename make_message_bags<input_ports>::type mbs) {
            if (get_messages<typename Tracker_defs::response_in>(mbs).size() > 1) {
                assert(false && "Tracker received more than one concurrent message");
            }
            // Handle velocity messages from responder
            for (const auto &x : get_messages<typename Tracker_defs::response_in>(mbs)) {
                state.velocity = x;
            }
        }

        // confluence transition
        void confluence_transition (TIME e, typename make_message_bags<input_ports>::type mbs) {
            //
        }

        // output function
        typename make_message_bags<output_ports>::type output () const {
            typename make_message_bags<output_ports>::type bags;
            vector<message_t> bag_port_out;
            bag_port_out.push_back(state.velocity);
            get_messages<typename Tracker_defs::response_out>(bags) = bag_port_out;
            return bags;
        }

        // time advance function
        TIME time_advance () const {
            return state.next_internal;
        }
}

#endif