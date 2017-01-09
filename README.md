###Installing:

from sources:
```sh
git clone https://github.com/Tiiffi/mcrcon.git
cd mcrcon
make
sudo make install
```
Check **INSTALL** for more details.

You can also download precompiled binaries*: https://github.com/Tiiffi/mcrcon/releases/latest

<sub>*At the moment binaries are provided for Linux and Windows.</sub>

---

###Usage:
mcrcon [OPTIONS]... [COMMANDS]...

Sends rcon commands to Minecraft server.

```
Option:
  -h            Print usage
  -H            Server address
  -P            Port (default is 25575)
  -p            Rcon password
  -t            Interactive terminal mode
  -s            Silent mode (do not print received packets)
  -c            Disable colors
  -r            Output raw packets (debugging and custom handling)
  -v            Output version information
```
Commands with arguments must be enclosed in quotes.

Server address, port and password can be set using following environment variables:
```
MCRCON_HOST
MCRCON_PORT
MCRCON_PASS
```
Note that command-line options will override environment variables.

Example:
  ```mcrcon -H my.minecraft.server -p password "say Server is restarting!" save-all stop```

---

###Enable rcon on server
Remember to enable rcon by adding following lines to ```server.properties``` file.
```
enable-rcon=true
rcon.port=25575
rcon.password=your_rcon_pasword
```

---

####Contact:

* WWW:            https://github.com/Tiiffi/mcrcon/
* MAIL:           tiiffi_at_gmail_dot_com
* IRC:            tiiffi @ quakenet
* BUG REPORTS:    https://github.com/Tiiffi/mcrcon/issues/

---

<sub>Master:</sub> ![Master build](https://api.travis-ci.org/Tiiffi/mcrcon.svg?branch=master)
<sub>Develop:</sub> ![Develop build](https://api.travis-ci.org/Tiiffi/mcrcon.svg?branch=develop)
