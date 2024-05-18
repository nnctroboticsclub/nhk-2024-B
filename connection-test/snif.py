import serial

ser = serial.Serial("/dev/ttyUSB0", baudrate=9600)
print("[+] Connected to: " + ser.portstr)

print("[*] Setting Reg18 ([group] address check)")

ser.write(b"@REG18\r\n")
flags = int(ser.readline()[0:2].decode(), 16)
print(f"REG18  =  {flags:02x}")

flags = flags & 0xFC
print(f"REG18 <-- {flags:02x}")
ser.write(f"@REG18:{flags:03}\r\n".encode())
res = ser.readline()
print(f"--> {res}")

print("REG00 <-- 200")
ser.write("@REG00:200\r\n".encode())
res = ser.readline()
print(f"--> {res}")

print("[*] Resetting the device")
ser.write(b"@RST\r\n")
res = ser.readline()
print(f"--> {res}")

print("[*] Broadcasting the test command")
ser.write(b"@TBN240008" + b"\x55\xaa\xcc\x00\x01@\x34\xcb" + b"\r\n")
print(f"--> {ser.readline()}")
print(f"--> {ser.readline()}")

while True:
    command = ser.read(3)
    if command != b"RBN":
        remain = ser.readline()
        print(command, remain)
        continue

    from_addr = int(ser.read(3))  # FEP を信じる
    length = int(ser.read(3))  # FEP を信じる
    data = ser.read(length)
    ser.read(2)  # \r\n

    if data[0:3] == b"\x55\xaa\xcc":  # REP
        key = data[3]
        length = data[4]
        checksum = data[-2:]
        data = data[5:-2]

        print(
            f"[+] {from_addr:3d}: [REP] {data.hex()} (key={key:#2x}, checksum={checksum.hex()})"
        )
    else:
        print(f"[+] {from_addr:3d}: {data}")
