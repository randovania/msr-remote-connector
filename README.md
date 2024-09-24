# MSR-Remote-Connector

This is a simple remote connector for tracking items and playing multiworld in Metroid: Samus Returns randomizer.  
Intended to use with Randovania.

## Build

This project needs to be loaded with Magikoopa https://github.com/RicBent/Magikoopa  
You have to provide the code binary and exheader binary from your dumped original game. Please also note that the output is a modified new code and exheader binary.  

## Known Issues

At the current state, it doesn't work with Luma3DS. The biggest issue is not being able to initialize the sockets.  
Also older Citra versions had a bug in the services required for this to work.  
Ultimately, only "officially" supported version is Citra Canary 2798.
