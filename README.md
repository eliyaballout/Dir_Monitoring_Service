# ***Welcome to Dir Monitoring Service***



## Introduction

Welcome to Directory Monitoring Service project, this program is written in C/C++. <br>
DirMonService is a Windows service that monitors a specified directory for file system changes and logs events such as file addition, removal, modification, and renaming. The service ensures that critical directories are protected by providing real-time monitoring and logging of changes. <br><br>


**Warning**

The techniques demonstrated by this project are powerful and can be misused if applied maliciously. This tool is provided with the intention of advancing knowledge and should only be used in ethical hacking scenarios where explicit permission has been obtained. Misuse of this software can result in significant harm and legal consequences. By using this software, you agree to do so responsibly, ethically, and within legal boundaries.

<br><br>




## Service Control Manager (SCM):

In order to work with services in Windows OS, to create a service and start, stop or delete it you need to get familiar a little bit with Service Control Manager(SCM). <br>

The Service Control Manager (SCM) is a special system process that manages all services on a Windows machine. It handles the following:

1. Starting and stopping services.
2. Sending control requests to services (pause, resume, stop).
3. Maintaining the status of each service.

The `sc` command-line tool is used to communicate with the SCM to install, start, stop, and delete services.

<br><br>




## Key Components

1. **Windows Service:** Implements a Windows service to run in the background and monitor directory changes.

2. **Directory Monitoring:** Uses `ReadDirectoryChangesW` to monitor the specified directory for changes.

3. **Event Logging:** Logs detected changes with timestamps to a specified log file.

<br><br>




## Features

1. **DirMonService:** it has a cpp file called [DirMonService.cpp](https://github.com/eliyaballout/Dir_Monitoring_Service/blob/main/DirMonService/DirMonService.cpp), main service implementation for monitoring directory changes.

<br><br>




## Requirements, Installation & Usage

**I will explain here the requirements, installation and the usage of this keylogger:** <br>

**Requirements:**
1. Ensure you have a C++ compiler and Windows SDK installed. <br><br>


**Installation:**
1. Download and extract the [ZIP file](https://github.com/eliyaballout/Keylogger/archive/refs/heads/main.zip).<br>
2. Navigate to **x64 --> Debug**, you will find the `DirMonService.exe` executable file, this is the executable that you need to run in order to monitor directory changes. <br><br>


**Usage:**

**Make sure you run all the executables in cmd with administrator privileges (Run as Administrator)** <br>

**Creating or installing the service:**

```
sc create DirMonService binPath= "C:\Path\To\DirMonService.exe"
```
where `"C:\Path\To\DirMonService.exe"` should be the full path of the executable (which is located in **x64 --> Debug**).

<br>

**Starting the service to start monitoring:** <br>
```
sc start DirMonService <directory_To_Monitor> <log_File_Path>
```
where `<directory_To_Monitor>` should be the full path of the directory you want to monitor its changes .<br>
And `<log_File_Path>` should be the full path where you want to save the log file that logs all the changes that is captured by the service.

<br>

**Stopping the service:**
```
sc stop DirMonService
```

<br><br>


**You also can pause, resume and even delete the service:**

**pause:**
```
sc pause DirMonService
```
<br>


**resume:**
```
sc resume DirMonService
```
<br>


**delete:**
**Make sure you have stopped the service before deleting it.**
```
sc delete DirMonService
```
<br>



## Ethical Considerations

This tool is intended for educational use only to demonstrate techniques commonly used by monitoring the file system. It should be used in a controlled environment, such as a penetration testing lab, where explicit permission has been granted. Always practice responsible disclosure and use ethical hacking principles.<br><br>




## Technologies Used
<img src="https://github.com/devicons/devicon/blob/master/icons/c/c-original.svg" title="c" alt="c" width="40" height="40"/>&nbsp;
<img src="https://github.com/devicons/devicon/blob/master/icons/cplusplus/cplusplus-original.svg" title="c++" alt="c++" width="40" height="40"/>&nbsp;
<br><br><br>




## Demonstration of the Directory Monitoring Service



<br>
