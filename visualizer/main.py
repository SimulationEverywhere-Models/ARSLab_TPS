# Carleton University - ARSLab
# Thomas Roller
# Initial operations

from Control import Control
import sys

control = Control()

config = "config.json"
if (len(sys.argv) == 2):
    config = sys.argv[1]

try:
    control.start(config=config)
except MemoryError:
    sys.exit(1)