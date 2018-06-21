
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ap_int.h"
#include "action_interfacetest_hls.h"

#define RELEASE_LEVEL       0x00000011

static int process_action(snap_membus_t *din_gmem,
	      snap_membus_t *dout_gmem,
	      /* snap_membus_t *d_ddrmem, *//* not needed */
	      action_reg *Action_Register)
{

    short rc = 0;
    uint64_t i_idx, o_idx;
    uint32_t size;
    snapu32_t ReturnCode = SNAP_RETC_SUCCESS;
    
    i_idx = Action_Register->Data.in.addr >> ADDR_RIGHT_SHIFT;
    o_idx = Action_Register->Data.out.addr >> ADDR_RIGHT_SHIFT;
    int32_t done;
    int32_t final_done;
    done = 0;
    final_done = 0;
    int32_t cnt=0;
    int32_t var;
    int32_t var1;
    int32_t cont = 1;
    //snap_membus_t buffer[MAX_NB_OF_BYTES_READ/BPERDW];
    snap_membus_t buffer[128];
    snapu32_t xfer_size=128;
    //xfer_size = MIN(size, MAX_NB_OF_BYTES_READ);
    while (1)
    {
	cnt++;
    	//This time the size is 128 and less than MAX_NB_OF_BYTES_READ
    	//Read and write out.
   	memcpy(buffer, (din_gmem + i_idx), xfer_size);
    	var = buffer[0](31, 0);
    	done = buffer[0](63,32);
        final_done= buffer[0](95, 64);	
        if(!done)
        {
    		var1 = var1+1;
                done = 1;
    		buffer[0](31, 0) = var1;
                buffer[0](63, 32) = 1;
                buffer[0](127, 96) = cnt;
    		memcpy((dout_gmem + o_idx), buffer, xfer_size);
	}
        buffer[0](63, 32) = 0;
    	memcpy((dout_gmem + o_idx), buffer, xfer_size);
        //done = 0;

        if(final_done == 1)
	{
		break;
	}
    }
    if (rc != 0)
        ReturnCode = SNAP_RETC_FAILURE;
    
    Action_Register->Control.Retc = ReturnCode;
    return 0;
}

void hls_action(snap_membus_t  *din_gmem,
                snap_membus_t  *dout_gmem,
                //snap_membus_t  *d_ddrmem,
                action_reg            *Action_Register,
                action_RO_config_reg  *Action_Config)
{
    // Host Memory AXI Interface
#pragma HLS INTERFACE m_axi port=din_gmem bundle=host_mem offset=slave depth=512
#pragma HLS INTERFACE m_axi port=dout_gmem bundle=host_mem offset=slave depth=512
#pragma HLS INTERFACE s_axilite port=din_gmem bundle=ctrl_reg         offset=0x030
#pragma HLS INTERFACE s_axilite port=dout_gmem bundle=ctrl_reg         offset=0x040
    
    //DDR memory Interface
    //#pragma HLS INTERFACE m_axi port=d_ddrmem bundle=card_mem0 offset=slave depth=512
    //#pragma HLS INTERFACE s_axilite port=d_ddrmem bundle=ctrl_reg         offset=0x050
    
    // Host Memory AXI Lite Master Interface
#pragma HLS DATA_PACK variable=Action_Config
#pragma HLS INTERFACE s_axilite port=Action_Config bundle=ctrl_reg    offset=0x010
#pragma HLS DATA_PACK variable=Action_Register
#pragma HLS INTERFACE s_axilite port=Action_Register bundle=ctrl_reg    offset=0x100
#pragma HLS INTERFACE s_axilite port=return bundle=ctrl_reg
    
    
    /* Required Action Type Detection */
    switch (Action_Register->Control.flags) {
        case 0:
            Action_Config->action_type = (snapu32_t)INTERFACETEST_ACTION_TYPE;
            Action_Config->release_level = (snapu32_t)RELEASE_LEVEL;
            Action_Register->Control.Retc = (snapu32_t)0xe00f;
            return;
        default:
            break;
    }
	    process_action(din_gmem, dout_gmem, Action_Register);
    
}