Various scripts and programs used to interface I2C modules with the RedPitaya.

# Be sure to install i2c-tools.
$ apt-get install i2c-tools

Some useful commands:

# Show i2c devices on bus. 
$ i2cdetect -y -r 0 

# Read HOURS register from RTC ds3231. 
$ i2cget -y 0 0x68 0x02


