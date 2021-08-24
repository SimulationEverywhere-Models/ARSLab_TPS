# Carleton University - ARSLab
# Thomas Roller
# Control class, responsible for managing the preparation of data and initialization of the GUI

from Parser import Parser
from Interface import Interface
from pathlib import Path

class Control:

    def __init__ (self):
        pass

    # start the program
    # args:
    #     config: name of visualizer configuration file
    # return:
    #     N/A
    def start (self, config="config.json"):
        print("Importing data...")

        # import visualizer configuration file
        setup = Parser.importJSON(config)

        print("Parsing data...")
        data = {}
        try:
            # attempt to load and prepare data (caught if a MemoryError is raised)
            # - state log parsing method can be set with the "transient" argument
            #data = Parser.getData(setup["files"]["states"], setup["files"]["messages"], setup["files"]["config"], setup["settings"]["stateLoading_isTransient"])
            data = Parser.getData(setup["files"]["messages"], setup["files"]["config"])
        except MemoryError:
            # one or more files were collectively too large for Python to load into memory
            # print information for user (file sizes and suggested action(s))
            print("ERROR: Memory Error")
            print("| Files sizes:")
            for purpose, filename in setup["files"].items():
                size = Path(filename).stat().st_size
                print(f"| {purpose}: {size} bytes ({round(size / 1000000, 2)} MB, {round(size / (2 ** 20), 2)} MiB)")
            print("Try setting file loading as transient in visualizer configuration")
            raise MemoryError

        print("Running visualizer...")
        Interface.start(data=data, simDims=setup["dimensions"]["simulation"], visDims=setup["dimensions"]["visualizer"])