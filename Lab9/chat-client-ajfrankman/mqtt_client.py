import time
import click
import paho.mqtt.client as mqtt
import logging
import json
import threading
from tkinter import *


validActions = ["uppercase", "lowercase", "title-case", "reverse", "shuffle"]

# TODO change helpMessage
helpMessage = """Usage: mqtt_client.py [OPTIONS] NETID ACTION MESSAGE

  NETID The NetID of the user.

  ACTION Must be uppercase, lowercase, title-case, reverse, or shuffle.

  MESSAGE Message to send to the server.

Options:
  -v, --verbose
  -h, --host TEXT
  -p, --port TEXT
  --help              Show this message and exit."""

subFlag = False

username = ""
mqttc = 0
messageEntry = 0
messageBox = 0
statusBox = 0

online = False

messageSub = "chat/+/message"
statusSub = "chat/+/status"
connectedStatus = False

colorDict = {"green" : '\033[92m', "blue" : '\033[94m', "default" : '\033[0m', "red" : '\033[91m'}
userStatusDict = {}

# Description: loginfo with coloring for pretty stuff.
# Arguments:
#         message: text to color and log
#         color: color string wanted. Must be in color dict
# Return Value: NA
def logInfoColor(logMessage, color):
    color = color.lower()
    colorCode = colorDict.get(color, '\033[0m')
    defaultCode = colorDict["default"]
    logging.info(f"{colorCode}{logMessage}{defaultCode}")

# Description: logerror with coloring for pretty stuff.
# Arguments:
#         message: text to color and log
#         color: color string wanted. Must be in color dict
# Return Value: NA
def logError(logMessage):
    colorCode = colorDict.get("red", '\033[0m')
    defaultCode = colorDict["default"]
    logging.error(f"{colorCode}{logMessage}{defaultCode}")


# Description: Callback for broker to confirm connections
# Arguments:
#         mqttc       -> mqtt client
#         obj         -> userdata
#         flags       -> response flags sent by the broker
#         rc          -> the connection result
# Return Value: NA
def on_connect(mqttc, obj, flags, rc):
    if  rc == 0: 
        global connectedStatus
        connectedStatus = True

        statusPayload = createStatusPayload(username, 1)
        statusPubTopic = f"chat/{username}/status"
        mqttc.publish(topic = statusPubTopic, payload = statusPayload, qos = 1, retain = True)

        logInfoColor("Connected", "green");
        # client message subscription
        logInfoColor(f"subcribing to {messageSub}...", "green")
        mqttc.subscribe(messageSub, 1)
        # client status subscription
        logInfoColor(f"Subscribing to {statusSub}...", "green")
        mqttc.subscribe(statusSub, 1)
    else:
        mqttc.reconnect()



# Description: Callback for when broker has a message for subscribers
# Arguments:
#         mqttc   -> mqtt client
#         obj     -> userdata
#         msg     -> The message from something we are subscribed to
# Return Value: NA
def on_message(mqttc, obj, msg):
    global messageBox, username

    payload = jsonDeserialize(msg.payload.decode("utf-8"))

    if(len(payload) >2):
        timeStamp = payload['timestamp']
        timeStamp = time.strftime('%Y-%m-%d %H:%M:%S', time.localtime(timeStamp))
        name = payload['name']
        message = payload['message']
        if (username == name):
            formattedMessage = f"""\n{message} \n^ sent by you at {timeStamp}\n"""
        else:
            formattedMessage = f"""\n{message} \n^ sent by {name} at {timeStamp}\n"""
        messageBox.insert(END, formattedMessage)
    else:
        name = payload['name']
        status = payload['online']
        if (status == 0): status = "offline"
        else: status = "online"

        #update User status dict
        userStatusDict[name] = status

        if (username == name):
            formattedMessage = f"You are now {status}.\n"
        else:
            formattedMessage = f"{name} is now {status}.\n"
        messageBox.insert(END, formattedMessage)



# check for invalid data



# Description: Callback for when broker completes a publish request mqtt messages
# Arguments:
#         mqttc       -> mqtt client
#         obj         -> userdata
#         mid         -> message ID used for tracking
# Return Value: NA
def on_publish(mqttc, obj, mid):
    logInfoColor(f"mid: {str(mid)}", "green")


# Description: Callback for when broker completes a subscribe request mqtt messages
# Arguments:
#         mqttc       -> mqtt client
#         obj         -> userdata
#         mid         -> message ID used for tracking
#         granted_qos -> Quality of service level
# Return Value: NA
def on_subscribe(mqttc, obj, mid, granted_qos):
    global subFlag
    subFlag = True
    logInfoColor("Subscribed!", "green")

# Description: Basically just an alias function that made things easier for me. Just serializes and object into JSON
# Arguments:
#         payload: message to serialize
# Return Value: returns the serialized object
def jsonSerialize(payload):
    return json.dumps(payload, indent = 4)

# Description: Basically just an alias function that made things easier for me. Just deserializes JSON into object
# Arguments:
#         payload: message to deserialize
# Return Value: returns the deserialized object
def jsonDeserialize(payload):
    return json.loads(payload)

# Description: given necesarry info create a serialize status object.
# Arguments:
#         name: name of user
#         status: online status
# Return Value: returns the serialized status object.
def createStatusPayload(name, status):
    tempDict = {"name" : name, "online" : status}
    return jsonSerialize(tempDict) 

# Description: given necesarry info create a serialize message object.
# Arguments:
#         name: name of user
#         message: the payload or message
# Return Value: returns the serialized status object.
def createMessagePayload(name, message):
    tempDict = {"timestamp" : time.time(), "name" : name, "message" : message}
    return jsonSerialize(tempDict)

# Description: grabs the user input from the GUI and sends it to the broker.
# Arguments:
# Return Value:
def sendMessage():

    # Send message
    userMessage = messageEntry.get()
    messageEntry.delete(0, END)
    messagePayload = createMessagePayload(username, userMessage)
    messagePubTopic = f"chat/{username}/message"
    mqttc.publish(messagePubTopic, messagePayload, 1)

# Description: Check all user's more recent status update and update the status column
# Arguments:
# Return Value:
def updateStatuses():
    global userStatusDict, statusBox
    while(True):
        statusBox.delete(0, END)
        for x in userStatusDict:
            nameStatus = f"{x} {userStatusDict[x]}"
            statusBox.insert(END, nameStatus)
        time.sleep(1)

# Description: Lays out the gui and gives it the callback functions needed to function
# Arguments:
#         port: port to run on
#         host: host to use
# Return Value:
def startGUI(port, host):
    ###### main:
    global messageEntry, messageBox, mqttc, username, statusBox

    # create client object and assign callback functions
    mqttc = createMQTTClient()

    window = Tk()
    window.title(f"{username} | Mqtt Chat Client")
    window.configure(background='black')

    #create message label
    Label(window, text='Enter your message: ', bg = 'black', fg = 'white', font = 'none 12 bold').grid(row = 1, column = 0, sticky = W, padx=(10, 10), pady=(10, 10))
    #create a text entry box

    messageEntry = Entry(window, width = 20, bg = "white")
    messageEntry.grid(row=2, column=0, sticky=W, padx=(10, 10), pady=(0, 10))

    #add send button
    Button(window, text = "Send Message", width = 13, command = sendMessage).grid(row = 3, column = 0, sticky = W, padx=(10, 10), pady=(0, 15))

    #create a list box
    messageBox = Text(window, width = 50, height = 25)
    messageBox.grid(row = 4, column = 0, columnspan = 2, sticky = W, padx=(10, 10), pady=(10,10))

    statusBox = Listbox(window, width = 25, height = 25)
    statusBox.grid(row = 0, column = 2, columnspan = 2, rowspan=8, sticky = W, padx=(10, 10), pady=(10, 10))

    # client connect and start
    # client subscribes to who it wants in on_connect callback
    try:
        mqttc.connect(host, port, 60)
    except:
        logError("Could not connect to broker.")
        return 0

    x = threading.Thread(target=updateStatuses)
    x.start()
    mqttc.loop_start()




    ###### runs the main loop
    window.mainloop()

# Description: create the mqtt client object. Makes main a little cleaner.
# Arguments:
# Return Value: returns the mqtt client object
def createMQTTClient():
    global username
    mqttc = mqtt.Client()
    mqttc.on_message = on_message
    mqttc.on_connect = on_connect
    mqttc.on_publish = on_publish
    mqttc.on_subscribe = on_subscribe
    lastWillPayload = createStatusPayload(username, 0)
    mqttc.will_set(f"chat/{username}/status", payload = lastWillPayload, qos=1, retain = True)
    return mqttc


# Description: Entry point for the program. Does intial setup then starts the GUI.
# Arguments:
#         name: name of user
#         verbose: verbose flag. Defaults to not verbose
#         port: which port to use. Has default value 1885
#         host: which hose to use. Has default value "localhost"
# Return Value: returns the serialized status object.
@click.command()
@click.argument('name')
@click.option('-v', '--verbose', is_flag=True)
@click.option('-h', '--host', default="localhost")
@click.option('-p', '--port', default=1885)
def main(name, verbose, host, port):
    global username, mqttc
    username = name

    # Set log level
    if (verbose):
        logging.basicConfig(level=logging.INFO)

    startGUI(port, host)



if __name__ == "__main__":
    main()