/*
 * Copyright 2015 Cisco Systems, Inc.
 * Author: Nobody <nbd@sysrepo.org>
 */
/*
 * Generates simulated data for /proc/net/dev
 * This provides an API to ConfD ifstatus.c
 * for obtaining a /proc/net/dev file for
 * 1 to MAX_DEVICES interface.
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "demo.h"
/*
 * Defines
 */
#define __SYSREPO_TEST__ 0
#define MULTICAST_RATIO 5
#define MULTICAST_DELTA ((random()>>28) * MULTICAST_RATIO)
/*
 * Globals
 */

 /* devs accumulates each sampling of simulated counters */
static unsigned long long devs[MAX_DEVICES][N_COUNTERS];

 /* To do: the following should be #defines and not variables */
int delta_time = 18; //scales random() down to less than 2^32 maximum
int avg_packet_size = 7; //divides byte count into packets of size 2^7

/*
 * Macros
 */

#define max(a,b) \
  ({ typeof (a) _a = (a); \
     typeof (b) _b = (b); \
      _a > _b ? _a : _b; })

/*
 * Procedures
 */
int initialize_devs_from_dev(char *filename, 
			     int devices, 
			     unsigned long long devs[devices][N_COUNTERS])
{
  FILE *dev = NULL;
  int i = 0;
  char *ifname, *p, buf[BUFSIZ];

    dev = fopen(filename, "r");
    i=-1;
      if (dev != NULL)
	{
	  while (fgets(buf, sizeof(buf), dev) != NULL) 
	    {
	      unsigned long long counter;
              if ((p = strchr(buf, ':')) == NULL)
	        continue;
	      *p = ' ';
	      if ((ifname = strtok(buf, " \t")) == NULL)
	        continue;
	      if (++i >= devices)
		{
		  break;
		}
	      sscanf(strtok(NULL, " \t"), "%llu", &devs[i][RX_BYTES]);
	      sscanf(strtok(NULL, " \t"), "%llu", &devs[i][RX_PACKETS]);
	      /* Get /proc/net/dev  RX Errors */
	      sscanf(strtok(NULL, " \t"), "%llu", &devs[i][RX_ERRS]);

	      /* Next, get RX Dropped and Missed Errors */ 
  	      sscanf(strtok(NULL, " \t"), "%llu", &devs[i][RX_DROPPED]);
	      devs[i][RX_ERRS] += devs[i][RX_DROPPED];

	      /* RX FIFO Erros */
	      sscanf(strtok(NULL, " \t"), "%llu", &counter);
	      devs[i][RX_ERRS] += counter;

	      /* RX Length, Overflow, CRC and Frame Errs */
	      sscanf(strtok(NULL, " \t"), "%llu", &counter);
	      devs[i][RX_ERRS] += counter;

	      sscanf(strtok(NULL, " \t"), "%llu", &devs[i][RX_COMPRESSED]);
	      sscanf(strtok(NULL, " \t"), "%llu", &devs[i][MULTICAST]);
	      sscanf(strtok(NULL, " \t"), "%llu", &devs[i][TX_BYTES]);
	      sscanf(strtok(NULL, " \t"), "%llu", &devs[i][TX_PACKETS]);

	      /* /proc/net/dev TX Errors */
	      sscanf(strtok(NULL, " \t"), "%llu", &devs[i][TX_ERRS]);

	      /* TX Dropped Errors */
	      sscanf(strtok(NULL, " \t"), "%llu", &devs[i][TX_DROPPED]);
	      devs[i][TX_ERRS] += devs[i][TX_DROPPED];

	      /* TX FIFO Errors added into TX_ERRS */
	      sscanf(strtok(NULL, " \t"), "%llu", &counter);
	      devs[i][TX_ERRS] += counter;

	      /* Collisions */
	      sscanf(strtok(NULL, " \t"), "%llu", &counter);
	      devs[i][TX_ERRS] += counter;

	      /* TX Carrier Errors from /proc/net/dev */
	      sscanf(strtok(NULL, " \t"), "%llu", &counter);
	      devs[i][TX_ERRS] += counter;
	      sscanf(strtok(NULL, " \t"), "%llu", &devs[i][TX_COMPRESSED]);
	    }
	  fclose(dev);
	}
      return i;
}
int fprint_devs(FILE* dev, 
	       char *devfmt, 
		int devices)
{
  int i;
  char dev_name[6];
  fputs("\n\nInter-|   Receive                            "
       "                    |  Transmit\n"
       " face |bytes    packets errs drop fifo frame "
       "compressed multicast|bytes    packets errs "
	"drop fifo colls carrier compressed\n", dev);

  for (i=0; i<devices; i++){
        sprintf(dev_name, devfmt, i);
	fprintf(dev, "%6s:%8llu %7llu %4llu %4llu %4llu %5llu %10llu %9llu "
		   "%8llu %7llu %4llu %4llu %4llu %5llu %7llu %10llu\n",
	       dev_name,
	       devs[i][RX_BYTES],           // ndev.stats.rx_bytes,
	       devs[i][RX_PACKETS],         // ndev.stats.rx_packets,
	       devs[i][RX_ERRS],            // ndev.stats.rx_errors,
	       devs[i][RX_DROPPED]          // ndev.stats.rx_dropped 
                 + devs[i][RX_MISSED_ERRS], // ndev.stats.rx_missed_errors,
	       devs[i][RX_FIFO_ERRS],       // ndev.stats.rx_fifo_errors,
	       devs[i][RX_LENGTH_ERRS]      // ndev.stats.rx_length_errors 
	       + devs[i][RX_OVER_ERRS]      // ndev.stats.rx_over_errors 
	       + devs[i][RX_CRC_ERRS]       // ndev.stats.rx_crc_errors 
               + devs[i][RX_FRAME_ERRS],    // ndev.stats.rx_frame_errors,
	       devs[i][RX_COMPRESSED],      // ndev.stats.rx_compressed,
	       devs[i][MULTICAST],          // ndev.stats.multicast,
	       devs[i][TX_BYTES],           // ndev.stats.tx_bytes,
	       devs[i][TX_PACKETS],         // ndev.stats.tx_packets,
	       devs[i][TX_ERRS],            // ndev.stats.tx_errors,
	       devs[i][TX_DROPPED],         // ndev.stats.tx_dropped,
	       devs[i][TX_FIFO_ERRS],       // ndev.stats.tx_fifo_errors,
	       devs[i][COLLISIONS],         // ndev.stats.collisions,
               devs[i][TX_CARRIER_ERRS]     // ndev.stats.tx_carrier_errors 
               + devs[i][TX_ABORTED_ERRS]   // ndev.stats.tx_aborted_errors 
	       + devs[i][TX_WINDOW_ERRS]    // ndev.stats.tx_window_errors  
	       + devs[i][TX_HEARTBEAT_ERRS],// ndev.stats.tx_heartbeat_errors
	       devs[i][TX_COMPRESSED]);     // ndev.stats.tx_compressed)
  }
  return 0;
}
int dump_devs(uint devices, char *tag)
{
  int i;
  if ((devices == 0) || (devices > MAX_DEVICES-1))
    {
      return 1;
    }
  printf("\n%s\n",tag);
  for (i=0; i < devices; i++)
    {
     char dev_name[6];
     sprintf(dev_name, "eth%i", i);
     printf("%6s:%8llu %7llu %4llu %4llu %4llu %5llu %10llu %9llu "
	    "%8llu %7llu %4llu %4llu %4llu %5llu %7llu %10llu\n",
	    dev_name,
	    devs[i][RX_BYTES],           // ndev.stats.rx_bytes,         
	    devs[i][RX_PACKETS],         // ndev.stats.rx_packets,       
	    devs[i][RX_ERRS],            // ndev.stats.rx_errors,        
	    devs[i][RX_DROPPED]          // ndev.stats.rx_dropped        
	    + devs[i][RX_MISSED_ERRS],   // ndev.stats.rx_missed_errors, 
	    devs[i][RX_FIFO_ERRS],       // ndev.stats.rx_fifo_errors,   
	    devs[i][RX_LENGTH_ERRS]      // ndev.stats.rx_length_errors  
	    + devs[i][RX_OVER_ERRS]      // ndev.stats.rx_over_errors    
	    + devs[i][RX_CRC_ERRS]       // ndev.stats.rx_crc_errors     
	    + devs[i][RX_FRAME_ERRS],    // ndev.stats.rx_frame_errors,  
	    devs[i][RX_COMPRESSED],      // ndev.stats.rx_compressed,    
	    devs[i][MULTICAST],          // ndev.stats.multicast,        
	    devs[i][TX_BYTES],           // ndev.stats.tx_bytes,      
	    devs[i][TX_PACKETS],         // ndev.stats.tx_packets,     
	    devs[i][TX_ERRS],            // ndev.stats.tx_errors,                 
	    devs[i][TX_DROPPED],         // ndev.stats.tx_dropped,         
	    devs[i][TX_FIFO_ERRS],       // ndev.stats.tx_fifo_errors,      
	    devs[i][COLLISIONS],         // ndev.stats.collisions,      
	    devs[i][TX_CARRIER_ERRS]     // ndev.stats.tx_carrier_errors   
	    + devs[i][TX_ABORTED_ERRS]   // ndev.stats.tx_aborted_errors           
	    + devs[i][TX_WINDOW_ERRS]    // ndev.stats.tx_window_errors                          
	    + devs[i][TX_HEARTBEAT_ERRS],// ndev.stats.tx_heartbeat_errors 
	    devs[i][TX_COMPRESSED]);     // ndev.stats.tx_compressed)     
    }
  return 0;
}

/* Each invocation of simulated_devs() accumlates counters      */
int simulate_devs(uint devices)
{
  int i;
  if ((devices == 0) || (devices > MAX_DEVICES-1))
    {
      return 1;
    }
  
  for (i=0; i< devices; i++)
    {
	devs[i][RX_BYTES]          += random()>>delta_time;                     
	devs[i][TX_BYTES]          += random()>>delta_time;                      
	devs[i][RX_PACKETS]        += max(1,devs[i][RX_BYTES]>>avg_packet_size);
	devs[i][TX_PACKETS]        += max(1,devs[i][TX_BYTES]>>avg_packet_size);
	devs[i][RX_ERRS]           += devs[i][RX_PACKETS] * 0.000001;            
	devs[i][TX_ERRS]           += devs[i][TX_PACKETS] * 0.000001;            
	devs[i][RX_DROPPED]        += devs[i][RX_PACKETS] * 0.000001;         
	devs[i][TX_DROPPED]        += devs[i][TX_PACKETS] * 0.000001;         
	devs[i][MULTICAST]         += (devs[i][RX_PACKETS]>>MULTICAST_RATIO) + MULTICAST_DELTA;    
	devs[i][COLLISIONS]        += devs[i][RX_PACKETS] * 0.00001; 
	devs[i][RX_LENGTH_ERRS]    += devs[i][RX_PACKETS] * 0.000001; 
	devs[i][RX_OVER_ERRS]      += devs[i][RX_PACKETS] * 0.000001; 	
	devs[i][RX_CRC_ERRS]       += devs[i][RX_PACKETS] * 10E-12; 	
	devs[i][RX_FRAME_ERRS]     += devs[i][TX_PACKETS] * 10E-10; 	
	devs[i][RX_FIFO_ERRS]      += devs[i][RX_PACKETS] * 10E-8; 	
	devs[i][RX_MISSED_ERRS]    += devs[i][RX_PACKETS] * 10E-8; 

	devs[i][TX_ABORTED_ERRS]   += devs[i][TX_PACKETS] * 10E-9; 	
	devs[i][TX_CARRIER_ERRS]   += devs[i][TX_PACKETS] * 10E-11; 	
	devs[i][TX_FIFO_ERRS]      += devs[i][TX_PACKETS] * 10E-10;
	devs[i][TX_HEARTBEAT_ERRS] += devs[i][TX_PACKETS] * 10E-7;
	devs[i][TX_WINDOW_ERRS]    += devs[i][TX_PACKETS] * 10E-6; 	

	devs[i][RX_COMPRESSED]     += devs[i][RX_PACKETS] * 10E-10; 
	devs[i][TX_COMPRESSED]     += devs[i][RX_PACKETS] * 10E-10;
    }

  return 0;
}
	       
FILE* if_simulate_and_open(char* f, char* m)
{
  /* arguments f and m are not used to simulate the fopen */
    extern const int IFSTATUS_N_DEVICES;
    int n = IFSTATUS_N_DEVICES;
    FILE *dev;
    int i;

    // To do: make into a #define or accept as parameter
    char devfmt[6] = "eth%i";
#if __SYSREPO_TEST__
    char *devfilefmt = "../dev%5i";
    char *xmlfilefmt = "../dev%5i.xml";
    char devbackup[BUFSIZ];
#endif
    if ((n < 1) || (n > MAX_DEVICES))
      {
	fprintf(stderr, "Number of devices must be between 1 and %d devices/n", MAX_DEVICES);
	return NULL;
      }
    i = initialize_devs_from_dev("../dev", n, devs) ;
    if (i != (n-1))
      {
	bzero(devs, sizeof(devs));
      }

#if __SYSREPO_TEST__
    i = random();
    sprintf(devbackup,devfilefmt,i);
    rename("../dev",devbackup);
    sprintf(devbackup, xmlfilefmt, i); 
    rename("../dev.xml", devbackup);
#endif
    dev = fopen("../dev","w");
    if (dev == NULL) 
      {
	fprintf(stderr, "if_simulate_and_open cannot open simultated /proc/net/dev file\n");
	return NULL;
      }
    simulate_devs(n);
    fprint_devs(dev, devfmt, n);
    fclose(dev);
    dev=fopen("../dev","r");
    return dev;
}
