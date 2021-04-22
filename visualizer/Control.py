from Parser import Parser
from Interface import Interface
import pprint

class Control:

    def __init__ (self):
        self.simLog = "../simulation_results/iter_1_test_output_state.txt"
        self.simConfig = "../input/config.json"

    def start (self):

        data = Parser.getData(self.simLog, self.simConfig)
        #pprint.pprint(data)

        #scaledData = Parser.scaleData(data, [2, 5], [150, 100])

        #print(scaledData)

        Interface.start(data=data, simDims=[5, 5], visDims=[700, 900])  # [500, 700]