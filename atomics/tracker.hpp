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
            vector<tracker_message_t> message;
            map<int, vector<int>> particle_locations;  // particle_id, {subV_id}
        };
        state_type state;

        Tracker () {
            // initialization
            // TODO: should be initialized from arguments
            state.particle_locations = {
                {1, {1}},
                {2, {1}}
            };
        }

        // internal transition
        void internal_transition () {
            state.next_internal = numeric_limits<TIME>::infinity();
        }

        // external transition
        void external_transition (TIME e, typename make_message_bags<input_ports>::type mbs) {
            if (DEBUG_TR) cout << "tr external transition called" << endl;
            if (get_messages<typename Tracker_defs::response_in>(mbs).size() > 1) {
                cout << "NOTE: Tracker received more than one concurrent message" << endl;
            }
            // Handle velocity messages from responder
            for (const auto &x : get_messages<typename Tracker_defs::response_in>(mbs)) {
                // create a message and add relevant the subV_ids
                state.message = tracker_message_t(x, state.particle_locations[x.particle_id]);
            }
            if (DEBUG_TR) cout << "tr added messages (# subVs:" << state.message.subV_ids.size() << ")" << endl;
        }

        // confluence transition
        void confluence_transition (TIME e, typename make_message_bags<input_ports>::type mbs) {
            internal_transition();
            external_transition(TIME(), move(mbs));
        }

        // output function
        typename make_message_bags<output_ports>::type output () const {
            typename make_message_bags<output_ports>::type bags;
            vector<tracker_message_t> bag_port_out;
            bag_port_out = state.message;
            get_messages<typename Tracker_defs::response_out>(bags) = bag_port_out;
            return bags;
        }

        // time advance function
        TIME time_advance () const {
            return state.next_internal;
        }

        friend ostringstream& operator<<(ostringstream& os, const typename Tracker<TIME>::state_type& i) {
            if (DEBUG_TR) cout << "tracker << called" << endl;
            string result = "";
            for (auto msg : i.messages) {
                result += "[(pid:" + to_string(msg.particle_id) + ", sv_id:" + to_string(msg.subV_id) + ") ";
                for (auto velocity_comp : msg.data) {
                    result += to_string(velocity_comp) + " ";
                }
                result += "] ";
            }
            os << "velocities: " << result;
            if (DEBUG_TR) cout << "tracker << returning" << endl;
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