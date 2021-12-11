import sys
import click
import paho.mqtt.client as mqtt
import logging
import crypto
import time
import subprocess


validActions = ["uppercase", "lowercase", "title-case", "reverse", "shuffle"]

helpMessage = """Usage: mqtt_client.py [OPTIONS] NETID ACTION MESSAGE

  NETID The NetID of the user.

  ACTION Must be uppercase, lowercase, title-case, reverse, or shuffle.

  MESSAGE Message to send to the server.

Options:
  -v, --verbose
  -h, --host TEXT
  -p, --port TEXT
  --help              Show this message and exit."""
messageReceived = False
verboseFlag = False
subFlag = False
peerPublicKey = ""
peerKeySet = False
peerKeyReceived = False
sharedKey = ""


class textColoring:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKCYAN = '\033[96m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    ERROR = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'


# Description: Callback for broker to confirm connections
# Arguments:
#         mqttc       -> mqtt client
#         obj         -> userdata
#         flags       -> response flags sent by the broker
#         rc          -> the connection result
# Return Value: NA
def on_connect(mqttc, obj, flags, rc):
    if (rc == 0):
        connected = "True"
    else:
        connected = "False"
    logging.info(
        f"{textColoring.OKGREEN}Connected:  {connected}{textColoring.ENDC}")


# Description: Callback for when broker has a message for subscribers
# Arguments:
#         mqttc   -> mqtt client
#         obj     -> userdata
#         msg     -> The message from something we are subscribed to
# Return Value: NA
def on_message(mqttc, obj, msg):
    global peerKeySet
    global sharedKey
    if peerKeySet:
        global messageReceived
        messageReceived = True
        messageForUser = crypto.decrypt(msg.payload, sharedKey).decode("utf-8")
        print(f"{textColoring.OKBLUE}{messageForUser}{textColoring.ENDC}")
    else:
        global peerPublicKey
        global peerKeyReceived
        peerPublicKey = msg.payload
        peerKeySet = True
        peerKeyReceived = True

# Description: Callback for when broker completes a publish request mqtt messages
# Arguments:
#         mqttc       -> mqtt client
#         obj         -> userdata
#         mid         -> message ID used for tracking
# Return Value: NA
def on_publish(mqttc, obj, mid):
    logging.info(f"{textColoring.OKGREEN}mid: {str(mid)}{textColoring.ENDC}")


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
    logging.info(F"{textColoring.OKGREEN}Subscribed!{textColoring.ENDC}")


# Description: Make sure the user gave valid arguements and options
# Arguments: clienConfig -> This is a dictionary of the options and arguments
# Return Value:
#       1: invalid
#       0: valid
def checkValidInfo(clientConfig):
    # check action
    for i in range(6):
        if i == 5:
            logging.error(
                f"{textColoring.ERROR}invalid action{textColoring.ENDC}")
            return 1
        elif (validActions[i] == clientConfig.get("action")):
            break

    # check out of range port
    if (clientConfig.get("port") > 65535 or clientConfig.get("port") < 1025):
        logging.error(
            f"{textColoring.ERROR}invalid port: must be greater than 1025 and less than 65536{textColoring.ENDC}")
        return 1

    # check for zero length message. Basically just to protect
    # against Lundrigan's silly tests -_- because what psychopath
    # would enter "" for there message?
    if (clientConfig.get("message") == ""):
        logging.error(
            f"{textColoring.ERROR}invalid message: must be greater than 0 characters{textColoring.ENDC}")
        return 1

    # check for invalid host length.
    if (len(clientConfig.get("host")) < 3 or len(clientConfig.get("host")) > 63):
        logging.error(
            f"{textColoring.ERROR}invalid host: must be greater than 2 characters and less than 64{textColoring.ENDC}")
        return 1

    # check for invalid host.
    command = ['ping', '-c', '1', clientConfig.get("host")]
    if subprocess.call(command, stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT) != 0:
        logging.error(
            f"{textColoring.ERROR}invalid host: Given host won't respond{textColoring.ENDC}")
        return 1

    logging.info(f"{textColoring.OKGREEN}VALID ACTION{textColoring.ENDC}")
    return 0


# Description: Generate a shared key.
# Arguments:
#       mqttc -> the client mqtt object
#       clienConfig -> This is a dictionary of the options and arguments
#       param_file -> specifies values to create key.
# Return Value: None
def diffieHellman(mqttc, clientConfig, param_file="dhparam4096.pem"):
    global subFlag
    global peerPublicKey
    global sharedKey

    privateKey, publicKey = crypto.generate_keyset(param_file)
    # privateKey = privateKey.decode('utf-8')
    # publicKey = publicKey.decode('utf-8')

    diffieTopicResponse = clientConfig.get("netid") + '/key-exchange/response'
    diffieTopicRequest = clientConfig.get("netid") + '/key-exchange/request'

    mqttc.subscribe(diffieTopicResponse, 1)
    while not subFlag:
        time.sleep(.05)
    subFlag = False  # Reset for next sub

    mqttc.publish(diffieTopicRequest, publicKey, 1)
    while not peerKeyReceived:
        time.sleep(.05)

    sharedKey = crypto.create_shared_key(privateKey, peerPublicKey)


@click.command()
@click.argument('netid')
@click.argument('action')
@click.argument('message')
@click.option('-v', '--verbose', is_flag=True)
@click.option('-h', '--host', default="localhost")
@click.option('-p', '--port', default=1884)
def main(netid, action, message, verbose, host, port):
    """NETID The NetID of the user.\n
    ACTION Must be uppercase, lowercase, title-case, reverse, or shuffle.\n
    MESSAGE Message to send to the server."""
    global verboseFlag
    verboseFlag = verbose

    # Set log level
    if (verbose):
        logging.basicConfig(level=logging.INFO)

    # This was strictly for organization. I also thought it would be good if
    # I ever needed to pass it to many functions if the program got bigger.
    clientConfig = {"netid": netid, "action": action, "message": message,
                    "verbose": verbose, "host": host, "port": port}

    # Make sure the user input was valid
    if (checkValidInfo(clientConfig) == 1):
        return 1

    logging.info(
        f"{textColoring.OKGREEN}paramters: {clientConfig}{textColoring.ENDC}")

    # create client object and assign callback functions
    mqttc = mqtt.Client()
    mqttc.on_message = on_message
    mqttc.on_connect = on_connect
    mqttc.on_publish = on_publish
    mqttc.on_subscribe = on_subscribe

    # client connect and start
    try:
        mqttc.connect(clientConfig.get("host"), clientConfig.get("port"), 60)
    except:
        logging.error(
            f"{textColoring.ERROR}Could not connect to broker.{textColoring.ENDC}")
        return 1
    mqttc.loop_start()

    # key exchange
    diffieHellman(mqttc, clientConfig)

    # client subscribtion
    subscribeTopic = clientConfig.get(
        "netid") + '/' + clientConfig.get("action") + '/response'
    mqttc.subscribe(subscribeTopic, 1)
    while not subFlag:
        time.sleep(.05)
    
    # client publish
    publishTopic = clientConfig.get(
        "netid") + '/' + clientConfig.get("action") + '/request'
    infot = mqttc.publish(publishTopic, crypto.encrypt(clientConfig.get("message"), sharedKey), 1)
    infot.wait_for_publish()

    # Wait for message
    while not messageReceived:
        time.sleep(.05)


if __name__ == "__main__":
    main()
