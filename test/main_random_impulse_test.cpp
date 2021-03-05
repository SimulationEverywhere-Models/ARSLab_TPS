// Cadmium Simulator headers
#include <cadmium/modeling/ports.hpp>
#include <cadmium/modeling/dynamic_model.hpp>
#include <cadmium/modeling/dynamic_model_translator.hpp>
#include <cadmium/engine/pdevs_dynamic_runner.hpp>
#include <cadmium/logger/common_loggers.hpp>

// Time class header
#include <NDTime.hpp>

// Message structures
#include "../data_structures/impulse_message.hpp"

// Atomic model headers
#include <cadmium/basic_model/pdevs/iestream.hpp>  // atomic model for inputs
#include "../atomics/random_impulse.hpp"

// C++ libraries
#include <iostream>
#include <string>

using namespace std;
using namespace cadmium;
using namespace cadmium::basic_models::pdevs;

using TIME = float;

/*** Define input ports for coupled models ***/
// no input ports

/*** Define output ports for coupled models ***/
struct top_out: public out_port<impulse_message_t>{};

int main () {
    /*** RI atomic model instantiation ***/
    shared_ptr<dynamic::modeling::model> random_impulse;
    random_impulse = dynamic::translate::make_dynamic_atomic_model<RandomImpulse, TIME>("random_impulse");

    /*** TOP MODEL ***/
    dynamic::modeling::Ports iports_TOP;
    iports_TOP = {};
    dynamic::modeling::Ports oports_TOP;
    oports_TOP = {typeid(top_out)};
    dynamic::modeling::Models submodels_TOP;
    submodels_TOP = {random_impulse};
    dynamic::modeling::EICs eics_TOP;
    eics_TOP = {};
    dynamic::modeling::EOCs eocs_TOP;
    eocs_TOP = {
        dynamic::translate::make_EOC<RandomImpulse_defs::impulse_out, top_out>("random_impulse")
    };
    dynamic::modeling::ICs ics_TOP;
    ics_TOP = {};
    shared_ptr<dynamic::modeling::coupled<TIME>> TOP;
    TOP = make_shared<dynamic::modeling::coupled<TIME>>(
        "TOP", submodels_TOP, iports_TOP, oports_TOP, eics_TOP, eocs_TOP, ics_TOP
    );

    /*** Loggers ***/
    static ofstream out_messages("../simulation_results/random_impulse_test_output_messages.txt");
    struct oss_sink_messages{
        static ostream& sink(){
            return out_messages;
        }
    };
    static ofstream out_state("../simulation_results/random_impulse_test_output_state.txt");
    struct oss_sink_state{
        static ostream& sink(){
            return out_state;
        }
    };

    using state=logger::logger<logger::logger_state, dynamic::logger::formatter<TIME>, oss_sink_state>;
    using log_messages=logger::logger<logger::logger_messages, dynamic::logger::formatter<TIME>, oss_sink_messages>;
    using global_time_mes=logger::logger<logger::logger_global_time, dynamic::logger::formatter<TIME>, oss_sink_messages>;
    using global_time_sta=logger::logger<logger::logger_global_time, dynamic::logger::formatter<TIME>, oss_sink_state>;

    using logger_top=logger::multilogger<state, log_messages, global_time_mes, global_time_sta>;

    /*** Runner call ***/
    dynamic::engine::runner<TIME, logger_top> r(TOP, {0});
    //r.run_until(NDTime("00:05:00:000"));
    r.run_until(TIME(100));
    return 0;
}