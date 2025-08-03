[modbus doc](https://www.modbustools.com/modbus.html#function16)

## filter
```log
ftdi-ft && usb.dst == 2.5.2
```

## Function 02(02hex) Read Discrete Inputs
```
A TX payload: 02021001580001020000af78
```

## Function 03 (03hex) Read Holding Registers
```
A TX payload: 0203000e0001e5fa
A TX payload: 02030019000155fe
A TX payload: 0203000f0002f43b
A TX payload: 0203001d0001143f
A TX payload: 0203001e0001e43f
A TX payload: 0203001a0001a5fe
A TX payload: 0203000e0001e5fa
A TX payload: 02030019000155fe
A TX payload: 0203001d0001143f
A TX payload: 0203001e0001e43f
A TX payload: 0203001a0001a5fe
A TX payload: 0203000e0001e5fa
A TX payload: 02030019000155fe
A TX payload: 0203000f0002f43b
A TX payload: 0203001d0001143f
A TX payload: 020300110002943d
A TX payload: 0203001e0001e43f
A TX payload: 020300170002743c
A TX payload: 0203001a0001a5fe
A TX payload: 0203001f0001b5ff
A TX payload: 0203000e0001e5fa
A TX payload: 02030019000155fe
```

## Function 16 (10hex) Write Multiple Registers
```
A TX payload: 0210015400020400000000f5b4
```


## payload examples
```
A TX payload: 0203000e0001e5fa
A RX payload: 020302044fbf70
A TX payload: 02030019000155fe
A RX payload: 020302435e4c8c
A TX payload: 0203000f0002f43b
A RX payload: 02030404c4000089fe
A TX payload: 0203001d0001143f
A RX payload: 020302003a7c57
A TX payload: 0203001e0001e43f
A RX payload: 0203020257bcda
A TX payload: 0203001a0001a5fe
A RX payload: 020302001cfd8d
A TX payload: 020300110002943d
A RX payload: 02030402fc000008bb
A TX payload: 020300170002743c
A RX payload: 02030442f900000d7a
```