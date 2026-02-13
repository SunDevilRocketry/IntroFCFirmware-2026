#terminalserial.py

class SerialTerminal:
    def __init__(self, comport, baudrate):
        self.__comport = comport
        self.__baudrate = baudrate
        

        def available_ports():
            #direct pyserial call avail COM ports, gather all available in a list and returns
#create an if statement to check for connection issues before function call
            pass
        def read(read_byte):
            #pyserial call
            pass
        def write(write_byte):
            #pyserial call
            pass