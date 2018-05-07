#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ap_int.h"
#include "action_interfacetest_hls.h"

#define RELEASE_LEVEL       0x00000010




//Pricing a European vanilla call option with a Monte Carlo method
//void monte_carlo_price(double sum, double gauss_bm[NUM_ENG], double S, double K, double r, double v, double T, int cp_flag)
//void monte_carlo_price(float gauss_bm[NUM_ENG], int cp_flag, float sum[0])
//void monte_carlo_price(int cp_flag, float gauss_bm[NUM_ENG],float sum[0])
/*void monte_carlo_price(int cp_flag, int num_sims[0], float S[0], float K[0], float r[0], float v[0], float T[0], float sum[0])
{
    //my_srandom(seed);
    float payoff_sum = 0.0;
    float pre_payoff_sum = 0.0;
    float gauss_bm[NUM_ENG];
    float mid_value1 = exp(sqrt(v[0]*v[0]*T[0]));
    float S_adjust = S[0] * exp(T[0]*(r[0]-0.5*v[0]*v[0]));
    float mid_value2 = exp(-r[0]*T[0]);
    float para = K[0];
    int cnt = num_sims[0];
    int i;
    
    for(i=0; i<cnt;i=i+100)
    {
        gaussian_box_muller(gauss_bm);
        pre_payoff_sum = MC(cp_flag, gauss_bm, mid_value1, S_adjust, para);
        payoff_sum = pre_payoff_sum + payoff_sum;
        //cnt -= 100;
    }
    //payoff_sum = MC(cp_flag, mid_value1, S_adjust);
    //printf("%f \n", payoff_sum / (double)(NUM_ENG));
    //printf("%f \n", (payoff_sum / (double)(NUM_ENG))* exp(-r*T));
    sum[0] = (payoff_sum / (float)(cnt)) * mid_value2;
    //return sum;
}*/

/*static snapu32_t write_bulk (snap_membus_t *tgt_mem,
                             snapu64_t      byte_address,
                             snapu32_t      byte_to_transfer,
                             snap_membus_t *buffer)
{
    snapu32_t xfer_size;
    xfer_size = MIN(byte_to_transfer, (snapu32_t)  MAX_NB_OF_BYTES_READ);
    memcpy((snap_membus_t *)(tgt_mem + (byte_address >> ADDR_RIGHT_SHIFT)), buffer, xfer_size);
    return xfer_size;
}

void write_out_buf (snap_membus_t  * tgt_mem, snapu64_t address, float * out_buf)
{
    snap_membus_t lines;
    //Convert it to one cacheline.
    lines(31,0) = out_buf[0];
    lines(63,32) = 0;
    lines(95,64) = 0;
    lines(127,96) = 0;
    lines(159,128) = 0;
    lines(191,160) = 0;
    lines(223,192) = 0;
    lines(255,224) = 0;
    lines(287,256) = 0;
    lines(319,288) = 0;
    lines(351,320) = 0;
    lines(383,352) = 0;
    lines(415,384) = 0;
    lines(447,416) = 0;
    lines(479,448) = 0;
    lines(511,480) = 0;
    
    write_bulk(tgt_mem, address, BPERCL, &lines);
}*/
#define N 2
#define M 2
#define P 2

void MatrixMultiply(int32_t a[M][N], int32_t b[N][P], int32_t **c)
{
    int i, j, k;  
    int32_t temp;  
    for(i = 0; i < M; i++){  
        for(j = 0; j < P; j++){  
            temp = 0;  
            for(k = 0; k < N; k++){  
                temp += a[i][k] * b[k][j];  
            }  
            c[i][j] = temp;
        }  
    }
} 
void write_out_buf (snap_membus_t  * tgt_mem, snapu64_t address, int32_t **buf_out)
{
//#pragma HLS ARRAY_PARTITION variable=buf_out complete dim=0
    snap_membus_t lines[M*P];
    //explicit data type casting
    int i,j;
    int k=0;
    for(i=0; i<M; i++)
    {
    	for(j=0; j<P; j++)
        {
          lines[k] = * (snap_membus_t*)buf_out;
        }
    }
    //lines[0] = * (snap_membus_t*)buf_out;
    //lines[1] = * (snap_membus_t*)(buf_out + 16);

//   short i=0;
//   for (i = 0; i < 16; i++) 
//       lines[0](31 + i*32, i*32) = buf_out[i];
//   for (i = 0; i < 16; i++) 
//       lines[1](31 + i*32, i*32) = buf_out[16+i];

    memcpy(tgt_mem + (address >> ADDR_RIGHT_SHIFT),
           lines,
           sizeof(int32_t));
    
}


static snapu32_t read_bulk ( snap_membus_t *src_mem,
        snapu64_t      byte_address,
        snapu32_t      byte_to_transfer,
        snap_membus_t *buffer)
{

    snapu32_t xfer_size;
    xfer_size = MIN(byte_to_transfer, (snapu32_t) MAX_NB_OF_BYTES_READ);
    memcpy(buffer, (snap_membus_t *) (src_mem + (byte_address >> ADDR_RIGHT_SHIFT)), xfer_size);
    return xfer_size;
}

void read_matrix(snapu32_t matrix_size_col, snapu32_t matrix_size_row, int32_t ** matrix_array, snapu64_t  address, snap_membus_t * src_mem )
{
    if((matrix_size_col <=0) || (matrix_size_row <= 0))
        return;

    snapu64_t 	    address_xfer_offset = 0;
    snap_membus_t   block_buf;
    snapu32_t left_bytes = matrix_size_col * matrix_size_row * sizeof (int32_t);
    snapu32_t xfer_bytes;
    //ap_uint<VEX_WIDTH> index = 0;
    snapu32_t i, j;
    snapu32_t k=0;
    while (left_bytes > 0)
    {
        xfer_bytes = read_bulk(src_mem, address + address_xfer_offset, left_bytes, block_buf);

        for(i = 0; i < (matrix_size_col*matrix_size_row); i++)
        {
        	for(j=0; j<matrix_size_row; j++)
                {
                	matrix_array[i][j] = block_buf(k+31,k);
                        k = k+32;
                }
            }
        }
        left_bytes -= xfer_bytes;
        address_xfer_offset += MAX_NB_OF_BYTES_READ;
    }
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
#pragma HLS INTERFACE s_axilite port=din_gmem bundle=ctrl_reg 		offset=0x030
#pragma HLS INTERFACE s_axilite port=dout_gmem bundle=ctrl_reg 		offset=0x040
    
    //DDR memory Interface
//#pragma HLS INTERFACE m_axi port=d_ddrmem bundle=card_mem0 offset=slave depth=512
//#pragma HLS INTERFACE s_axilite port=d_ddrmem bundle=ctrl_reg 		offset=0x050
    
    // Host Memory AXI Lite Master Interface
#pragma HLS DATA_PACK variable=Action_Config
#pragma HLS INTERFACE s_axilite port=Action_Config bundle=ctrl_reg	offset=0x010
#pragma HLS DATA_PACK variable=Action_Register
#pragma HLS INTERFACE s_axilite port=Action_Register bundle=ctrl_reg	offset=0x100
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
            Action_Config->action_type = (snapu32_t)MC_ACTION_TYPE;
            Action_Config->release_level = (snapu32_t)RELEASE_LEVEL;
            Action_Register->Control.Retc = (snapu32_t)0xe00f;
            return;
        default:
            break;
    }
    int32_t var1[N][N];
    int32_t var2[N][N];
    int32_t result[N][N];

    read_matrix(N, N, var1, i_idx, (snap_membus_t *)(din_gmem + i_idx));
    read_matrix(N, N, var1, i_idx, (snap_membus_t *)(din_gmem + i_idx));
    MatrixMultiply(var1, var2, &result);
    write_out_buf (o_idx, o_idx, result)l;

    /*snap_membus_t ibuffer[MAX_NB_OF_BYTES_READ/BPERDW];
    snapu32_t xfer_size;
    xfer_size = MIN((snapu32_t) sizeof(int32_t), (snapu32_t) MAX_NB_OF_BYTES_READ);
    memcpy(ibuffer, (snap_membus_t *)(din_gmem + i_idx), xfer_size);
    var = ibuffer[0](32, 0);
    var = var+1;

    snap_membus_t obuffer[MAX_NB_OF_BYTES_READ/BPERDW];
    obuffer[0](32, 0) = var;
    memcpy((dout_gmem + o_idx), obuffer, sizeof(int32_t));
    //MAT_ROWS = act_reg->Data.rows ;
    //MAT_COLS = act_reg->Data.cols ;

    //int32_t mat_a_t[MAT_ROWS][MAT_COLS];
    //number = Action_Register->Data.size;
    //bytes_to_transfer = MIN(size, BPERDW);
    /*for (int k=0; k<MAT_ROWS; k++)
    {
        for(int j=0; j<MAT_COLS; j++)
        {
           while(number > 0)
           {
           	temp = (din_gmem + i_idx
                mat_a_t[k][j] = (din_gmem + i_idx)(31, 0);
           
        }
    }
    }*/
    //write_out_buf(dout_gmem,output_address, &sum);
    if (rc != 0)
        ReturnCode = SNAP_RETC_FAILURE;
    
    Action_Register->Control.Retc = ReturnCode;
    return;
    
}
