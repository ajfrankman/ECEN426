import json

def jsonSerialize(payload):
    return json.dumps(payload, indent = 4)

def jsonDeserialize(payload):
    return json.loads(json_payload)


payload = {"timestamp" : 1000000, "name" : "fr4nkm4n", "message" : "hello world!"}


colorDict = {"green" : '\033[92m', "blue" : '\033[94m', "default" : '\033[0m'}

green = "green"
blue = "blue"
red = "red"
default = "default"


def logInfoColor(logMessage, color):
    color = color.lower()
    colorCode = colorDict.get(color, '\033[0m')
    defaultCode = '\033[0m'
    print(f"{colorCode}{logMessage}{defaultCode}")


logInfoColor("hello!", "green");
print("test")

# print(f"{test}")

# json_payload = jsonSerialize(payload)
# print(f"json_payload: {json_payload}")
# print(f"json deserial: {jsonDeserialize(json_payload)}")
