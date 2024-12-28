import serial
import time

with serial.Serial(
    "/dev/ttyS3",
    baudrate=115200,
    bytesize=serial.EIGHTBITS,
    stopbits=serial.STOPBITS_ONE,
    parity=serial.PARITY_NONE,
    timeout=10,
) as uart3:
    uart3.write(b"Hello World!\n")
    buf = uart3.read(128)
    print("Raw data:\n", buf)
    data_strings = buf.decode("utf-8")
    print("Read {:d} bytes, printed as string:\n {:s}".format(len(buf), data_strings))