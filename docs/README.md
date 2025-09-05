
## Manual for the Beijing module

**Document version:** ${/var/document-version} - ${/var/creation-time}
[HISTORY](./history.md)


![Accra](/images/beijing3.png)

Beijing is a general I/O module that connects to a CAN4VSCP bus and can handle general I/O on ten channels that individually can be set as input or output. The module can be attached to a standard DIN Rail or be mounted directly on a wall (ordered as separate versions).

The module fully adopts to the CAN4VSCP specification and can be powered directly over the bus with a 9-28V DC power source. It has a rich register set for configuration and any information events defined. It also have a decision matrix for easy dynamic event handling.

VSCP CAN modules are designed to work on a VSCP4CAN bus which use ordinary RJ-45 connectors and is powered with 9-28V DC over the same cable. This means there is no need for a separate power cable. All that is needed is a CAT5 or better twisted pair cable. Buss length can be a maximum of 500 meters with drops of maximum 24 meters length (up to a total of 120 meters). As for all VSCP4CAN modules the communication speed is fixed at 125 kbps.

All VSCP modules contains information of there own setup, manual, hardware version, manufacturer etc. You just ask the module for the information you need and you will get it.

When they are started up they have a default functionality that often is all that is needed to get a working setup. If the module have something to report it will send you an event and if it is setup to react on a certain type of event it will do it's work when you send event(s) to it. 

* [Repository for the module](https://github.com/grodansparadis/can4vscp_bejing)
  * This manual is available [here](https://grodansparadis.github.io/can4vscp_beijing/)
  * Latest schema for the module is available [here](https://github.com/grodansparadis/can4vscp_beijing/tree/master/eagle)
  * Latest firmware for the module is available [here](https://github.com/grodansparadis/can4vscp_beijing/tree/master/firmware)
  * [MDF for the module](https://github.com/grodansparadis/can4vscp_beijing/tree/master/mdf)


## VSCP

![VSCP logo](./images/logo_100.png)

VSCP is a free and open automation protocol for IoT and m2m devices. Visit [the VSCP site](https://www.vscp.org) for more information.

**VSCP is free.** Placed in the **public domain**. Free to use. Free to change. Free to do whatever you want to do with it. VSCP is not owned by anyone. VSCP will stay free and gratis forever.

The specification for the VSCP protocol is [here](https://docs.vscp.org) 

VSCP documentation for various parts can be found [here](https://docs.vscp.org/).

If you use VSCP please consider contributing resources or time to the project ([https://vscp.org/support.php](https://vscp.org/support.php)).

## Buy a module

<img src="./images/grodan_logo.png" alt="Grodans PAradis AB" width="200"/>

Ready made modules can be bought from [Grodans Paradis AB](https://www.grodansparadis.com).

## Document license

This document is licensed under [Creative Commons BY 4.0](https://creativecommons.org/licenses/by/4.0/) and can be freely copied, redistributed, remixed, transformed, built upon as long as you give credits to the author.



[filename](./bottom-copyright.md ':include')
