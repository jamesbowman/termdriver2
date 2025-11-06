import sys
import serial

def CSI(s : str):
    return b'\x1b[' + s.encode()

def reset_to_uf2(port):
    ser = serial.Serial(port, 115200, timeout = 1)
    
    # First make sure nothing is pending, and consume any uncollected characters
    ser.write(b" " * 40)
    ser.read(ser.inWaiting())

    # request the TermDriver's unique id, a 6 character hex string
    ser.write(CSI("7n"))
    hexid = ser.read(6)
    if len(hexid) != 6:
        print("Unexpected response b'{hexid}'to id code request")
        sys.exit(1)
    print(f"Got 6 digit hex id: {hexid}")

    # This is the ID in decimal, needed for unlock
    id_decimal = int(hexid, 16)

    # Now put the TermDriver into UF2 bootloader mode
    ser.write(CSI(f"1413829197;{id_decimal};0l"))
    ser.flush()

    print("TermDriver2 should now be in UF2 bootloader mode")

if __name__ == "__main__":
    if len(sys.argv) != 2:
        printf("Usage: python reset_to_uf2.py <port>")
        sys.exit(1)
    reset_to_uf2(sys.argv[1])
