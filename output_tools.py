#!/bin/python3


import sys

def parse_msg_file(msg_file):
    start_s = "SubV_defs::logging_out: {"
    end_s   = "}"

    time = float('-inf')
    for line in msg_file:
        #this code may need to change for new/different file formats
        if line.strip().replace('.','',1).isdigit():
            #this is a time change
            time = float(line.strip())
        elif start_s in line:
            #this is a line with particle info
            start_i = line.find(start_s)+len(start_s)
            end_i   = line.find(end_s, start_i)
            while end_i > start_i and line[start_i] == '[' and line.find(']', start_i, end_i) >= 0:
                event = line[start_i+1:line.find(']', start_i, end_i)]
                start_i = line.find('[', start_i+1, end_i)

                '''
                volume_id_start = event.find("subV_id: ")+len("subV_id: ")
                volume_id_end = event.find(",", volume_id_start)
                volume_id = int(event[volume_id_start:volume_id_end])
                '''

                p_id_start = event.find("p_id: ")+len("p_id: ")
                p_id_end = event.find(",", p_id_start)
                p_id = int(event[p_id_start:p_id_end])

                vel_start = event.find("vel: <")+len("vel: <")
                vel_end = event.find(">", vel_start)
                vel = list([float(n) for n in event[vel_start:vel_end].split(' ')])

                pos_start = event.find("pos: <")+len("pos: <")
                pos_end = event.find(">", pos_start)
                pos = list([float(n) for n in event[pos_start:pos_end].split(' ')])

                yield([time, p_id, pos, vel])

def _quantize_state_to_times_output_helper(state, next_time):
    out = dict()
    for key, (update_time, po, ve) in state.items():
        dt = next_time-update_time
        out[key] = list([p+v*dt for p, v in zip(po, ve)])
    return(out)

def quantize_state_to_times(events, times):
    marker = object()
    times_iter = iter(times)
    next_time = next(times_iter)
    state = dict()
    for time, p_id, pos, vel in events:
        if time > next_time:
            #we need to yield a snapshot before advancing
            yield([next_time, _quantize_state_to_times_output_helper(state, next_time)])

            next_time = next(times_iter, marker)
            if next_time is marker:
                #we have run out of requested times, so it is time to return
                return
        state[p_id] = (time, pos, vel)
    #if we got here, we ran out of file before running out of times, so we prattel out all future positions in order
    for t in times_iter:
        yield([t, _quantize_state_to_times_output_helper(state, t)])

def _float_range_helper(start, end, step):
    while start <= end:
        yield(start)
        start += step

if __name__ == "__main__":
    #print(str(sys.argv))
    if len(sys.argv) > 1 and '-h' in sys.argv[1]:
        print(
        'Usage: \n'+
        '\tmessages.txt | python3 output_tools.py                                       #outputs a cleanned sequence of events of the form [time, p_id, [pos], [vel]]\n'+
        '\tpython3 output_tools.py messages.txt                                         #as if messages.txt was piped in\n'+
        '\tpython3 output_tools.py messages.txt <end time>                              #at each time in [0.0, end] with a stepsize of 1.0, print a snapeshot of the state, of the form [time, {p_id:[pos]}]\n'+
        '\tpython3 output_tools.py messages.txt <end time> <timestep size>              #as the last case, but with the specified step size instead of 1.0\n'+
        '\tpython3 output_tools.py messages.txt <end time> <timestep size> <start time> #as the last case, but with the specified start time instead of 0.0\n'
        )
        exit()
    if len(sys.argv) == 2:
        with open(sys.argv[1]) as msg_file:
            for event in parse_msg_file(msg_file):
                print(event)
    elif len(sys.argv) > 2:
        #end | end, step size | end, step size, start
        end  = float(sys.argv[2])
        step = float(sys.argv[3]) if len(sys.argv) > 3 else 1.0
        start = float(sys.argv[4]) if len(sys.argv) > 4 else 0.0
        if(start > end):
            print(f"start:{start} is > end:{end}, no states will be printed")
            exit(-1)
        if(step == 0):
            print("step size is set to 0. This program does not suport a step size of 0")
            exit(-1)
        if(step < 0):
            print(f"step:{step} is <0, we can only walk forwards through the input, we cannot produce states out of order or in reverse order like this")
            exit(-1)
        with open(sys.argv[1]) as msg_file:
            for state in quantize_state_to_times(parse_msg_file(msg_file), _float_range_helper(start, end, step)):
                print(state)

    else:
        for event in parse_msg_file(sys.stdin):
            print(event)
