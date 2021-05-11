from Parser import Parser
from Interface import Interface

class Control:

    def __init__ (self):
        pass

    def start (self, config="config.json"):
        print("Importing data...")
        setup = Parser.importJSON(config)

        print("Parsing data...")
        data = Parser.getData(setup["files"]["states"], setup["files"]["messages"], setup["files"]["config"])

        print("Running visualizer...")
        Interface.start(data=data, simDims=setup["dimensions"]["simulation"], visDims=setup["dimensions"]["visualizer"])