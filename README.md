####Compiling:

```gcc -std=gnu11 -pedantic -Wall -Wextra -O2 -s -o mcrcon mcrcon.c```

On windows, remember to link with winsockets.
Add ```-lws2_32``` to compiler command line on Mingw GCC.

---

More info [here](http://forums.bukkit.org/threads/admin-rcon-mcrcon-remote-connection-client-for-minecraft-servers.70910/).

####Usage:
Usage: mcrcon [OPTIONS]... [COMMANDS]...
Sends rcon commands to minecraft server.

```
Option:
  -h		Prints usage.
  -s		Silent mode. Do not print data received from rcon.
  -t		Terminal mode. Acts as interactive terminal.
  -p		Rcon password. Default: "".
  -H		Host address or ip.
  -P		Port. Default: 25575.
  -c		Do not print colors. Disables bukkit color printing.
  -r		Print everything in raw mode.
		    Good for debugging and custom handling of the output.
```
Invidual commands must be separated with spaces.

Example:
  ```mcrcon -c -H 192.168.1.42 -P 9999 -p password cmd1 "cmd2 with spaces"```

####Enable rcon
Remember to enable rcon by changing/adding these lines to server.properties
```
enable-rcon=true
rcon.password=your_rcon_pasword
rcon.port=9999
```
