# PiLink
Modem emulator for multiple "virtual circuit" serial port access

I created PiLink as a way of giving multiple devices access to the single serial port on the Raspberry Pi.  
It follows the principle of an old-fashioned analog modem on the PSTN - dial a number to create a virtual circuit
between two ports.

The number to dial is the number of the entry for a port in an array within the program, and it responds to a subset
of the standard Hayes AT command set.

I wrote it originally for the chiipKIT™ Pi from Element14 but it should work fine on just about any Arduino
style board since it just uses serial ports.

Commands it understands:

* ATDTx / ATDPx - Dial number "x"
* ATDL - Redial the last numnber used
* ATZ - Reset port to default settings
* ATI / ATI0 - Display version
* ATI1 - List the ports and their names
* ATE / ATE0 - Disable echo
* ATE1 - Enable echo
* ATV / ATV0 - Disable verbose mode
* ATV1 - Enable verbose mode
* ATQ / ATQ0 - Disable quiet mode
* ATQ1 - Enable quiet mode
* ATH - Hang up
* ATA - Answer an incoming call
* (pause)+++(pause) - Escape from "online" to "local" mode

Caller ID (CID) is also supported and includes the name and port number the call originated from.

A typical session might look like this:

    Port 0                          Port 1
    > ATZ
    < OK
    > ATE1V1Q0
    < OK
    > ATDT1
                                    < RING
                                    < NMBR=0
                                    < NAME=Raspberry Pi
                                    < RING
                                    < RING
                                    > ATA
    < CONNECT 115200                < CONNECT 115200
    [the two sessions now chat to each other through the serial link]
                                    > +++
                                    < OK
                                    > ATH
    < NO CARRIER                    < OK
    
For best results on the Raspberry Pi you might want to install the "mgetty" package and use that to manage the
login prompt on the Pi's serial port instead of the default "agetty".  "mgetty" allows both incoming and outgoing
calls using the standard Unix "UUCP" locking mechanism.  This means you can "dial" in to the Raspberry Pi to access
the serial console while the Raspberry Pi is able to (as long as you're not dialling in) connect outwards to
communicate with other devices.

My /etc/mgetty/mgetty.conf contains the following:

    port ttyAMA0
        data-only yes
        init-chat "" \d\d+++\d\dATH "" ATZ OK ATE0V1Q0 OK
        toggle-dtr no
        debug 9
        rings 3

My /etc/inittab has been changed so the ttyAMA line reads:

    T0:23:respawn:/sbin/mgetty -s 115200 ttyAMA0 vt100

For a process to be able to access the serial port for its own communications it must first create a "lock" file
to stop mgetty from taking over the port when it detects data on the line.  This file is called:

    /var/lock/LCK..ttyAMA0
    
The file must contain the process ID of the program using the serial port.  "megtty" will check that process ID and
if it finds that the process isn't running it will delete the lock file.

Not the "double dot" in the filename - it's not a typo.  Be sure to delete the lock file after you have finished with
it.
