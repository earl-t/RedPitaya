/* Multichannel Analyzer for RedPitaya 125-14. 
    
   Reads in pulse from radiation detector and places into pulse-height spectrum
   Contious acquisition and writes spectrum to file for later evaluation.
   
   written by T Lee, adapted from examples */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include "/RedPitaya/rp-api/api/include/rp.h"
#include "/RedPitaya/rp-api/api-hw-profiles/include/rp_hw-profiles.h"

#define ACQ_TIME 600 // acquisition time in seconds
#define DIRECTORY "/media/usb"
#define TRIGGER_LEVEL 0.0 // volts
#define BUFF_SIZE 250
#define NUM_BINS 1024 // set number of MCA channels
#define MAX_VOLTAGE 10.000 // input voltage range



FILE* initialize_file(const char* hostname, const char* timestamp) {

    char filename[200];
    snprintf(filename, sizeof(filename), "%s/%s/%s_%s.txt", DIRECTORY, hostname, hostname, timestamp);
    FILE *file = fopen(filename, "w");
    fprintf(file, "BOARD: rp-%s\n", hostname);
    fprintf(file, "RANGE: +/-%f V\n", MAX_VOLTAGE);
    fprintf(file, "TRIGGER: %f V\n", TRIGGER_LEVEL);
    fprintf(file, "START TIME (UTC): %s\n", timestamp);
    
    if (file == NULL) {
        perror("Failed to open file");
        exit(EXIT_FAILURE);
    }
    return file;
}

char* get_timestamp() {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    static char timestamp[100]; 
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d_%H-%M-%S", t);

    return timestamp;
}

float find_pulse_height(float *channel_data) {
    float pulse_height = channel_data[0] < 0 ? 0 : channel_data[0];
    int index = 0;
    for(int i = 1; i < BUFF_SIZE; i++) {
        if (channel_data[i] > pulse_height) {
            pulse_height = channel_data[i];
	    index = i;
        }
    }
	
    pulse_height = (channel_data[index-1] + channel_data[index] + channel_data[index+1]) / 3;
    return pulse_height;
}

void update_histogram(float pulse_height, int histogram[][2], int channel_index) {
    int bin = (int)((pulse_height * (NUM_BINS / MAX_VOLTAGE))+0.5);
    if (bin >= 0 && bin < NUM_BINS) {
        histogram[bin][channel_index]++;
    }

}

char* get_rpName() {
    static char result[1024];  // this is huge
    FILE *fp;

    fp = popen("hostname", "r");
    if (fp == NULL) {
        perror("Failed to run command");
        return NULL;
    }

    if (fgets(result, sizeof(result), fp) == NULL) {
        fprintf(stderr, "No output from command.\n");
        pclose(fp);
        return NULL;
    }

    pclose(fp);

    size_t len = strlen(result);
    if (len > 0 && result[len - 1] == '\n') {
        result[len - 1] = '\0';
    }
    const char *prefix = "rp-";
    if (strncmp(result, prefix, strlen(prefix)) == 0) {
        memmove(result, result + strlen(prefix), strlen(result) - strlen(prefix) + 1);
    }

    return result;
}

int main(int argc, char **argv) {

    int histogram[NUM_BINS][2] = {0}; 
    bool stop = false;
    time_t start_t, end_t; // real time program running
    double diff_t;
    clock_t start_dt, end_dt; // dead time aka time program is processing pulses
    double deadtime;

    FILE *file = initialize_file(get_rpName(), get_timestamp());

    if(rp_Init() != RP_OK){
        fprintf(stderr, "Rp api init failed!\n");
        exit(EXIT_FAILURE);
    }

    rp_AcqReset();
    rp_AcqSetDecimation(RP_DEC_4);
    rp_AcqSetTriggerDelay(0);
    rp_AcqSetGain(RP_CH_1, RP_HIGH);
    rp_AcqSetGain(RP_CH_2, RP_HIGH);    
    
    time(&start_t);
    // for testing different acquisition times.
    int acq_time = ACQ_TIME;
    if(argc > 1) {
        acq_time = atoi(argv[1]);
    }
    while(!stop) {

        rp_AcqStart();
        usleep(10);
        rp_AcqSetTriggerLevel(RP_CH_1, TRIGGER_LEVEL);
        rp_AcqSetTriggerSrc(RP_TRIG_SRC_CHA_PE);
        rp_acq_trig_state_t state = RP_TRIG_STATE_TRIGGERED;
        buffers_t *b = rp_createBuffer(2,BUFF_SIZE,false,false,true); 

        while(1) { // this will loop until time is up or a trigger condition
            diff_t = difftime(time(&end_t), start_t);
            if (diff_t >= acq_time) {
                stop = true;
                break;
            }

            rp_AcqGetTriggerState(&state);
            if(state == RP_TRIG_STATE_TRIGGERED) {
                break;
            }
        }

        if (stop) {
            break;
        }
        
        bool fillstate = false;
        while(!fillstate){
            rp_AcqGetBufferFillState(&fillstate);
        }

        rp_AcqStop();
        uint32_t pos = 0;
        rp_AcqGetWritePointerAtTrig(&pos);
        rp_AcqGetData(pos, b);
        start_dt = clock();

        update_histogram(find_pulse_height(b->ch_f[0]), histogram, 0);  // Update first column for channel 0
        update_histogram(find_pulse_height(b->ch_f[1]), histogram, 1);  // Update second column for channel 1

        rp_deleteBuffer(b);
        
        end_dt = clock();
        deadtime = deadtime + ((double) (end_dt - start_dt))/CLOCKS_PER_SEC;
    }

    fprintf(file, "ACQ TIME: %d s\n", acq_time);
    fprintf(file, "DEADTIME: %f\n", deadtime);
    printf("%f\n", deadtime);
    fprintf(file, "<<DATA>>\n");
    for(int i = 0; i < NUM_BINS; i++){
        fprintf(file, "%d\t%d\n", histogram[i][0], histogram[i][1]);
    }
    fprintf(file, "<<END DATA>>");
    fclose(file);
    rp_Release();
    return 0;
}
