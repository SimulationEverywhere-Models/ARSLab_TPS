import subprocess
import json

demos = {
    1 : {
        "name" : "General (2D, 30 particles, no random impulses)",
        "address" : "2D_30p_noRI",
        "simDim" : [-15, 15]
    },
    2 : {
        "name" : "Loading (2D, 3 particles, no random impulses)",
        "address" : "2D_3p_bounce_noRI",
        "simDim" : [-5, 5]
    },
    3 : {
        "name" : "Random Impulses (2D, 4 particles, with random impulses)",
        "address" : "2D_4p_wRI",
        "simDim" : [-5, 5]
    },
    4 : {
        "name" : "3D (3D, 2 particles, no random impulses)",
        "address" : "3D_2p_noRI",
        "simDim" : [-5, 5]
    },
    5 : {
        "name" : "3D (3D, 2 particles, with random impulses)",
        "address" : "3D_2p_wRI",
        "simDim" : [-5, 5]
    }
}

def getVisualizerConfig (address, simDim):
    return {
        "files" : {
            "states" : f"../simulation_results/{address}/iter_1_test_output_state.txt",
            "messages" : f"../simulation_results/{address}/iter_1_test_output_messages.txt",
            "config" : f"../input/config_{address}.json"
        },
        "dimensions" : {
            "simulation" : simDim,
            "visualizer" : [900, 700]
        }
    }

response = ""
while (True):
    for key in demos:
        name = demos[key]["name"]
        print(f"{key}) {name}")
    response = input("> ")
    if (response == "exit" or response == "quit"):
        break
    if (not response.isdigit()):
        print("NaN")
        continue
    selection = int(response)
    if (selection not in demos):
        print("KeyError")
        continue
    address = demos[selection]["address"]

    # create temporary config
    with open("config_temp.json", "w") as f:
        f.write(json.dumps(getVisualizerConfig(address, demos[selection]["simDim"])))

    # run visualizer
    subprocess.run(["py", "../visualizer/main.py", f"../demo/config_temp.json"])

    print()