#ifndef TRACKER_HPP
#define TRACKER_HPP

#include <cadmium/modeling/ports.hpp>
#include <cadmium/modeling/message_bag.hpp>

#include <limits>
#include <assert.h>
#include <string>

using namespace cadmium;
using namespace std;

/*
Funcationality:
- Currently just relays reponse messages
- Does not worry about various subV modules as there will be only one for now
- Communicates with lattice coupled model (which, for now, only contains one subV module)

TODO:
- Add support for multiple subV modules
*/

// Port definition
struct Tracker_defs {
    struct response_in : public in_port<___> {};
    struct response_out : public out_port<___> {};
    struct departure_in : public in_port<___> {};
    struct arrival_in : public in_port<___> {};
};

template<typename TIME> class Tracker {
    public:
        // ports definition
        using input_ports = tuple<typename Tracker_defs::response_in,
                                  typename Tracker_defs::departure_in,
                                  typename Tracker_defs::arrival_in>;
        using output_ports = tuple<typename Tracker_defs::response_out>;

        struct state_type {
            // state information
        };
        state_type state;

        Tracker () {
            // initialization
        }

        // internal transition
        void internal_transition () {
            //
        }

        // external transition
        void external_transition () {
            //
        }

        // confluence transition
        void confluence_transition (___, ___) {
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
            //
            return ___
        }
}

#endif