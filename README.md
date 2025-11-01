# Keyboard_Driver

So install the custom keyboard driver you will need to open up the command prompt and run it in administrator. Once it is open you will need to type in the file path to where the files for the driver are eg. sys and inf. To do the following you will need to get devcon installed on the tester machine. You can download it from the windows driver kit.
After that type in the following command devcon install [the file name of the inf file] \root\ + [driver name]. Eg. devcon install KDMFDriver.inf \root\KDMFDriver. Next go to the system32 folder the new driver sys file should be there.
