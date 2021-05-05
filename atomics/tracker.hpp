/*
tracker
Responsibilities
- route messages to the appropriate subV module(s)
*/

#ifndef TRACKER_HPP
#define TRACKER_HPP

#include <cadmium/modeling/ports.hpp>
#include <cadmium/modeling/message_bag.hpp>

#include <limits>
#include <assert.h>
#include <string>
#include <map>
#include <vector>

#include "../test/tags.hpp"  // debug tags
#include "../utilities/vector_utils.hpp"  // vector functions

#include "../data_structures/message.hpp"

using namespace cadmium;
using namespace std;

/*
Functionality:
- Currently just relays reponse messages.
- Does not worry about various subV modules as there will be only one for now.
- Communicates with lattice coupled model (which, for now, only contains one subV module).

TODO:
- Add support for multiple subV modules
*/

// Port definition
struct Tracker_defs {
    struct response_in : public in_port<message_t> {};
    struct response_out : public out_port<tracker_message_t> {};
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
            //map<int, vector<int>> particle_data;
            vector<tracker_message_t> messages;
            map<int, vector<int>> particle_locations;  // particle_id, {subV_id}
        };
        state_type state;

        Tracker () {
            if (DEBUG_TR) cout << "Tracker constructor called" << endl;
            // initialization
            // TODO: should be initialized or calculated from arguments
            state.particle_locations = {
                {1, {1}},
                {2, {1}},
                {3, {1}}
            };
        }

        // internal transition
        void internal_transition () {
            if (DEBUG_TR) cout << "tracker internal transition called" << endl;
            state.messages.clear();
            state.next_internal = numeric_limits<TIME>::infinity();
            if (DEBUG_TR) cout << "tracker internal transition finishing" << endl;
        }

        // external transition
        void external_transition (TIME e, typename make_message_bags<input_ports>::type mbs) {
            if (DEBUG_TR) cout << "tracker external transition called" << endl;
            if (DEBUG_TR && get_messages<typename Tracker_defs::response_in>(mbs).size() > 1) {
                cout << "NOTE: Tracker received more than one concurrent message" << endl;
            }
            // Handle velocity messages from responder
            for (const auto &x : get_messages<typename Tracker_defs::response_in>(mbs)) {
                // create a message and add relevant the subV_ids
                //state.messages.push_back(tracker_message_t(x, state.particle_locations[x.particle_id]));
                state.messages.push_back(tracker_message_t(x, {1}));  // temporarily send all messages to subV 1
            }
            //if (DEBUG_TR) cout << "tr added messages (# messages:" << state.messages.size() << ")" << endl;
            state.next_internal = 0;
            if (DEBUG_TR) cout << "tracker external transition finishing" << endl;
        }

        // confluence transition
        void confluence_transition (TIME e, typename make_message_bags<input_ports>::type mbs) {
            if (DEBUG_TR) cout << "tracker confluence transition called" << endl;
            internal_transition();
            external_transition(e, move(mbs));
            if (DEBUG_TR) cout << "tracker confluence transition finishing" << endl;
        }

        // output function
        typename make_message_bags<output_ports>::type output () const {
            if (DEBUG_TR) cout << "tracker output called" << endl;
            typename make_message_bags<output_ports>::type bags;
            vector<tracker_message_t> bag_port_out;
            bag_port_out = state.messages;
            get_messages<typename Tracker_defs::response_out>(bags) = bag_port_out;
            if (DEBUG_TR) cout << "tracker output returning" << endl;
            return bags;
        }

        // time advance function
        TIME time_advance () const {
            if (DEBUG_TR) cout << "tracker time advance called/returning" << endl;
            return state.next_internal;
        }

        friend ostringstream& operator<<(ostringstream& os, const typename Tracker<TIME>::state_type& i) {
            if (DEBUG_TR) cout << "tracker << called" << endl;
            string result = "";
            // include information on which subV a particle is located in
            for (auto msg : i.messages) {
                result += "[(p_id:" + to_string(msg.particle_id) + ", sv_ids:[" + VectorUtils::get_string<int>(msg.subV_ids) + "]) ";
                result += VectorUtils::get_string<float>(msg.data, true) + "] ";
            }
            os << "velocity messages: " << result;
            if (DEBUG_TR) cout << "tracker << returning" << endl;
            return os;
        }
};

#endif