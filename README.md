# midi-usb-controller

Custom PCB and firmware to implement a MIDI controller board 
- based on Atmel/Microchip ATSAMD21J18A-AU microcontroller
- hardware/gerber contains files necessary to have PCB's manufactured
- USB MIDI device class means no custom drivers are required for WIN or Linux
- bus-powered requires only a single USB micro cable

NOTES
-----
- Controls used were 10K linear potentiometers
- the controls are sampled periodically (~5ms)
- when control changes are detected, they are entered into a FIFO queue
- USB is handled via interrupts and the start of frame (SOF) interrupt checks the queue and transmits 
any events found there
- based on ATMEL STUDIO CDC project with only a single change to the core code to expose one function 
(udd_ep_job_t* udd_ep_get_job(udd_ep_id_t ep))
- several serial ports and crystals are supported by the board to make it more flexible although they are not necessary for 
normal USB operation
- the 100nF capacitors in the low pass filter of each control input were not used/necessary
- src/main.c contains the initialization and controller sampling.  It also defines which control is treated as a pitchbend wheel
- src/midi dir contains the actual device implementation and USB config descriptors
- src/config/  contains some important definitions in the clock and usb files (including the midi device descriptor strings)
