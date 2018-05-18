
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ap_int.h"
#include "action_interfacetest_hls.h"

#define RELEASE_LEVEL       0x00000011

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
    
    short rc = 0;
    uint64_t i_idx, o_idx;
    snapu32_t ReturnCode = SNAP_RETC_SUCCESS;
    
    i_idx = Action_Register->Data.in.addr >> ADDR_RIGHT_SHIFT;
    //i_idx1 = act_reg->Data.in1.addr >> ADDR_RIGHT_SHIFT;
    o_idx = Action_Register->Data.out.addr >> ADDR_RIGHT_SHIFT;
    
    // byte address received need to be aligned with port width
    //input_address  = Action_Register->Data.in.addr;
    //output_address = Action_Register->Data.out.addr;
    
    
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
    int32_t var;
    snap_membus_t ibuffer[MAX_NB_OF_BYTES_READ/BPERDW];
    snapu32_t xfer_size;
    xfer_size = MIN((snapu32_t) sizeof(int32_t), (snapu32_t) MAX_NB_OF_BYTES_READ);
    memcpy(ibuffer, (snap_membus_t *)(din_gmem + i_idx), xfer_size);
    var = ibuffer[0](31, 0);
    var = var+1;
    
    snap_membus_t obuffer[MAX_NB_OF_BYTES_READ/BPERDW];
    obuffer[0](31, 0) = var;
    memcpy((dout_gmem + o_idx), obuffer, sizeof(int32_t));
    
    if (rc != 0)
        ReturnCode = SNAP_RETC_FAILURE;
    
    Action_Register->Control.Retc = ReturnCode;
    return;
    
}
