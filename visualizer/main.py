from Control import Control
import sys

control = Control()

config = "config.json"
if (len(sys.argv) == 2):
    config = sys.argv[1]

control.start(config=config)