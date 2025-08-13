"""
Base example:
```python
    mypiezo = piezointerface()
    mypiezo.send(np.array([1.0e-6,1.0e-6,1.0e-6,1.0e-6]))
```
"""


import numpy as np
import serial
import threading
import time
gain = 4000.0/9.5 # ADU / microns
gains = gain*np.ones(4)
port_params = {"port":"/dev/ttyACM0", "baudrate":57600,
                     "bytesize":serial.EIGHTBITS, "parity":"N",
                     "stopbits":1}
class piezointerface(object):
    def __init__(self, n=4, gains=gains, offsets=None,
                 port_params=port_params,):
        print("Opening the interface")
        try :
            self.ser = serial.Serial(**port_params)
        except :
            self.ser = None
            print("Could not open the serial port")
        self.value_min = 0
        self.value_max = 9.5
        self.n = n
        self.values = np.zeros(self.n)
        if offsets is None:
            self.offsets = np.zeros(n)
        else:
            self.offsets = offsets
        self.gains = gains
        self.raw_values = self.values2raw(self.values)

        self.listening = True
        listen_thread = threading.Thread(target=self.listen)
        listen_thread.start()
    
    def listen(self):
        self.ser.timeout = 0.1
        while(self.listening):
            answer = self.ser.read_until()
            if(len(answer) > 0):
                print(answer.decode("utf-8"))

    def __del__(self):
        print("Reseting server")
        self.reset_server()
        print("Closing the interface")
        self.listening = False
        time.sleep(.01)
        self.ser.close()
        print("done")
        
    def get_raw_values(self):
        return self.raw_values
        
    def values2raw(self, values):
        raw = ((values + self.offsets) * self.gains).astype(int)
        return self.sanitize_raws(raw)
        
    def raw2values(self, raws):
        values = raws/self.gains - self.offsets
        return self.sanitize_values(values)

    def send_current(self,):
        bytearray = self.vals2bytes("s", self.raw_values)
        self.ser.write(bytearray)
        pass

    def reset_server(self,):
        self.ser.write(b"[z,0]")
    
    def set_verbose_mode(self, val: bool):
        if val:
            self.ser.write(b"[v,1]")
        else:
            self.ser.write(b"[v,0]")

    def _send(self):
        myrawvalues = self.raw_values
        self.ser.write(self.vals2bytes("s",myrawvalues))

    def send(self, any_values=None):
        if isinstance(any_values, np.ndarray):
            if any_values.dtype == int:
                self.raw_values = any_values
                self.values = self.raw2values(self.raw_values)
            elif (any_values.dtype == float) or (any_values.dtype == np.float64):
                thevalues = any_values
                self.values = thevalues
                self.raw_values = self.values2raw(self.values)
        self._send()
        
    def sanitize_values(self, values):
        return np.clip(values, self.value_min, self.value_max)
        
    def sanitize_raws(self, raws):
        newraws = np.clip(raws, 0, 4000)
        return newraws
        
    def vals2bytes(self, mystring, myarray):
        newstring = "[" + mystring
        for i in np.arange(self.n):
            newstring = newstring + ","
            newstring = newstring + f"{myarray[i]}"
        newstring = newstring + "]" + "\n"
        bytearray = newstring.encode("utf-8")
        return bytearray
