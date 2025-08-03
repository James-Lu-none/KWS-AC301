[modbus doc](https://www.modbustools.com/modbus.html#function16)
[termios.h](https://www.ibm.com/docs/en/aix/7.3.0?topic=files-termiosh-file)
[input modes](https://www.gnu.org/software/libc/manual/html_node/Input-Modes.html)
[fd](https://blog.csdn.net/yushuaigee/article/details/107883964)
[termios.h cdsn](https://blog.csdn.net/Flag_ing/article/details/125644852)
[termios.h sourcecode](https://github.com/python/cpython/blob/main/Modules/termios.c#L293)
[nlohmann](https://github.com/nlohmann/json)

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




level=error ts=2025-08-03T11:18:11.247756862Z caller=client.go:430 component=client host=10.0.1.100:3100 msg="final error sending batch" status=400 tenant= error="server returned HTTP status 400 Bad Request (400): entry with timestamp 2025-08-01 16:43:38.854444845 +0000 UTC ignored, reason: 'entry too far behind, entry timestamp is: 2025-08-01T16:43:38Z, oldest acceptable timestamp is: 2025-08-03T10:18:03Z',"