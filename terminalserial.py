#terminalserial.py
import serial

class SerialTerminal:
    def __init__(self, comport, baudrate,numbytes):
        self.__comport = comport
        self.__baudrate = baudrate
        serialinfo = open(comport,baudrate)
        read_test = serial.SerialException
        self.__numbytes = numbytes
        def available_ports():
            if read_test == "":
                print(serial.tools.list_ports.comports(include_links=False))
            else:
                print(serial.SerialException)
            #direct pyserial call avail COM ports, gather all available in a list and returns
            #create an if statement to check for connection issues before function call
            pass
        def read(read_byte):
            #pyserial read
            timeout_exception = serial.SerialTimeoutException
            return serialinfo.read(read_byte)
            if serial.SerialTimeoutException == "b''":
                read()
            pass
        def write(write_byte):
            #pyserial write
            serialinfo.write(write_byte)
            pass