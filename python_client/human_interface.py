import numpy as np
import matplotlib.pyplot as plt
from time import sleep, time
from tqdm import tqdm

import sys
sys.path.append("/home/labo/src/NOTTControl/")

dburl = "redis://nott-server.ster.kuleuven.be:6379"

x_filter = np.array([0.000003332231310433949, 0.000003359090941083894, 0.0000033818181178322115, 0.0000033962809146615277, 0.000003416942128401158, 0.000003431404925230474, 0.0000034438015830896854, 0.0000034561982409488967, 0.000003474793315718422, 0.0000034975206684281573, 0.0000035285124010568945, 0.0000035574379947155264, 0.000003604958663143684, 0.000003640082497751213, 0.000003664875989431053, 0.0000037082643799190013, 0.000003741322251517843, 0.000003784710642005791, 0.000003823966930514947, 0.0000038528925241735795, 0.000003919008091409845, 0.000003943801583089685, 0.000003966528759838003, 0.000003978925593658632, 0.000003989256112547738, 0.0000040037189093770535, 0.00000401404942826616, 0.000004032644503035686, 0.000004047107299865002, 0.0000040760328935236335, 0.0000042206610377782125])
y_filter = np.array([0.0017543387818981376, 0.004561388410207326, 0.014385942578987246, 0.036842100544856524, 0.2277192639592904, 0.45789470381399533, 0.6599999760939395, 0.7863157750042266, 0.8508771992735247, 0.8705263076110844, 0.9028070197457336, 0.89017544583122, 0.8873684260854863, 0.9056140394914672, 0.9014035098728668, 0.8410526451047448, 0.8410526451047448, 0.8621052782564592, 0.916842103533114, 0.9210526331517145, 0.8957894703813994, 0.8957894703813994, 0.9028070197457336, 0.8803508916624401, 0.7975438539871611, 0.4761403471025518, 0.2726315201258773, 0.07473679240582154, 0.028420951659928514, 0.005964793694059473, 0.0017543387818981376])
mean_wl = np.sum(x_filter*y_filter) / np.sum(y_filter)

# ts = redisclient.RedisClient(dburl)

class HumInt(object):
    def __init__(self, lam_mean=mean_wl,
                pad=0.15, interf=None,
                act_index=0,
                rois_interest=[3,4],
                verbose=False,
                db_server=None):
        # self.lamb_min = lam_range[0]
        # self.lamb_max = lam_range[-1]
        self.lam_mean = lam_mean
        self.pad = pad
        self.interf = interf
        self.act_index = act_index
        self.verbose = verbose
        self.ts = db_server
        self.rois = [f"roi{n}_sum" for n in rois_interest]
        self.dark = None
    
    def move_and_sample(self, position):
        self.move(position)
        sleep(self.pad)
        res = self.sample()
        return res

    def get_dark(self, dt):
        print("Taking darks")
        measurement = self.sample_long(dt=dt)
        self.dark = measurement.mean(axis=0)
        print("You can remove the shutters")

    def sample(self):
        mes = np.array([self.ts.ts.get(akey) for akey in self.rois])
        return mes

    def sample_cal(self):
        return self.sample() - self.dark

    def sample_long(self, dt=1.0):
        start = np.round(time()*1000)
        sleep(dt)
        end = np.round(time()*1000)
        
        mes = np.array([self.ts.ts.range(akey,start, end) for akey in self.rois])
        return mes.T

    def sample_long_cal(self, dt):
        return self.sample_long(dt=dt) - self.dark

    def move(self, position ):
        print(f"moving to {position:.3e}")
        values = self.interf.values
        values[self.act_index] = position
        self.interf.send(any_values=values)
        
    def get_position(self):
        pos = self.interf.values[self.act_index]
        return pos

    def do_scan(self, start=-3.0, end=3.0, nsteps=1000):
        steps = np.linspace(start, end, nsteps)
        print("Starting a scan")
        results = []
        for n, astep in tqdm(enumerate(steps)):
            ares = self.move_and_sample(astep)
            results.append(ares)
        results = np.array(results)
        print("Scan ended")
        return results
        
    def relative_move(self, motion):
        thepos = self.get_position()
        thepos[self.act_index] += motion 
        self.interf.send(any_vaues=thepos)
        

    def evaluate_lag(self, n=10, lag_min=0.05, lag_max=0.15, amplitude=0.5):
        start_pos = self.get_position()
        lags = np.linspace(lag_min,lag_max, n)
        signal_amplitudes = []
        signal_stds = []
        for i, alag in enumerate(range(lags)):
            measurements = []
            for i in range(3):
                self.move(start_pos + amplitude)
                sleep(alag)
                val1 = self.sample()
                self.move(start_pos)
                val2 = self.sample()
                measurements.append(val1 - val2)
            measurements = np.array(measurements)
            signal_amplitudes.append(measurements.mean())
            signal_stds.append(measurements.std())
        signal_amplitudes = np.array(signal_amplitudes)
        signal_stds = np.array(signal_stds)
        plt.figure()
        plt.plot(lags, signal_amplitudes)
        plt.errorbar(lags, signal_amplitudes, yerr=signal_stds)
        plt.xlabel("Lag [s]")
        plt.ylabel("Amplitude of light variation")
        plt.show()
