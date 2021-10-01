#!/usr/bin/env python3

# This is designed to receive input from sigrok-cli:
# sigrok-cli --continuous --config samplerate=1M --driver=fx2lafw -P i2c:scl=D1:sda=D0 -A i2c=address-read:address-write:data-read:data-write:warnings | ./filter_i2c.py

import sys

stok = 'WAIT'
mode = ''
addr = ''
buff = ''
data = []
oput = {}
iput = {}

# // Write + Read
# Write
# Address write: 02
# ACK
# Data write: 75
# ACK
# Read
# Address read: 02
# ACK
# Data read: 20
# ACK
# Data read: 20
# ACK
# Data read: ..
# NACK

# // Disconnect
# Write
# Address write: 02
# NACK

# // Disconnect
# Read
# Address read: 02
# NACK

for line in sys.stdin:
    line = line.rstrip()[7:]

    #print('LINE', line, '::', stok, mode)

    if line in ('Read', 'Write'):
        if stok == 'DATA' and len(data):
            if mode == 'Read' and oput[addr][-1] != data:
                oput[addr].append(data)
                print('[R]', addr, '<<', ' '.join(data))

            elif mode == 'Write' and iput[addr][-1] != data:
                iput[addr].append(data)
                print('[W]', addr, '>>', ' '.join(data))

        data = []
        mode = line
        stok = 'ADDR'

    elif stok == 'ADDR':
        if mode == 'Write' and line.startswith('Address write: '):
            addr = line[-2:]
            if addr not in iput:
                iput[addr] = [['00']]
            stok = 'ADDR_ACK'

        elif mode == 'Read' and line.startswith('Address read: '):
            addr = line[-2:]
            if addr not in oput:
                oput[addr] = [['00']]
            stok = 'ADDR_ACK'

    elif stok == 'ADDR_ACK':
        if line == 'ACK':
            stok = 'DATA'
        elif mode == 'Write' and line == 'NACK':
            print('[!] Disconnect', addr)
            stok = 'WAIT'

    elif stok == 'DATA':
        if line.startswith('Data read: ') or line.startswith('Data write: '):
            buff = str(line[-2:])
            stok = 'DATA_ACK'

    elif stok == 'DATA_ACK':
        if line in ('ACK', 'NACK'):
            data.append(buff)
            buff = ''
            stok = 'DATA'

    #print('STOK:', stok)

