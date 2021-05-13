/*******************************************************************************
 * Copyright (c) 2015 Thomas Telkamp and Matthijs Kooijman
 *
 * Permission is hereby granted, free of charge, to anyone
 * obtaining a copy of this document and accompanying files,
 * to do whatever they want with them without any restriction,
 * including, but not limited to, copying, modification and redistribution.
 * NO WARRANTY OF ANY KIND IS PROVIDED.
 *
 * This example sends a valid LoRaWAN packet with payload "Hello,
 * world!", using frequency and encryption settings matching those of
 * the The Things Network.
 *
 * This uses OTAA (Over-the-air activation), where where a DevEUI and
 * application key is configured, which are used in an over-the-air
 * activation procedure where a DevAddr and session keys are
 * assigned/generated for use with all further communication.
 *
 * Note: LoRaWAN per sub-band duty-cycle limitation is enforced (1% in
 * g1, 0.1% in g2), but not the TTN fair usage policy (which is probably
 * violated by this sketch when left running for longer)!

 * To use this sketch, first register your application and device with
 * the things network, to set or generate an AppEUI, DevEUI and AppKey.
 * Multiple devices can use the same AppEUI, but each device has its own
 * DevEUI and AppKey.
 *
 * Do not forget to define the radio type correctly in config.h.
 *
 *******************************************************************************/

#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

#include <lmic.h>
#include <hal/hal.h>

#include "rpi-lora-demonstrator.h"

int port = 1;
byte recv_buffer[] = { 0x00, 0x00, 0x00, 0x00 };

union l2ba{
   long lvalue;
   uint8_t bvalue[4];
};

union f2ba{
   float fvalue;
   uint8_t bvalue[4];
};

long get_uptime(void) {
    FILE * uptimefile;
    char uptime_chr[28];
    if((uptimefile = fopen("/proc/uptime", "r")) == NULL)
      perror("supt"), exit(EXIT_FAILURE);
    fgets(uptime_chr, 12, uptimefile);
    fclose(uptimefile);
    return strtol(uptime_chr, NULL, 10);
}

unsigned long get_temp(void) {
    FILE * tempfile;
    char temp_chr[28];
    long temp = 0;
    if((tempfile = fopen("/sys/class/thermal/thermal_zone0/temp", "r")) == NULL)
      perror("supt"), exit(EXIT_FAILURE);
    fgets(temp_chr, 12, tempfile);
    fclose(tempfile);
    return strtol(temp_chr, NULL, 10);
}

void do_send(osjob_t* j) {
    char strTime[16];
    l2ba uptime, temp;
    f2ba load1, load5, load15;
    double load[3];

    getSystemTime(strTime , sizeof(strTime));
    printf("%s: ", strTime);

    // Check if there is not a current TX/RX job running
    if (LMIC.opmode & OP_TXRXPEND) {
        printf("OP_TXRXPEND, not sending\n");
    } else {
        digitalWrite(RF_LED_PIN, HIGH);
        if (port == 1) {
          temp.lvalue = get_temp();
          uint8_t mydata[] = { temp.bvalue[0], temp.bvalue[1], temp.bvalue[2], temp.bvalue[3], recv_buffer[0], recv_buffer[1], recv_buffer[2], recv_buffer[3] };
          LMIC_setTxData2(port, mydata, sizeof(mydata), 0);
          port = 2;
        }
        else {
          uptime.lvalue = get_uptime();
          if (getloadavg(load, 3) != -1) {
            load1.fvalue = load[0];
            load5.fvalue = load[1];
            load15.fvalue = load[2];
            printf("load average : %f , %f , %f\n", load[0],load[1],load[2]);
          }
          else {
            load1.fvalue = 999.99;
            load5.fvalue = 9999.99;
            load15.fvalue = 9999.99;
          }
          uint8_t mydata[] = { uptime.bvalue[0], uptime.bvalue[1], uptime.bvalue[2], uptime.bvalue[3], load1.bvalue[0], load1.bvalue[1], load1.bvalue[2], load1.bvalue[3], load5.bvalue[0], load5.bvalue[1], load5.bvalue[2], load5.bvalue[3], load15.bvalue[0], load15.bvalue[1], load15.bvalue[2], load15.bvalue[3], };
          LMIC_setTxData2(port, mydata, sizeof(mydata), 0);
          port = 1;
        }
        printf("Packet queued\n");
    }
    // Next TX is scheduled after TX_COMPLETE event.
}

void onEvent (ev_t ev) {
    char strTime[16];
    getSystemTime(strTime , sizeof(strTime));
    printf("%s: ", strTime);

    switch(ev) {
        case EV_SCAN_TIMEOUT:
            printf("EV_SCAN_TIMEOUT\n");
        break;
        case EV_BEACON_FOUND:
            printf("EV_BEACON_FOUND\n");
        break;
        case EV_BEACON_MISSED:
            printf("EV_BEACON_MISSED\n");
        break;
        case EV_BEACON_TRACKED:
            printf("EV_BEACON_TRACKED\n");
        break;
        case EV_JOINING:
            printf("EV_JOINING\n");
        break;
        case EV_JOINED:
            printf("EV_JOINED\n");
            digitalWrite(RF_LED_PIN, LOW);
            // Disable link check validation (automatically enabled
            // during join, but not supported by TTN at this time).
            LMIC_setLinkCheckMode(0);
        break;
        case EV_RFU1:
            printf("EV_RFU1\n");
        break;
        case EV_JOIN_FAILED:
            printf("EV_JOIN_FAILED\n");
        break;
        case EV_REJOIN_FAILED:
            printf("EV_REJOIN_FAILED\n");
        break;
        case EV_TXCOMPLETE:
            printf("EV_TXCOMPLETE (includes waiting for RX windows)\n");
            if (LMIC.txrxFlags & TXRX_ACK)
              printf("%s Received ack\n", strTime);
            if (LMIC.dataLen) {
              printf("############################################### %s Received %d bytes of payload\n", strTime, LMIC.dataLen);
//              byte payload[LMIC.dataLen];
              for (int i = 0; i < LMIC.dataLen; i++) {
                  if ( i <= sizeof(recv_buffer) ) {
                      recv_buffer[i] = LMIC.frame[i];
                  }
              }
            }
            digitalWrite(RF_LED_PIN, LOW);
            // Schedule next transmission
            os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
        break;
        case EV_LOST_TSYNC:
            printf("EV_LOST_TSYNC\n");
        break;
        case EV_RESET:
            printf("EV_RESET\n");
        break;
        case EV_RXCOMPLETE:
            // data received in ping slot
            printf("EV_RXCOMPLETE\n");
        break;
        case EV_LINK_DEAD:
            printf("EV_LINK_DEAD\n");
        break;
        case EV_LINK_ALIVE:
            printf("EV_LINK_ALIVE\n");
        break;
        default:
            printf("Unknown event\n");
        break;
    }
}

/* ======================================================================
Function: sig_handler
Purpose : Intercept CTRL-C keyboard to close application
Input   : signal received
Output  : -
Comments: -
====================================================================== */
void sig_handler(int sig)
{
  printf("\nBreak received, exiting!\n");
  force_exit=true;
}

/* ======================================================================
Function: main
Purpose : not sure ;)
Input   : command line parameters
Output  : -
Comments: -
====================================================================== */
int main(void)
{
    // caught CTRL-C to do clean-up
    signal(SIGINT, sig_handler);

    printf("%s Starting\n", __BASEFILE__);

      // Init GPIO bcm
    if (!bcm2835_init()) {
        fprintf( stderr, "bcm2835_init() Failed\n\n" );
        return 1;
    }

	// Show board config
    printConfig(RF_LED_PIN);
    printKeys();

    // Light off on board LED
    pinMode(RF_LED_PIN, OUTPUT);
    digitalWrite(RF_LED_PIN, HIGH);

    // LMIC init
    os_init();
    // Reset the MAC state. Session and pending data transfers will be discarded.
    LMIC_reset();

    // Start job (sending automatically starts OTAA too)
    do_send(&sendjob);

    while(!force_exit) {
      os_runloop_once();

      // We're on a multitasking OS let some time for others
      // Without this one CPU is 99% and with this one just 3%
      // On a Raspberry PI 3
      usleep(1000);
    }

    // We're here because we need to exit, do it clean

    // Light off on board LED
    digitalWrite(RF_LED_PIN, LOW);

    // module CS line High
    digitalWrite(lmic_pins.nss, HIGH);
    printf( "\n%s, done my job!\n", __BASEFILE__ );
    bcm2835_close();
    return 0;
}
