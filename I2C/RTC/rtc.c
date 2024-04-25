#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

/* Connect an RTC to SDA/SCL GPIO pins. This Section will retrieve timestamp from RTC
        I used DS3231 for testing.
 */
typedef struct { // Struct to hold full date and time components
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
} RTC_Time;

/* Function to convert BCD to decimal */
int bcdToDec(char b) {
    return (b/16)*10 + (b%16);
}

/* Function to get RTC full date and time */
RTC_Time getRTCTime() {
    
    // I2C device file path
    char *devicePath = "/dev/i2c-0";
    int file;

    // Initialize RTC_Time with default values in case of failure
    RTC_Time time = {0, 0, 0, 0, 0, 0};

    if ((file = open(devicePath, O_RDWR)) < 0) {
        perror("Failed to open the i2c bus");
        return time; // Return default time if bus cannot be opened
    }

    int addr = 0x68; // The I2C address of the RTC
    if (ioctl(file, I2C_SLAVE, addr) < 0) {
        perror("Failed to acquire bus access and/or talk to slave.\n");
        close(file);
        return time; // Return default time if cannot talk to slave
    }

    // DS3231 time and date registers start at 0x00
    char reg = 0x00;
    write(file, &reg, 1);

    // Buffer to store the values read from the RTC, now including date
    char buf[7];
    if (read(file, buf, 7) != 7) {
        perror("Failed to read from the i2c bus.\n");
    } else {
        // Convert the BCD values to normal decimal numbers
        time.second = bcdToDec(buf[0]);
        time.minute = bcdToDec(buf[1]);
        time.hour = bcdToDec(buf[2] & 0x3F); // Adjust for 24-hour clock
        // Assuming day, date, month, year order after time in DS3231
        time.day = bcdToDec(buf[4]);
        time.month = bcdToDec(buf[5]);
        time.year = bcdToDec(buf[6]) + 2000; // Adjust based on century (add 2000 for 21st century)
    }

    close(file);
    return time;
}

int main(int argc, char *argv[]){

        /* Get RTC timestamp */
        RTC_Time currentTime = getRTCTime();

        if (argc == 2 && strcmp(argv[1], "--iso8601") == 0) {
            printf("%04d-%02d-%02dT%02d:%02d:%02d\n",
                currentTime.year,
                currentTime.month,
                currentTime.day,
                currentTime.hour,
                currentTime.minute,
                currentTime.second);
        } else {

        printf("%04d_%02d_%02d_%02d_%02d_%02d\n",
            currentTime.year,
            currentTime.month,
            currentTime.day,
            currentTime.hour,
            currentTime.minute,
            currentTime.second);
        }

        return 0;
}
