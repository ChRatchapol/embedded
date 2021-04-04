#!/bin/python3

from Adafruit_PureIO.smbus import SMBus
from Raspberry_Pi_Master_for_ESP32_I2C_SLAVE.packer import Packer
from Raspberry_Pi_Master_for_ESP32_I2C_SLAVE.unpacker import Unpacker
import time
import json
import requests
import traceback
import os

IDLE = 0
REGISTER_SUCCESS = 1
REGISTER_FAILED = 255
LOGGING = 2
UNLOCK = 3
REGISTER_RFID = 4
VERIFY_FAILED = 5

slave_address = 0x04
backend = "http://localhost:7002"
headers = {'Content-Type': 'application/json'}


def handleResponse(res):
    if res["type"] == IDLE:
        # print("IDLE")
        return
    elif res["type"] == REGISTER_SUCCESS:
        params = {
            "type": REGISTER_SUCCESS,
            "key": res["OTP"],
            "rfid": res["uuid"]
        }
        #print("Register Success with", params)
        requests.post(url=backend+'/signup/validate', headers=headers, data=json.dumps(params))
    elif res["type"] == REGISTER_FAILED:
        params = {
            "type": REGISTER_FAILED
        }
        requests.post(url=backend+'/signup/validate', headers=headers, data=json.dumps(params))
        #print("Register Failed!")
    elif res["type"] == LOGGING:
        params = {
            "from": res["from"],
            "uuid": res["uuid"]
        }
        
        requests.post(url=backend+'/logging', headers=headers, data=json.dumps(params))

        packed = None

        with Packer() as packer:
            for c in json.dumps("{'type':0}").strip('"'):
                packer.write(ord(c))
            packer.end()
            packed = packer.read()

        with SMBus(1) as smbus:
            try:
                smbus.write_bytes(slave_address, bytearray(packed))
            except Exception as e:
                print(e)
                pass
            time.sleep(0.3)

        print(" -> Door opened by", res["uuid"], "using", res["from"], end='')
        #requests.post(url=backend+'/logging', headers=headers,
        #             data=json.dumps(params))

    elif res["type"] == UNLOCK:
        params = {
            "uuid": res['uuid'],
        }
        requests.post(url=backend+'/unlock/validate', headers=headers, data=json.dumps(params))

    return


def slavePolling():
    while True:
        try:
            with open("message.json", "r") as f:
                msg = f.read().strip('\n')
            if msg != '':
                os.system("echo '' > message.json")
        except:
            msg = input()
        if msg == '':
            msg = None
        # print(msg)
        print(f'\nMessage: {msg},', end=' ')
        packed = None
        unpacked = None
        raw_list = None

        with Packer() as packer:
            #print("send:", msg)
            if msg != None:
                for c in json.dumps(msg).strip('"'):
                    packer.write(ord(c))
            packer.end()
            packed = packer.read()

        with SMBus(1) as smbus:
            try:
                smbus.write_bytes(slave_address, bytearray(packed))
            except Exception as e:
                print(e)
                pass
            time.sleep(0.3)
            if msg == None:
                try:
                    raw_list = list(smbus.read_bytes(slave_address, 74))
                except:
                    pass

        try:
            with Unpacker() as unpacker:
                unpacker.write(raw_list)
                unpacked = [chr(c) for c in unpacker.read() if c != 0x00]
            res = ''.join(unpacked)
            # print("i2c response:", res)
            print(f'i2c response: {res}', end='')
            handleResponse(json.loads(res))
        except:
            print("unknown response", end ='')
        try:
            #os.system("echo '' > message.json")
            pass
        except:
            pass
        time.sleep(0.2)


if __name__ == "__main__":
    slavePolling()
