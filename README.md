# Infrasonic_hardware
Infrasonic hardware


## Upload the sketch

When you try to upload a new sketch to your ESP32 and it fails to connect to your board, it means that your ESP32 is not in flashing/uploading mode. Having the right board name and COM por selected, follow these steps:

- Hold-down the “BOOT” button in your ESP32 board
- Press the “Upload” button in the Arduino IDE to upload a new sketch
- After you see the  “Connecting….” message in your Arduino IDE, release the finger from the “BOOT” button

## First Run- SPIFFS Flash Format
If you are using a new ESP32 and planning to use internal flash for storage, then you need to format it once. You can comment it for the second upload onwards.

```
`#define FILESYSTEM SPIFFS
// You only need to format the filesystem once
#define FORMAT_FILESYSTEM false
````

to

```
`#define FILESYSTEM SPIFFS
// You only need to format the filesystem once
#define FORMAT_FILESYSTEM true
````


AND
```

//Unless its inrernal flash, we dont have to 
  //format here. For example SD cards can be
  //formatted using a computer format
  //if (FORMAT_FILESYSTEM) FILESYSTEM.format();
```
to
```
  if (FORMAT_FILESYSTEM) FILESYSTEM.format();
```


## Pin layout
![](ESP32-DOIT-DEVKIT-V1-Board-Pinout-36-GPIOs-updated.jpg)
