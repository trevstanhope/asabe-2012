#!/usr/bin/python
# Worker.py

# Imports
from serial import SerialException
from socket import *
from numpy import *
from PIL import Image 
from cv2 import VideoCapture
from time import *
import numpy
import socket
import time
import json
import serial
import sys
import ast

# Setup
ADDRESS_IN = ('10.42.0.1',50000)
ADDRESS_OUT = ('10.42.0.3',50001)
BUFFER_SIZE = 4096
QUEUE_MAX = 5
BAUD = 9600
DEVICE = '/dev/ttyS0' # '/dev/ttyS0' for AlaMode
CAMERA_INDEX = 0
WIDTH = 640
HEIGHT = 480
CENTER = WIDTH/2
THRESHOLD = CENTER/4
ERROR = pi/32
RANGE = WIDTH/1.5
BIAS = 2
MINIMUM_COLOR = 0.01
MINIMUM_SIZE = WIDTH/32
INTERVAL = 1
MAX_CONNECTIONS = 5
TURN = pi/16
TRAVEL = 0.5
ZONE_X = 8
ZONE_Y = 8
ERROR_NONE = 0
ERROR_CONNECTION = 255
ERROR_CLOSE = 1
ERROR_FAR = 2
ERROR_LOAD = 3
ERROR_ACTION = 4

# Class Worker
class Worker:
  ## Initialize Worker robot.
  def __init__(self):
    try:
      print('Setting up Camera...')
      self.camera = VideoCapture(CAMERA_INDEX)
      self.camera.set(3,WIDTH)
      self.camera.set(4,HEIGHT)
      print('...Success.')
    except Exception:
      print('...Failure.')
    try:
      print('Setting up Controller...')
      self.arduino = serial.Serial(DEVICE, BAUD)
      print('...Success.')
    except Exception:
      print('...Failure.')
    try:
      print('Initializing Worker...')
      self.connected_out = False
      self.connected_in = False
      self.command = None
      self.response = None
      self.action = None
      self.error = None
      self.error_number = None
      self.previous_actions = []
      self.green_objects = []
      self.red_objects = []
      self.blue_objects = []
      self.gathered = 0
      self.stacked = False
      self.returned = False
      self.x = 0
      self.y = 0
      self.orientation = 0
      print('...Success.')
    except Exception:
      print('...Failure.')

  ## Capture image then identify target objects.
  def use_camera(self):
    (success, frame) = self.camera.read()
    raw = Image.fromarray(frame)
    BGR = raw.split()
    B = array(BGR[0].getdata(), dtype=float32)
    G = array(BGR[1].getdata(), dtype=float32)
    R = array(BGR[2].getdata(), dtype=float32)
    NDI_G = ((BIAS)*G)/(R+B+MINIMUM_COLOR)
    NDI_R = ((BIAS)*R)/(G+B+MINIMUM_COLOR)
    NDI_B = ((BIAS)*B)/(R+G+MINIMUM_COLOR)
    # Reshape
    green_image = NDI_G.reshape(HEIGHT,WIDTH)
    blue_image = NDI_B.reshape(HEIGHT,WIDTH)
    red_image = NDI_R.reshape(HEIGHT,WIDTH)
    # Columns
    green_columns = green_image.sum(axis=0)
    blue_columns = blue_image.sum(axis=0)
    red_columns = red_image.sum(axis=0)
    # Threshold
    green_threshold = numpy.mean(green_columns) #+ numpy.std(green_columns)
    red_threshold = numpy.mean(red_columns) #+ numpy.std(red_columns)
    blue_threshold = numpy.mean(blue_columns) #+ numpy.std(blue_columns)
    # Green
    x = 0
    self.green_objects = []
    while (x < WIDTH-1):
      if (green_columns[x] > green_threshold):
        start = x
        while (green_columns[x] > green_threshold) and (x < WIDTH-1):
          x += 1
        end = x
        size = (end - start)
        offset = (start + (end - start)/2) - CENTER
        self.green_objects.append((size,offset))
      else:
        x += 1
    # Red
    x = 0
    self.red_objects = []
    while (x < WIDTH-1):
      if (red_columns[x] > red_threshold):
        start = x
        while (red_columns[x] > red_threshold) and (x < WIDTH-1):
          x += 1
        end = x
        size = (end - start)
        offset = (start + (end - start)/2) - CENTER
        self.red_objects.append((size,offset))
      else:
        x += 1
    # Blue
    x = 0
    self.blue_objects = []
    while (x < WIDTH-1):
      if (blue_columns[x] > blue_threshold):
        start = x
        while (blue_columns[x] > blue_threshold) and (x < WIDTH-1):
          x += 1
        end = x
        size = (end - start)
        offset = (start + (end - start)/2) - CENTER # from center
        self.blue_objects.append((size,offset))
      else:
        x += 1

  ## Connect to Server.
  def connect(self):
    if (self.connected_out == False) and (self.connected_in == False):
      try:
        print('Establishing SEND Port to Server...')
        self.socket_out = socket.socket(AF_INET, SOCK_STREAM)
        self.socket_out.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.socket_out.bind((ADDRESS_OUT))
        self.socket_out.listen(QUEUE_MAX)
        (self.connection,self.address) = self.socket_out.accept()
        self.connected_out = True
        print('...Success.')
      except socket.error as SocketError:
        print('...Failure')
        self.connected_out = False
        self.socket_out.close()
        pass
      try:
        print('Establishing RECEIVE Port from Server...')
        self.socket_in = socket.socket(AF_INET,SOCK_STREAM) # tmp socket
        self.socket_in.connect((ADDRESS_IN))
        self.connected_in = True
        print('...Success.')
      except socket.error as error:
        self.socket_in.close()
        self.connected_in = False
        print('...Failure.')
        pass
    else:
      print('ALREADY CONNECTED')
  
  ## Receives COMMAND from Server.
  def receive_command(self):
    try: 
      print('Receiving COMMAND from Server...')
      json_command = self.socket_in.recv(BUFFER_SIZE)
      dict_command = json.loads(json_command)
      self.command = dict_command['COMMAND']
      print(str(json_command))
      print('...Success.')
    except socket.error as SocketError:
      print('...Connection Failure.')

  ## After receiving COMMAND, determine action.
  def decide_action(self):
    if (self.command == 'START') or (self.command == 'CONTINUE'):
      if (self.gathered < 4):
        self.gather()
      elif (not self.stacked):
        self.stack()
      elif (not self.returned):
        self.return_home()
      else:
        self.action = 'WAIT'
    elif (self.command == 'STOP'):
      self.action = 'WAIT'
    elif (self.command == 'PAUSE'):
      self.action = 'WAIT'
    elif (self.command == 'DISCONNECT'):
      self.action = 'WAIT'
      self.disconnect()
    else:
      self.action = 'UNKNOWN'
    self.previous_actions.append(self.action)

  ## Gather Logic
  def gather(self):
    print('GATHERING')
    self.use_camera()
    (size,offset) = max(self.green_objects)
    if (offset > THRESHOLD) and (size > MINIMUM_SIZE):
      if (offset > 3*THRESHOLD):
        self.action = 'RIGHT3'
        self.orientation += 3*TURN #!
      elif (offset > 2*THRESHOLD):
        self.action = 'RIGHT2'
        self.orientation += 2*TURN #!
      elif (offset < 1*THRESHOLD):
        self.action = 'RIGHT1'
        self.orientation += 1*TURN #!
    elif (offset < -THRESHOLD) and (size > MINIMUM_SIZE):
      if (offset < -3*THRESHOLD):
        self.action = 'LEFT3'
        self.orientation -= 3*TURN #!
      elif (offset < -2*THRESHOLD):
        self.action = 'LEFT2'
        self.orientation -= 2*TURN #!
      elif (offset < -1*THRESHOLD):
        self.action = 'LEFT1'
        self.orientation -= 1*TURN #!
    elif (size > MINIMUM_SIZE):
      if (size > RANGE):
        self.action = 'GRAB'
        self.gathered += 1 #!
      elif (not self.error_number == ERROR_CLOSE):
        self.action = 'FORWARD'
        self.x += TRAVEL*cos(self.orientation) #!
        self.y += TRAVEL*sin(self.orientation) #!
      else:
        self.action = 'RIGHT3'
        self.orientation += TURN #!
    else:
      self.action = 'RIGHT3'
      self.orientation += 3*TURN #!

  ## Stack Logic
  def stack(self):
    print('STACKING')
    if (not (int(self.x) == ZONE_X and int(self.y) == ZONE_Y)):
      if (self.orientation > tan((ZONE_Y - self.y)/(ZONE_X - self.x)) + ERROR): #!
        self.action = 'LEFT1'
        self.orientation -= TURN
      elif (self.orientation < tan((ZONE_Y - self.y)/(ZONE_X - self.x)) - ERROR): #!
        self.action = 'RIGHT1'
        self.orientation += TURN
      else:
        self.action = 'FORWARD'
        self.x += TRAVEL*cos(self.orientation) #!
        self.y += TRAVEL*sin(self.orientation) #!
    else:  
      self.action = 'STACK'
      self.stacked = True #!

  def return_home(self):
    print('RETURNING')
    self.action = 'RETURN'
    self.returned = True #!

  ## Execute action with arduino.
  def control_arduino(self):
    ### Send
    try:
      print('Sending ACTION to Controller...')
      print(self.action)
      self.arduino.write(self.action)
      print('...Success.')
    except Exception:
      print('...Failure.')
    ### Receive
    try:
      print('Receiving ERROR from Controller...')
      self.error_number = int(self.arduino.readline())
      print('...Success.')
    except ValueError:
      self.error_number = ERROR_PARSE
      print("ValueError: Failed to parse signal, retrying...")
    except OSError:
      self.error_number = ERROR_CONNECTION
      print("OSError: Connection lost, retrying...")
    except SerialException:
      self.error_number = ERROR_CONNECTION
      print("Serial Exception: Connection lost, retrying...")
    except SyntaxError:
      self.error_number = ERROR_PARSE
      print("Syntax Error: Failed to parse signal, retrying...")
    except KeyError:
      self.error_number = ERROR_PARSE
      print("KeyError: Failed to parse signal, retrying...")

  ## Handle Errors from Arduino
  def handle_error(self):
    if (self.error_number == ERROR_NONE):
      self.error = 'NONE'
    elif (self.error_number == ERROR_CLOSE):
      self.error = 'TOO CLOSE'
    elif (self.error_number == ERROR_FAR):
      self.error = 'TOO FAR'
    elif (self.error_number == ERROR_LOAD):
      self.error = 'LOAD FAILED'
      self.gathered -= 1
    elif (self.error_number == ERROR_PARSE):
      self.error = 'PARSE FAILED'
    elif (self.error_number == ERROR_ACTION):
      self.error = 'BAD ACTION'
    else:
      self.error = 'UNKNOWN ERROR'

  ## Send RESPONSE to Server
  def send_response(self):   
    try:
      print('Sending RESPONSE to Server ...')
      json_response = json.dumps({'ACTION':self.action, 'ERROR':self.error, 'GATHERED':str(self.gathered)})
      self.connection.send(json_response)
      print(str(json_response))
      print("...Success.")
    except Exception:
      print("...Failure.")

  ## Disconnect from Server.
  def disconnect(self):
    try:
      print('Disconnecting from Server...')
      self.socket_in.close()
      self.socket_out.close()
      self.connection.close()
      self.connected_in = False
      self.connected_out = False
      print('...Success')
    except Exception:
      print('...Failure')
      pass

# Main
if __name__ == "__main__":
  green = Worker()
  while 1:
    green.connect()
    while (green.connected_in) or (green.connected_out):
      green.receive_command()
      green.decide_action()
      green.control_arduino()
      green.handle_error()
      if (green.connected_in == False) or (green.connected_out == False):
        break
      else:
        green.send_response()
    else:
      green.disconnect()
