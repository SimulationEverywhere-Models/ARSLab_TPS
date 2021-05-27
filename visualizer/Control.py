from Parser import Parser
from Interface import Interface
from pathlib import Path

class Control:

    def __init__ (self):
        pass

    def start (self, config="config.json"):
        print("Importing data...")

        setup = Parser.importJSON(config)

        print("Parsing data...")
        data = {}
        try:
            data = Parser.getData(setup["files"]["states"], setup["files"]["messages"], setup["files"]["config"])
        except MemoryError:
            print("ERROR: Memory Error")
            print("| Files sizes:")
            for purpose, filename in setup["files"].items():
                size = Path(filename).stat().st_size
                print(f"| {purpose}: {size} bytes ({round(size / 1000000, 2)} MB, {round(size / (2 ** 20), 2)} MiB)")
            raise MemoryError

        print("Running visualizer...")
        Interface.start(data=data, simDims=setup["dimensions"]["simulation"], visDims=setup["dimensions"]["visualizer"])