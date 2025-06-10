"""
Base example:
```python
    mypiezo = piezointerface()
    mypiezo.send(np.array([1.0e-6,1.0e-6,1.0e-6,1.0e-6]))
```
"""


import numpy as np
import serial
gain = 4000.0/9.5e-6
gains = gain*np.ones(4)
port_params = {"port":"/dev/ttyACM0", "baudrate":9600,
                     "bytesize":serial.EIGHTBITS, "parity":"N",
                     "stopbits":1}
class piezointerface(object):
    def __init__(self, n=4, gains=gains, offsets=None,
                 port_params=port_params,):
        print("Opening the interface")
        self.ser = serial.Serial(**port_params)
        self.value_min = -9.5/2
        self.value_max = 9.5/2
        self.n = n
        self.values = np.zeros(self.n)
        if offsets is None:
            self.offsets = np.zeros(n)
        else:
            self.offsets = offsets
        self.gains = gains
        self.raw_values = self.values2raw(self.values)

    def __del__(self):
        print("Reseting server")
        self.reset_server()
        print("Closing the interface")
        self.res.close()
        print("done")
        
    def get_raw_values(self):
        return self.raw_values
        
    def values2raw(self,values):
        raw = ((values + self.offsets) * self.gains).astype(int)
        return self.sanitize_raws(raw)
        
    def raw2values(self,raws):
        values = raws/self.gains - self.offsets
        return self.sanitize_values(values)

    def send_current(self,):
        bytearray = self.vals2bytes("s", self.raw_values)
        self.ser.write(bytearray)
        pass

    def reset_server(self,):
        self.ser.write(b"[z,0]")
    
    def _send(self):
        myrawvalues = self.raw_values
        self.ser.write(self.vals2bytes("s",myrawvalues))

    def send(self, any_values=None):
        if isinstance(any_values, np.ndarray):
            if any_values.dtype == int:
                self.raw_values = any_values
                self.values = self.raw2values(self.raw_values)
            elif any_values.dtype == float:
                thevalues = any_values
                self.values = thevalues
                self.raw_values = self.raw2values(self.values)
        self._send()
        
    def sanitize_values(self, values):
        return np.clip(values, self.value_min, self.value_max)
        
    def sanitize_raws(self, raws):
        newraws = np.clip(raws, 0, 4000)
        return newraws
        
    def vals2bytes(self, mystring, myarray):
        newstring = mystring
        for i in np.arange(self.n):
            newstring = newstring + ","
            newstring = newstring + f"{myarray[i]}"
        newstring = newstring + "\n"
        bytearray = newstring.encode("utf-8")
        return bytearray
