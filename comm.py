import sys
import time
import serial
import argparse

connected = False


def snd(ser, data):
    ser.write(bytes([10]))
    ser.write(bytes(data))
    ser.write(bytes([10]))


def sndbytes(ser, data):
    ser.write(data)
    ser.write(bytes([10]))


def tstanswer(answer, right):
    print("Answer: ", answer[:-1])
    print("Excpected Answer: ", bytes(right))
    for i in range(len(answer) - 1):
        if not answer[i] == right[i]:
            return False
        if len(right) - 1 == i and answer[i] == right[i]:
            return True


def testConnection(ser):

    global connected
    snd(ser, [100, 100])

    answer = ser.readline()
    while(not tstanswer(answer, [100, 100])):
        answer = ser.readline()

    print('Connected sucessfully')
    connected = True
    return


def startAP(ser, ssid, pw, ch):

    snd(ser, [100, 110])
    sndbytes(ser, ssid.encode())
    sndbytes(ser, pw.encode())
    sndbytes(ser, bytes([ch+32]))
    answer = ser.readline()
    while(not tstanswer(answer, [110, 110])):
        answer = ser.readline()
    print("AP started")
    return


def connectAP(ser, ssid, pw):

    snd(ser, [100, 111])
    sndbytes(ser, ssid.encode())
    sndbytes(ser, pw.encode())
    #answer = ser.readline()
    #while(not tstanswer(answer, [110, 110])):
    #    answer = ser.readline()
    return


def startMesm(ser, pingpong, count, devicecount):
    snd(ser, [100, 130])
    if(pingpong):
        sndbytes(ser, bytes([200]))
    else:
        sndbytes(ser, bytes([201]))

    sndbytes(ser, bytes([count//10]))
    sndbytes(ser, bytes([devicecount]))
    answer = ser.readline()
    while(not tstanswer(answer, [130, 130])):
        answer = ser.readline()

    while(not tstanswer(answer, [130, 140])):
        answer = ser.readline()
    return


def startSniffing(ser):
    snd(ser, [200, 200, 100])
    while(True):
        answer = ser.readline()
        print(answer)

    return


def restart(ser):
    ser.write(bytes([100, 255, 100]))


def str2bool(v):
    if v.lower() in ('yes', 'true', 't', 'y', '1'):
        return True
    elif v.lower() in ('no', 'false', 'f', 'n', '0'):
        return False
    else:
        raise argparse.ArgumentTypeError('Boolean value expected.')


def main():

    parser = argparse.ArgumentParser(description='csi measurements')

    parser.add_argument('port', nargs='?', default='/dev/ttyUSB0', type=str)
    parser.add_argument('-b', '--baudrate', nargs='?', const=115200,
                        default=115200, type=int, help='change the baudrate of the Connection')
    parser.add_argument('-t', '--timeout', nargs='?', const=100,
                        default=100, type=int, help='timeout for readline')
    parser.add_argument('-p', '--pingpong', nargs='?', const=True, default=True,
                        type=str2bool, help='Decides if you want to have pingpong or just pong')
    parser.add_argument('-c', '--count', nargs='?', const=10,
                        default=10, type=int, help='amount of pingpongs/pongs')
    parser.add_argument('-dc', '--devicecount', nargs='?',
                        const=1, default=1, type=int, help='amount of devices')
    parser.add_argument('-ssid', '--ssid', nargs='?', const='esp32',
                        default='esp32', type=str, help='SSID of the created Hotspot')
    parser.add_argument('-pw', '--password', nargs='?', const='testingesp',
                        default='testingesp', type=str, help='Password of the created Hotspot')
    parser.add_argument('-ch', '--channel', nargs='?', const='1',
                        default='1', type=int, help='Channel for the Hotspot')
    parser.add_argument('-m', '--master', nargs='?', const=True,
                        default=True, type=str2bool, help='Controller or not')
    parser.add_argument('-deb', '--debug', nargs='?', const=False,
                        default=False, type=str2bool, help='Controller or not')

    # parser.add_argument('infile', nargs='?', type=argparse.FileType('r'),default=sys.stdin)
    # parser.add_argument('outfile', nargs='?', type=argparse.FileType('w'),default=sys.stdout)

    args = parser.parse_args()
    print(args)
    ser = serial.Serial(port=args.port, baudrate=args.baudrate, bytesize=serial.EIGHTBITS,
                        parity=serial.PARITY_NONE, stopbits=serial.STOPBITS_ONE, timeout=args.timeout)
    if args.master:
        if not args.debug:
            testConnection(ser)

            time.sleep(5)

            startAP(ser, args.ssid, args.password, args.channel)

            time.sleep(5)

            startMesm(ser, args.pingpong, args.count, args.devicecount)

            while True:
                answer = ser.readline()
                print(answer)

            restart(ser)
        else:
            testConnection(ser)

            time.sleep(5)

            startAP(ser, args.ssid, args.password, args.channel)

            time.sleep(5)

            startSniffing(ser)

    else:
        testConnection(ser)
        connectAP(ser, args.ssid, args.password)
        while True:
                answer = ser.readline()
                print(answer)



if __name__ == "__main__":
    main()
