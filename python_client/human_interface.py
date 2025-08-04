import numpy as np
import matplotlib.pyplot as plt
from time import sleep, time
from tqdm import tqdm
from copy import copy


import sys
sys.path.append("/home/labo/src/NOTTControl/")
sys.path.append("/home/labo/src/NOTTControl/script/lib/")

from nott_control import shutter_close, shutter_open


dburl = "redis://nott-server.ster.kuleuven.be:6379"

x_filter = np.array([0.000003332231310433949, 0.000003359090941083894, 0.0000033818181178322115, 0.0000033962809146615277, 0.000003416942128401158, 0.000003431404925230474, 0.0000034438015830896854, 0.0000034561982409488967, 0.000003474793315718422, 0.0000034975206684281573, 0.0000035285124010568945, 0.0000035574379947155264, 0.000003604958663143684, 0.000003640082497751213, 0.000003664875989431053, 0.0000037082643799190013, 0.000003741322251517843, 0.000003784710642005791, 0.000003823966930514947, 0.0000038528925241735795, 0.000003919008091409845, 0.000003943801583089685, 0.000003966528759838003, 0.000003978925593658632, 0.000003989256112547738, 0.0000040037189093770535, 0.00000401404942826616, 0.000004032644503035686, 0.000004047107299865002, 0.0000040760328935236335, 0.0000042206610377782125])
y_filter = np.array([0.0017543387818981376, 0.004561388410207326, 0.014385942578987246, 0.036842100544856524, 0.2277192639592904, 0.45789470381399533, 0.6599999760939395, 0.7863157750042266, 0.8508771992735247, 0.8705263076110844, 0.9028070197457336, 0.89017544583122, 0.8873684260854863, 0.9056140394914672, 0.9014035098728668, 0.8410526451047448, 0.8410526451047448, 0.8621052782564592, 0.916842103533114, 0.9210526331517145, 0.8957894703813994, 0.8957894703813994, 0.9028070197457336, 0.8803508916624401, 0.7975438539871611, 0.4761403471025518, 0.2726315201258773, 0.07473679240582154, 0.028420951659928514, 0.005964793694059473, 0.0017543387818981376])
mean_wl = np.sum(x_filter*y_filter) / np.sum(y_filter)

from opcua import OPCUAConnection
from nott_control import Shutter
from configparser import ConfigParser

config = ConfigParser()
config.read("/home/labo/src/NOTTControl/config.ini")
opcuad = config["DEFAULT"]["opcuaaddress"]


def shutter_close(shutter_id):
    """ Function to close a shutter """

    # initialize the OPC UA connection
    config = ConfigParser()
    config.read('/home/labo/src/NOTTControl/config.ini')
    url =  config['DEFAULT']['opcuaaddress']

    opcua_conn = OPCUAConnection(url)
    opcua_conn.connect()
    shutter = Shutter(opcua_conn, f"ns=4;s=MAIN.nott_ics.Shutters.NSH{shutter_id}Shutter {shutter_id}")
    shutter.close()

    # Disconnect
    opcua_conn.disconnect()
    return 'done'

def shutter_open(shutter_id):
    """ Function to open a shutter """

    # initialize the OPC UA connection
    config = ConfigParser()
    config.read('/home/labo/src/NOTTControl/config.ini')
    url =  config['DEFAULT']['opcuaaddress']

    opcua_conn = OPCUAConnection(url)
    opcua_conn.connect()
    shutter = Shutter(opcua_conn, f"ns=4;s=MAIN.nott_ics.Shutters.NSH{shutter_id}Shutter {shutter_id}")
    shutter = Shutter(opcua_conn, f"ns=4;s=MAIN.nott_ics.Shutters.NSH{shutter_id}Shutter {shutter_id}")
    shutter.open()
    
    # Disconnect
    opcua_conn.disconnect()
    return 'done'

class HumInt(object):
    def __init__(self, lam_mean=mean_wl,
                pad=0.15, interf=None,
                act_index=0,
                rois_interest=np.arange(1,10),
                verbose=False,
                db_server=None,
                opcuad=opcuad,
                nb_beams=4,
                non_motorized=0,
                offset = 8.0):
        # self.lamb_min = lam_range[0]
        # self.lamb_max = lam_range[-1]
        self.lam_mean = lam_mean
        self.pad = pad
        self.interf = interf
        self.act_index = act_index
        self.non_motorized = non_motorized # Index of the non-mororized beam
        self.nb_beams = nb_beams
        self.offset = offset * np.ones(self.nb_beams)
        self.offset[self.non_motorized] = 0
        self.verbose = verbose
        self.ts = db_server
        self.rois = [f"roi{n}_sum" for n in rois_interest]
        self.dark = None
        self.bg_noise = None
        self.opcua_conn = OPCUAConnection(opcuad)
        self.opcua_conn.connect()
        self.shutters = [
            Shutter(self.opcua_conn,
                f"ns=4;s=MAIN.nott_ics.Shutters.NSH{shutterid+1}",
                f"Shutter {shutterid+1}")\
                    for shutterid in range(4)
        ]

        self.move(np.array([0., 0., 0., 0.]))

    def __del__(self):
        self.opcua_conn.disconnect()

    def find_dark(self, frac=0.25, dt=0.5, gain=0.1,
                 roi_index=3, verbose=True,
                 amp=800.0):
        current_pos = self.get_position()
        pos_a = current_pos[self.act_index] + 1e6 * frac*self.lam_mean
        pos_b = current_pos[self.act_index] - 1e6 * frac*self.lam_mean
        if verbose:
            print(f"Trying {pos_a:.3f} and {pos_b:.3f}")
        a = self.move_and_sample(pos_a, dt=dt, move_back=False)
        b = self.move_and_sample(pos_b, dt=dt, move_back=False)
        # self.move(current_pos[self.act_index])
        a_val = a[:,roi_index].mean(axis=0)
        a_std = a[:,roi_index].std(axis=0)
        b_val = b[:,roi_index].mean(axis=0)
        b_std = b[:,roi_index].std(axis=0)
        raw_offset = a_val - b_val
        p = self.deltaval2p(raw_offset, frac, amp=amp)
        # offset = (raw_offset) * gain
        offset = (p ) * gain
        newpos = current_pos[self.act_index] + offset
        if verbose:
            print(f"a = {a_val:.1f} pm{a_std:.1f},   b = {b_val:.1f} pm{b_std:.1f}")
            print(f"Raw offset : {raw_offset:.3f},  p = {p:.3f}")
            print(f"Moving to {newpos:.3f}")
            import matplotlib.pyplot as plt
            # plt.figure(dpi=70)
            # plt.plot(a[:,roi_index], label="a")
            # plt.plot(b[:,roi_index], label="b")
            # plt.legend(fontsize=6)
            # plt.show()
        self.move(newpos)

    def deltaval2p(self, deltaval, frac, amp=800.):
        lam_micron = 1.0e6 * self.lam_mean
        deltap = lam_micron * frac
        inner = -deltaval / (amp * 2 * np.sin(2*np.pi/lam_micron * deltap))
        p = lam_micron /(2*np.pi) * np.arcsin(inner)
        return p

    def move_and_sample(self, position, dt=None, move_back=True):
        orig_pos = self.get_position()
        self.move(position)
        sleep(self.pad)
        if dt is None:
            res = self.sample_cal()
        else:
            res = self.sample_long_cal(dt)
        if move_back:
            print(f"moving_back to {orig_pos}")
            self.move(orig_pos)
            sleep(self.pad)
        return res

    def get_dark(self, dt):
        print("Taking darks")
        measurement = self.sample_long(dt=dt)
        self.dark = measurement.mean(axis=0)
        self.bg_noise = measurement.std(axis=0)/np.sqrt(measurement.shape[0])
        print("You can remove the shutters")

    def sample(self):
        mes = np.array([self.ts.ts.get(akey) for akey in self.rois])
        return mes.T[1]

    def sample_cal(self):
        return self.sample() - self.dark

    def db_time(self):
        aresp = self.ts.ts.get(self.rois[0])
        return aresp[0]

    def sample_long(self, dt=1.0):
        # start = int(np.round(time()*1000).astype(int))
        start = self.db_time()
        sleep(dt)
        # end = int(np.round(time()*1000).astype(int))
        end = self.db_time()
        mes = np.array([self.ts.ts.range(akey, start, end) for akey in self.rois])
        return mes.T[1]

    def sample_long_cal(self, dt):
        return self.sample_long(dt=dt) - self.dark

    def four2three(self, position):
        return position - position[self.non_motorized]

    def move(self, position ):
        # print(f"moving to {position:.3e}")
        values = self.four2three(position) + self.offset
        self.interf.send(any_values=values)

    def get_position(self):
        pos = self.interf.values.copy()
        pos -= self.offset
        return pos

    def do_scan(self, beam_index, start=-3.0, end=3.0, nsteps=1000, dt=0.1):
        step_vals = np.linspace(start, end, nsteps)
        starting_pos = self.get_position()
        steps = starting_pos[None,:] * np.ones_like(step_vals)[:,None]
        steps[:,beam_index] = step_vals
        mask = np.zeros(self.nb_beams)
        mask[beam_index] = 1
        step_full = steps[:,None] * mask[None,:]
        print("Starting a scan")
        results = []
        for n, astep in enumerate(tqdm(steps)):
            ares = self.move_and_sample(astep, move_back=False, dt=dt)
            results.append(np.mean(ares, axis=0))
        results = np.array(results)
        self.move(starting_pos)
        print("Scan ended")
        return steps, results

    def relative_move(self, motion):
        thepos = self.get_position()
        thepos[self.act_index] += motion 
        self.interf.send(any_values=thepos)

    def evaluate_lag(self, act_index, n=10, lag_min=0.05, lag_max=0.15, amplitude=0.5, roi_index=3):
        start_pos = self.get_position()
        lags = np.linspace(lag_min,lag_max, n)
        signal_amplitudes = []
        signal_stds = []
        for i, alag in enumerate(lags):
            measurements = []
            for i in range(8):
                newpos = copy(start_pos)
                newpos[act_index] += amplitude
                self.move(newpos)
                sleep(alag)
                val1 = self.sample()[roi_index]
                self.move(start_pos)
                sleep(alag)
                val2 = self.sample()[roi_index]
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

    def chip_calib(self, amp, steps=10, dt=0.5,
                    dn_object=None):
        import dnull as dn
        import jax.numpy as jp
        test_conditions = {
            "co2_ppm": 1e6,
            "temp": 25.0,
            "rhum": 0.3,
            "pres": 1e3,
            "co2" : 450,
        }
        ntel = 4
        shutter_probe, piston_probe = dn.dnull.full_hadamard_probe(ntel, amp, steps=steps)
        # shutter_probe = dn.dnull.shutter_probe(ntel)
        shutter_phasor = jp.ones_like(self.lambs)[None,:,None] * shutter_probe[:,None,:]
        hadamard_phasor = jp.exp(1j*2*np.pi/self.lambs[None,:,None] * piston_probe[:,None,:])
        probe_series = jp.concatenate((shutter_phasor, hadamard_phasor), axis=0)
        test_conditions["probe_series"] = probe_series

        if dt is None:
            test_sample = self.sample_long_cal(1.0)
            rms = np.std(test_sample, axis=0)

        print("shutter_calibration")
        print("Assuming all start open")
        measurements = []
        stds = []
        beam_state = np.ones(ntel, dtype=bool)
        for aprobe in shutter_probe.astype(bool):
            print("aprobe:", aprobe, "beam_state", beam_state)
            for i, (astate, target_state) in enumerate(zip(beam_state, aprobe)):
                beam_id = i
                # print(astate, target_state)
                if astate != target_state:
                    # print("Changing")
                    if target_state:
                        print(f"Opening {beam_id}")
                        self.shutters[beam_id].open()
                        beam_state[i] = True
                    else:
                        print(f"Closing {beam_id}")
                        self.shutters[beam_id].close()
                        beam_state[i] = False
            print(beam_state)
            sleep(self.pad)
            a = self.sample_long_cal(dt=dt)
            measurements.append(a.mean(axis=0))
            if dt is not None:
                stds.append(a.std(axis=0)/np.sqrt(a.shape[0]))
            else:
                stds.append(rms)
        for ashutter in self.shutters:
            ashutter.open()
        print("Sleeping to avoid shutter vibrations")
        sleep(2.0)

        initial_position = self.get_position()
        for aprobe in piston_probe:
            # Move_and_sample
            a = self.move_and_sample(aprobe, dt=dt, move_back=False)
            # append
            measurements.append(a.mean(axis=0))
        self.move(initial_position)
        measurements = np.array(measurements)
        stds = np.array(stds)
        return measurements, stds, test_conditions


# TODO: adjust the calibration strategy and long integration strategy for when we miss frames

