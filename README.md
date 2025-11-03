# Keyboard driver
# User guide 
To install the custom keyboard driver you will need to open up the command prompt and run it in administrator. Once it is open you will need to type in the file path to where the files for the driver are eg. sys and inf. To do the following you will need to get devcon installed on the tester machine. You can download it from the windows driver kit.
After that type in the following command devcon install [the file name of the inf file] \root\ + [driver name]. Eg. devcon install KDMFDriver.inf \root\KDMFDriver. Next you should see something like below and then click install this driver anyway.
Next go to the system32 folder the new driver sys file should be there.

![hello](https://github.com/Random-Devil-with-internet/Keyboard_Driver/blob/main/Screenshot%202025-11-03%20001722.png)

Now to check if it works you will need to uninstall the in the deceive manger and you can ues winDBG to debug the driver though the use of this function DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Hello\n"). And then sends through the com port in the kernel driver debugger. Now the way I’m doing this is by using a virtual machine which you can set up a com port to connect with the debugger. All you need to do is to name the pipe name and choose the port number. 

In the debugger side of things you will need to again copy the pipe name into there.

Once you have done that you can launch your virtual machine and the debugger. And you should see your message appear. 
Now this should have worked but no matter what ever I tried I couldn’t get the driver to install. The image above of the terminal actuality shows three failed attempts. 

In the setupapi.dev file which is located in C:\Windows\inf it shows you the installation attempts by devcon and the details about them. It shows you that the cause for the failure of installation was the specified path being invalid or it can’t find the sys driver file despite it being in the same folder as everything else.
Besides the recommended method that was shown in the microsoft article I attempted 3 other methods. The first of which was using the device manager and clicking on the action tab to then find a dropbox containing the words Add legacy hardware. Click on that to then launch Hardware Wizard which should allow you to install a driver.

As you can see above the installation failed two times and apparently because it was not for x64 based systems. Even though the folder for the driver on the image says clearly that it is x64. And the second time it was because the hash was not present in the catalogue file
And you can also see the failed attempts as Unknown device in device manager. After that I found out that you could use the Service Control Manager (SCM) to also do the same thing.

Unfortunately even though it said it was a success I still could not find the driver file in system 32. The final method was utilising Microsoft PnP Utility which is a command-line tool used to manage driver packages.

As it said the failure was due to the hash being not present in the catalog file just as above. So I deleted the catalog file and removed all incidences of it in the KMDFDriver3.inf or the driver’s setup Information file.

After that I tried a second time with the result being another failure. This time being about the inf file not having the digital signature information. So I guess there is no need for a video since the keyboard driver doesn't work.

# Bibliography
cazz, 2024, YOUR FIRST KERNEL DRIVER (FULL GUIDE), Youtube, Available at: https://www.youtube.com/watch?v=n463QJ4cjsU 
Walter Oney, 2003, Programming the Microsoft Windows Driver Model, Microsoft, Available at: https://empyreal96.github.io/nt-info-depot/Windows-DDK/Programming.the.Microsoft.Windows.Driver.Model.2nd.Edition.pdf 
N/A, 2025, Tutorial: Write a Hello World Windows Driver (Kernel-Mode Driver Framework), Microsoft, Available at: https://learn.microsoft.com/en-us/windows-hardware/drivers/gettingstarted/writing-a-very-small-kmdf--driver 







