from serial.tools.list_ports import comports
from serial import Serial

def main():
    device = [port.device for port in comports()][0]
    IO = Serial(device, 9600)

    while True:
        while IO.inWaiting() == 0:
            pass

        value = IO.readline()
        print(value)

if __name__ == "__main__":
    main()
