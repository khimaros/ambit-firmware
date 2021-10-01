# I2C

Components are attached to the Base via an I2C bus.

## Master => Slave

### 75

Identify yourself!

```
[M] >> 75
[S] << 20 20 34 6C 73 01 03
```

Response always starts with `20 20` and has 7 bits.

Bits 3-5 are the identity string: `4ls`

Bit 6 is the "kind": `01` (button) `02` (dial) `03` (slider)

Bit 7 is number of ports (`03` or `05`)

### 6C 00

Set RGB values for LED.

`06 DC 9D` are the R, G, and B values respectively.

Slave responds with `69`

```
[M] >> 6C 00 06 DC 9D
[S] << 69
```

### 61

Assign bus address to child?

Sent when slave returns `01` to a `64` query.

```
[M] 04 >> 64 00
[S] 04 << 01
[M] 04 >> 61 05
[M] 05 >> 75
[S] 05 << 20 20 ...
```

### 64

Query port status.

Ports are identified in clockwise direction from male.

Slave responds with number of attached components?

Otherwise, no response.

Uses the number of ports returned from `75` query.

Queries numbers 00 - 03 for normal. 00 - 04 for wide.

```
[M] >> 64 00
[S] << 00
```

if component connected to port 00

```
[M] >> 64 00
[S] << 01
```

Otherwise, no response.

If `01` is returned, master responds with `61` assignment
which seems to denote the next available slave address.

Master then `75` queries the assigned address.

### 73

Report current values.

Responds with current values (8 bits) and a status bit

```
[W] 04 >> 73
[R] 04 << 00 01 00 F3 00 00 00 00 01
[R] 04 << 00 00 00 F3 00 00 00 00 00
[R] 04 << 00 01 00 F2 00 00 00 00 01
```

Response always starts with C3.

### 74

Indicates number of components connected
or maybe something like update frequency?

```
[M] >> 74 3A
[S] << 69
...
[M] >> 74 36
[S] << 69
```

