#ifndef SUBV_HPP
#define SUBV_HPP

#include <cadmium/modeling/ports.hpp>
#include <cadmium/modeling/message_bag.hpp>

#include <limits>
#include <assert.h>
#include <string>

using namespace cadmium;
using namespace std;

// Port definition
struct SubV_defs {
    struct response_in : public in_port<___> {};
    struct collision_out : public out_port<___> {};
    struct departure_out : public out_port<___> {};
    struct arrival_out : public out_port<___> {};
};

template<typename TIME> class SubV {
    public:
        // ports definition
        // ADD ADJACENCY MESSAGES
        using input_ports = tuple<typename SubV_defs::response_in>;
        using output_ports = tuple<typename SubV_defs::collision_out,
                                   typename SubV_defs::departure_out,
                                   typename SubV_defs::arrival_out>;

        struct state_type {
            // state information
        };
        state_type state;

        SubV () {
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