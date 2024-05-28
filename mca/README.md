In our application, we require the program to run at startup, and continously. This is implemented using systemd services and a shell script.

Compile the program using 
$ make mca

Place the mca.service file in /etc/systemd/system/services

Enable the service using
$ systemctl enable mca

