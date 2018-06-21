/*
 * Copyright 2017 International Business Machines
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * SNAP HelloWorld Example
 *
 * Demonstration how to get data into the FPGA, process it using a SNAP
 * action and move the data out of the FPGA back to host-DRAM.
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <getopt.h>
#include <malloc.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <assert.h>
#include <action_interfacetest.h>
#include <snap_tools.h>
#include <libsnap.h>
#include <snap_hls_if.h>

int verbose_flag = 0;
static const char *version = GIT_VERSION;

/*static int memcmp_volatile(volatile void* s1, const void* s2,size_t n)
{
    volatile unsigned char *p1 = s1;
    const unsigned char *p2 = s2;
    while(n--)
        if( *p1 != *p2 )
            return *p1 - *p2;
        else
            p1++,p2++;
    return 0;
}*/
/**
 * @brief    prints valid command line options
 *
 * @param prog    current program's name
 */
static void usage(const char *prog)
{
    printf("Usage: %s [-h] [-v, --verbose] [-V, --version]\n"
           "  -C, --card <cardno>       can be (0...3)\n"
           "  -t, --timeout             timeout in sec to wait for done.\n"
           "  -N, --no-irq              disable Interrupts\n"
           "It shows a simple example that HW and SW do some exchanges on the memory data.\n"
           "  HW increases the number by 1\n"
           "  SW doubles the number\n"
           "After several iterations to measure the latency\n"
           "\n"
           "\n",
           prog);
}

// Function that fills the MMIO registers / data structure
// these are all data exchanged between the application and the action
static void snap_prepare_interfacetest(struct snap_job *cjob,
                                       struct interfacetest_job_t *mjob,
                                       void *addr_in,
                                       uint32_t size_in,
                                       uint16_t type_in,
                                       void *addr_out,
                                       uint32_t size_out,
                                       uint16_t type_out)
{
    
    snap_addr_set(&mjob->in, addr_in, size_in, type_in,
                  SNAP_ADDRFLAG_ADDR | SNAP_ADDRFLAG_SRC);
    snap_addr_set(&mjob->out, addr_out, size_out, type_out,
                  SNAP_ADDRFLAG_ADDR | SNAP_ADDRFLAG_DST |
                  SNAP_ADDRFLAG_END);
    
    snap_job_set(cjob, mjob, sizeof(*mjob), NULL, 0);
}

/* main program of the application for the hls_helloworld example        */
/* This application will always be run on CPU and will call either       */
/* a software action (CPU executed) or a hardware action (FPGA executed) */
int main(int argc, char *argv[])
{
    // Init of all the default values used
    int ch, rc = 0;
    int card_no = 0;
    struct snap_card *card = NULL;
    struct snap_action *action = NULL;
    char device[128];
    struct snap_job cjob;
    struct interfacetest_job_t mjob;
    unsigned long timeout = 600;
    struct timeval etime, stime;
    ssize_t size = 1024 * 1024;
    volatile int32_t *vol_ibuff = NULL, *vol_obuff = NULL;
    uint8_t type_in = SNAP_ADDRTYPE_HOST_DRAM;
    uint64_t addr_in = 0x0ull;
    uint8_t type_out = SNAP_ADDRTYPE_HOST_DRAM;
    uint64_t addr_out = 0x0ull;
    int exit_code = EXIT_SUCCESS;
    unsigned int mmio_out = 0;


    // default is interrupt mode enabled (vs polling)
    snap_action_flag_t action_irq = (SNAP_ACTION_DONE_IRQ | SNAP_ATTACH_IRQ);
    
    // collecting the command line arguments
    while (1) {
        int option_index = 0;
        static struct option long_options[] = {
            { "card",        required_argument,  NULL, 'C' },
            { "timeout",     required_argument,  NULL, 't' },
            { "no-irq",      no_argument,        NULL, 'N' },
            { "version",     no_argument,        NULL, 'V' },
            { "verbose",     no_argument,        NULL, 'v' },
            { "help",        no_argument,        NULL, 'h' },
            { 0,             no_argument,        NULL, 0   },
        };
        
        ch = getopt_long(argc, argv,
                         "C:t:NVvh",
                         long_options, &option_index);
        if (ch == -1)
            break;
        
        switch (ch) {
            case 'C':
                card_no = strtol(optarg, (char **)NULL, 0);
                break;
            case 't':
                timeout = strtol(optarg, (char **)NULL, 0);
                break;
            case 'N':
                action_irq = 0;
                break;
                /* service */
            case 'V':
                printf("%s\n", version);
                exit(EXIT_SUCCESS);
            case 'v':
                verbose_flag = 1;
                break;
            case 'h':
                usage(argv[0]);
                exit(EXIT_SUCCESS);
                break;
            default:
                usage(argv[0]);
                exit(EXIT_FAILURE);
        }
    }
    
    if (optind != argc) {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }
    
    volatile int32_t var = 1;
    // Simply set size = 128bytes (32 INT32)
    size = 32*sizeof(int32_t);
    
    /* Allocate in host memory the place to put the text to process */
    vol_ibuff = snap_malloc(size); //64Bytes aligned malloc
    if (vol_ibuff == NULL)
        goto out_error;
    //memset(vol_ibuff, 0, size);
    
    fprintf(stdout, "At beginning, var = %d\n",var);
    
    // prepare params to be written in MMIO registers for action
    type_in = SNAP_ADDRTYPE_HOST_DRAM;
    addr_in = (unsigned long)vol_ibuff;
    
    /* Allocate in host memory the place to put the text processed */
    vol_obuff = snap_malloc(size); //64Bytes aligned malloc
    if (vol_obuff == NULL)
        goto out_error;
    //memset(favol_obuff, 0x0, size);
    
    // prepare params to be written in MMIO registers for action
    type_out = SNAP_ADDRTYPE_HOST_DRAM;
    addr_out = (unsigned long)vol_obuff;
    
    // Allocate the card that will be used
    printf("/dev/cxl/afu%d.0s\n", card_no);
    snprintf(device, sizeof(device)-1, "/dev/cxl/afu%d.0s", card_no);
    card = snap_card_alloc_dev(device, SNAP_VENDOR_ID_IBM,
                               SNAP_DEVICE_ID_SNAP);
    if (card == NULL) {
        fprintf(stderr, "err: failed to open card %u: %s\n",
                card_no, strerror(errno));
        fprintf(stderr, "Default mode is FPGA mode.\n");
        fprintf(stderr, "Did you want to run CPU mode ? => add SNAP_CONFIG=CPU before your command.\n");
        fprintf(stderr, "Otherwise make sure you ran snap_find_card and snap_maint for your selected card.\n");
        goto out_error;
    }
    
    printf("before snap_attach_action\n");
    // Attach the action that will be used on the allocated card
    action = snap_attach_action(card, INTERFACETEST_ACTION_TYPE, action_irq, 60);
    if (action == NULL) {
        fprintf(stderr, "err: failed to attach action %u: %s\n",
                card_no, strerror(errno));
        goto out_error1;
    }
    
    // Fill the stucture of data exchanged with the action
    snap_prepare_interfacetest(&cjob, &mjob,
                               (void *)addr_in,  size, type_in,
                               (void *)addr_out, size, type_out);
    
    printf("ibuff address %08lx\n", addr_in);
    printf("obuff address %08lx\n", addr_out);

    // uncomment to dump the job structure
    //__hexdump(stderr, &mjob, sizeof(mjob));
    // Collect the timestamp BEFORE the call of the action
    gettimeofday(&stime, NULL);
    rc = snap_action_sync_execute_job_set_regs(action, &cjob, &mmio_out);
        if (rc != 0)
		goto out_error2;
    /* Start Action and wait for finish */
    snap_action_start(action);
    // Display the time of the action call
    gettimeofday(&etime, NULL);
    fprintf(stdout, "SNAP registers set + action start took %lld usec\n", (long long)timediff_usec(&etime, &stime));
    vol_ibuff[0] = var;
    vol_ibuff[1] = 0;
    vol_ibuff[2] = 0;
    gettimeofday(&stime, NULL);
    //int done=0;
    int number=0;
    int flag=0;
    while (number < 10)
    {
        number ++;
        //mjob.done = 0;
        fprintf(stdout, "num = %d, var is %d\n", number, var);
        printf("num = %d, var is %d\n", number, var);
        // Collect the timestamp BEFORE the call of the action
        //gettimeofday(&stime, NULL);
        
        // Call the action will:
        //    write all the registers to the action (MMIO)
        //  + start the action
        //  + wait for completion
        //  + read all the registers from the action (MMIO)
        //rc = snap_action_sync_execute_job(action, &cjob, timeout);

        while (flag != 1){
        	flag=vol_obuff[1];
                fprintf(stdout, "ibuff=%d, obuff=%d, inbuff_done=%d, outbuff_done=%d, final=%d, var=%d, cnt=%d\n", vol_ibuff[0], vol_obuff[0], vol_ibuff[1], flag, vol_obuff[2], var, vol_obuff[3]);
	        fprintf(stdout, "enter while on sw\n");
	}

        snap_mmio_write32(card, ACTION_CONTROL, ACTION_CONTROL_START);
        // Collect the timestamp AFTER the call of the action
        //gettimeofday(&etime, NULL);
        fprintf(stdout, "ibuff=%d, obuff=%d, out_done=%d, final=%d, var=%d, cnt=%d\n", vol_ibuff[0], vol_obuff[0], flag, vol_obuff[2], var, vol_obuff[3]);
        fprintf(stdout, "obuff_add=%p, ibuff_add=%p\n", vol_obuff, vol_ibuff);
	var = vol_obuff[0];
        printf("after HW operation, obuff[0] is %d. \n", var);
        var= var * 2;
         printf("after SW operation, var is %d. \n", var);
	vol_ibuff[0] = var;
        vol_ibuff[1] = 0;
        snap_mmio_write32(card, ACTION_CONTROL, ACTION_CONTROL_START);
        flag=0;
        fprintf(stdout, "ibuff=%d, obuff=%d, in_done=%d, final=%d, var=%d, cnt=%d\n", vol_ibuff[0],vol_obuff[0], vol_ibuff[1], vol_obuff[2], var, vol_obuff[3]);
        //fprintf(stdout, "INFO: It took %lld usec \n", (long long)timediff_usec(&etime, &stime));        
    }
        printf("out while loop\n");
		// Collect the timestamp AFTER the last step of the action completion
		gettimeofday(&etime, NULL);
		vol_ibuff[0]=0;
                vol_ibuff[1]=1;
		vol_ibuff[2]=1; 
		rc = snap_action_sync_execute_job_check_completion(action, &cjob, timeout, mmio_out);
		// Display the time of the action call (MMIO registers filled + execution)
		fprintf(stdout, "SNAP interfacetest closing action took %lld usec\n",(long long)timediff_usec(&etime, &stime));
           // test return code
        (cjob.retc == SNAP_RETC_SUCCESS) ? fprintf(stdout, "SUCCESS\n") : fprintf(stdout, "FAILED\n");
        if (cjob.retc != SNAP_RETC_SUCCESS) {
            fprintf(stderr, "err: Unexpected RETC=%x!\n", cjob.retc);
            goto out_error2;
        }
    // Detach action + disallocate the card
    snap_detach_action(action);
    snap_card_free(card);
    //__free(vol_obuff);
    //__free(vol_ibuff);
    exit(exit_code);
    
out_error2:
    snap_detach_action(action);
out_error1:
    snap_card_free(card);
out_error:
    //__free(vol_obuff);
    //__free(vol_ibuff);
    exit(EXIT_FAILURE);
}