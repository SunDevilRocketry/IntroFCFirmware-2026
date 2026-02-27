#terminalserial.py
import serial

class SerialTerminal:
    def __init__(self, comport, baudrate,numbytes):
        self.__comport = comport
        self.__baudrate = baudrate
        serialinfo = None
        read_test = serial.SerialException
        self.__numbytes = numbytes
        if read_test == "":
            print(serial.tools.list_ports.comports(include_links=False))
        else:
            print(serial.SerialException)
            
            #direct pyserial call avail COM ports, gather all available in a list and returns
            #create an if statement to check for connection issues before function call

        def read(num_bytes):
            #pyserial read
            return serialinfo.read(num_bytes)
        
        def write(write_byte):
            #pyserial write
            serialinfo.write(write_byte)
        
        def connect(comport,baudrate):
            #open com ports
            open(comport,baudrate)