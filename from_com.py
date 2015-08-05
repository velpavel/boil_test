#-------------------------------------------------------------------------------
# Author:      PaVel
#-------------------------------------------------------------------------------
import serial
import datetime
import os
serial_port = 'COM8'
serial_speed = 38400

def init_serial():
    return serial.Serial(serial_port,serial_speed)

def open_file(file_name):
    # Если файл есть и открывается, то используем его. 
    # При ошибке проверки существования или открытия сменим имя.
    # Создадим файл и шапку.
    try:
        if os.path.exists(file_name):
            f_out=open(file_name, 'a')
            return f_out
    except:
        file_name='temperature_'+datetime.datetime.now().strftime("%y_%m_%d_%H_%M_%S")+'.csv'
    f_out = open(file_name, 'w')
    f_out.write('date;RID;MS;Temperature;boil\n')
    return f_out
    
def save_to_file(f_out_name='temperature.csv'):
    try:
        ser=init_serial()
    except:
        print("Serial port error")
        input("Press Enter")
        return
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
            f_out = open_file(f_out_name)
            f_out_name = f_out.name
            continue
        if s=='STOP':
            print(s)
            f_out.close()
            continue
        try:
            valuse=s.split(':')
            f_out.write(datetime.datetime.now().strftime("%y%m%d %H:%M:%S")+';'+valuse[1].strip()+';'+valuse[3].strip()+';'+valuse[5].strip()+';'+valuse[7].strip()+'\n')
            print(valuse[5].strip()+"   "+str(int(valuse[3].strip())//60000)+":"+str(int(valuse[3].strip())//1000%60))
        except:
            print ("Can't decode")
    f_out.close()
    ser.close()

def main():
    save_to_file()

if __name__ == '__main__':
    main()
