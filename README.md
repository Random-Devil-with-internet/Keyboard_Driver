# Keyboard_Driver

So install the custom keyboard driver you will need to open up the command prompt and run it in administrator. Once it is open you will need to type in the file path to where the files for the driver are eg. sys and inf. To do the following you will need to get devcon installed on the tester machine. You can download it from the windows driver kit.
After that type in the following command devcon install [the file name of the inf file] \root\ + [driver name]. Eg. devcon install KDMFDriver.inf \root\KDMFDriver. Next go to the system32 folder the new driver sys file should be there.

Now to check if it works you will need to uninstall the in the deceive manger and you can ues winDBG to debug the driver though the use of this function DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Hello\n"). And then sends through the com port in the kernel driver debugger. Now the way I’m doing this is by using a virtual machine which you can set up a com port to connect with the debugger. All you need to do is to name the pipe name and choose the port number.


In the debugger side of things you will need to again copy the pipe name into there.


Once you have done that you can launch your virtual machine and the debugger. And you should see your message appear. Now I have
