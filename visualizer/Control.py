from Parser import Parser
from Interface import Interface

class Control:

    def __init__ (self):
        pass

    def start (self, config="config.json"):
        setup = Parser.importJSON(config)
        data = Parser.getData(setup["files"]["states"], setup["files"]["messages"], setup["files"]["config"])
        Interface.start(data=data, simDims=setup["dimensions"]["simulation"], visDims=setup["dimensions"]["visualizer"])