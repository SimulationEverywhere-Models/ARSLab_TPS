import plotly.express as px
import pandas as pd
import sys

def parseLine (line):
    result = []
    timeStart = line.find("t:") + 3
    timeEnd = line.find("):")
    result.append(line[timeStart:timeEnd])
    result.append(line[timeEnd + 3:-1])
    return result

inputFilename = "cache_size.txt"
if (len(sys.argv) == 2):
    inputFilename = sys.argv[1]

with open(inputFilename, "r") as f:
    data = f.readlines()

data = [parseLine(line) for line in data]

dataFrame = pd.DataFrame(data=data, columns=["time", "cache size"])

graph = px.line(dataFrame, x="time", y="cache size", title="Collision Cache Size")
graph.update_xaxes(type="linear")
graph.update_yaxes(type="linear")
#graph.update_yaxes(autorange="reversed")

graph.show()