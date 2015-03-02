#-------------------------------------------------------------------------------
# Name:        модуль1
# Purpose:
#
# Author:      Pavel
#
# Created:     11.11.2014
# Copyright:   (c) Pavel 2014
# Licence:     <your licence>
#-------------------------------------------------------------------------------
import serial
import datetime
serial_port = 'COM8'
serial_speed = 38400

def init_serial():
    return serial.Serial(serial_port,serial_speed)

def save_to_file():
    ser=init_serial()
    #f_out = open('temperature_'+datetime.datetime.now().strftime("%y_%m_%d_%H_%M_%S")+'.csv', 'w')
    f_out_name='temperature.csv'
#    f_out = open(f_out_name, 'a')
    #ser=serial.Serial(serial_port,serial_speed)
    print("Wait data")
    while 1:
        try:
            s = ser.readline().decode('ASCII').strip()
        except KeyboardInterrupt:
            break
        except:
            print ("No data")
        if s=='START':
            print(s)
            f_out = open(f_out_name, 'a')
            continue
        if s=='STOP':
            print(s)
            f_out.close()
            continue
        try:
            valuse=s.split(':')
            f_out.write(valuse[1].strip()+';'+valuse[3].strip()+';'+valuse[5].strip()+';'+valuse[7].strip()+'\n')
            print(valuse[5].strip()+"   "+str(int(valuse[3].strip())//60000)+":"+str(int(valuse[3].strip())//1000%60))
        except:
            print ("Can't decode")
    f_out.close()
    ser.close()

def main():
    save_to_file()

if __name__ == '__main__':
    main()
