/* @description Redesigned CAN library for the FLEXCAN module in the Teensy 3.1.
 * @file can.c
 * @author hngiannopoulos
 */

#include "can.h"
#include "kinetis_flexcan.h"
#define FIFO_START_INT_BIT 5

FLEXCAN_callback_t isr_table[32];

/* Store configuration settings in case of reset */
FLEXCAN_config_t config_g;

void FLEXCAN_freeze(void);
void FLEXCAN_unfreeze(void);

/* =========================================================================  */
/* Implement ISRs                                                             */
/* =========================================================================  */

void can0_message_isr( void )
{
   uint32_t flags;
   NVIC_CLEAR_PENDING(IRQ_CAN_MESSAGE);

   flags = FLEXCAN0_IFLAG1;

   /* Ignore all bits bellow fifo start bit (these are undefined) */
   flags >>= FIFO_START_INT_BIT; 
   for(uint8_t i = FIFO_START_INT_BIT; i < 32; i++)
   {
      if(flags & 0x00000001)
      {
         if(isr_table[i] != NULL)
            (isr_table[i])(i);

         FLEXCAN0_IFLAG1 |= (1 << i);
         return;
      }
      else
      {
         flags >>= 1;
      }

   }
   return;
}

/*
void can0_error_isr(void)
{
   NVIC_CLEAR_PENDING(IRQ_CAN_MESSAGE);
   FLEXCAN0_ESR1 |= FLEXCAN_ESR_BIT1_ERR | FLEXCAN_ESR_BIT0_ERR;
   for (uint8_t i = FLEXCAN_TX_BASE_MB; i < (FLEXCAN_TX_BASE_MB + FLEXCAN_TX_MB_WIDTH); i++) 
   {  
      FLEXCAN_abort_mb(i);
   }

}
*/

//void can0_error_isr(void);
//extern void can0_bus_off_isr(void);
//extern void can0_tx_warn_isr(void);
//extern void can0_rx_warn_isr(void);
//extern void can0_wakeup_isr(void);

/* =========================================================================  */
/* END: Implement ISRs                                                        */
/* =========================================================================  */


/* Local function ONLY. */
void FLEXCAN_freeze(void)
{
   FLEXCAN0_MCR |= FLEXCAN_MCR_HALT;
   FLEXCAN0_MCR |= FLEXCAN_MCR_FRZ;

   /* Wait for the CAN hardware to stop. */
   while(!(FLEXCAN0_MCR & FLEXCAN_MCR_FRZ_ACK))
      ;
}

/* Local function ONLY. */
void FLEXCAN_unfreeze(void)
{
   FLEXCAN0_MCR &= ~(FLEXCAN_MCR_FRZ);
   /* Wait for the hardware to be ready */
   while((FLEXCAN0_MCR & FLEXCAN_MCR_FRZ_ACK))
      ;
}


int FLEXCAN_init(FLEXCAN_config_t config)
{

   /* Save the most recent configuration */

   config_g = config;
   // set up the pins, 3=PTA12=CAN0_TX, 4=PTA13=CAN0_RX
   CORE_PIN3_CONFIG = PORT_PCR_MUX(2);
   CORE_PIN4_CONFIG = PORT_PCR_MUX(2);// | PORT_PCR_PE | PORT_PCR_PS;

   // select clock source 16MHz xtal
   OSC0_CR           |= OSC_ERCLKEN;
   SIM_SCGC6         |=  SIM_SCGC6_FLEXCAN0;
   FLEXCAN0_CTRL1    &= ~FLEXCAN_CTRL_CLK_SRC;

   /* Set The freeze register to allow for changing settings */ 

   /* Enable the Flexcan Module */ 
   FLEXCAN0_MCR &= ~FLEXCAN_MCR_MDIS;

   FLEXCAN_freeze();

   /* ENABLE MCR REGISTER */

   FLEXCAN0_MCR |= FLEXCAN_MCR_SRX_DIS;   // disable self-reception
   FLEXCAN0_MCR |= FLEXCAN_MCR_IRMQ;

   FLEXCAN0_MCR &= ~FLEXCAN_MCR_MAXMB_MASK;
   FLEXCAN0_MCR |= FLEXCAN_MCR_MAXMB(FLEXCAN_MAX_MB);  // Enable the maximum mailbox.

   FLEXCAN0_MCR |= FLEXCAN_MCR_FEN;       //enable RX FIFO
   FLEXCAN0_MCR |= FLEXCAN_MCR_WRN_EN;    //Enable FLExcan Warnings
   FLEXCAN0_MCR |= FLEXCAN_MCR_AEN;       // Enable Abort commands for MBs
   FLEXCAN0_MCR |= FLEXCAN_MCR_LPRIO_EN;  // Enable local priority (1)
   FLEXCAN0_MCR |= FLEXCAN_MCR_IDAM(FLEXCAN_ID_FILT_TYPE);   // (00) ONE FULL (Extended) ID per table element 
                                          // (01) Two full standard IDS per filter table element.
                                          // (10) Four partial (8-Bit) Standard IDS per table element.
                                          // (11) All Frames Rejected
                                          
   FLEXCAN0_CTRL1 = (FLEXCAN_CTRL_PROPSEG(config.propseg) | FLEXCAN_CTRL_RJW(config.rjw) | 
                     FLEXCAN_CTRL_PSEG1(config.pseg_1) | FLEXCAN_CTRL_PSEG2(config.pseg_2) | 
                     FLEXCAN_CTRL_PRESDIV(config.presdiv));


   /* =========== Handle Interrupts ============================== */
   FLEXCAN0_CTRL1 |= FLEXCAN_CTRL_BOFF_MSK;  // ENABLE BUS OFF INTERRUPT MASKB
   NVIC_ENABLE_IRQ(IRQ_CAN_BUS_OFF);

   FLEXCAN0_CTRL1 |= FLEXCAN_CTRL_ERR_MSK;   // Enable Error INT. Mask
   NVIC_ENABLE_IRQ(IRQ_CAN_ERROR);
   
   FLEXCAN0_CTRL1 |= FLEXCAN_CTRL_TWRN_MSK;  // Enable TX Warning INT. Mask
   NVIC_ENABLE_IRQ(IRQ_CAN_TX_WARN);

   //NVIC_ENABLE_IRQ(IRQ_CAN_RX_WARN);
   //NVIC_ENABLE_IRQ(IRQ_CAN_WAKEUP);
   //NVIC_ENABLE_IRQ(IRQ_CAN_MESSAGE);

   //FLEXCAN0_CTRL1 |= FLEXCAN_CTRL_RWRN_MSK;  // Enable RX Warning INT. Mask
   FLEXCAN0_CTRL1 |= FLEXCAN_CTRL_BOFF_REC;  // DISABLE (1) automatic bus off recovery.
   FLEXCAN0_CTRL1 |= FLEXCAN_CTRL_LBUF;      // Send the lowest number buffer is transmitted first.
   FLEXCAN0_CTRL1 |= FLEXCAN_CTRL_SMP;       // Enable tripple bit sampling.

   /* DEFAULT: Set to accept all messages */
   FLEXCAN0_RXFGMASK = 0;


   FLEXCAN0_CTRL2 |= FLEXCAN_CTRL2_MRP;   // Start Matching through MB first.
   FLEXCAN_set_rffn(FLEXCAN0_CTRL2, 0);   // 0 - 8 RX FIFOS. 
                                          // 1 - 16 RX fifos.


   FLEXCAN_unfreeze();

   // FLEXCAN0_IMASK1 - Used for masking Interrupts for mailboxes.
   // FLEXCAN0_IFLAG -  Interrupt flags for mailboxes.
   
   /* set tx buffers to inactive */
   uint8_t i;
   for (i = FLEXCAN_RX_BASE_MB; i < FLEXCAN_MAX_MB; i++) 
   {
      /* Clear IFLAGS */
      FLEXCAN0_IFLAG1 |= (1<<i);
      FLEXCAN0_MBn_CS(i) = FLEXCAN_MB_CS_CODE(FLEXCAN_MB_CODE_TX_INACTIVE);
   }

   return FLEXCAN_SUCCESS;
}


/** Deitilize the CAN hardware.
 * @return FLEXCAN_SUCCESS or FLEXCAN_ERROR
 */
// TODO: 
int FLEXCAN_deinit(void)
{
   return FLEXCAN_SUCCESS;
}

/** Reads a frame from a specified mailbox.
 * @param mb The mailbox to read from.
 * @param frame Pointer to a struct to copy data into.
 * @return FLEXCAN_SUCCESS or FLEXCAN_ERROR.
 */
int FLEXCAN_read_frame(uint8_t mb, FLEXCAN_frame_t *frame)
{
   uint32_t data_in;
   // get identifier and dlc
   frame->dlc = FLEXCAN_get_length(FLEXCAN0_MBn_CS(mb ));
   frame->ide = (FLEXCAN0_MBn_CS(mb ) & FLEXCAN_MB_CS_IDE)? 1:0;

   frame->id  = (FLEXCAN0_MBn_ID(mb) & FLEXCAN_MB_ID_EXT_MASK);

   /* Shift the frame back if necessary */
   if(!frame->ide)
      frame->id >>= FLEXCAN_MB_ID_STD_BIT_NO;

   data_in = FLEXCAN0_MBn_WORD0(mb);
   frame->data[3] = data_in & 0xFF;
   data_in >>= 8;
   frame->data[2] = data_in & 0xFF;
   data_in >>= 8;
   frame->data[1] = data_in & 0xFF;
   data_in >>= 8;
   frame->data[0] = data_in & 0xFF;
   data_in >>= 8;

   if(frame->dlc > 4)
   {
      data_in = FLEXCAN0_MBn_WORD0(mb);
      frame->data[7] = data_in & 0xFF;
      data_in >>= 8;
      frame->data[6] = data_in & 0xFF;
      data_in >>= 8;
      frame->data[5] = data_in & 0xFF;
      data_in >>= 8;
      frame->data[4] = data_in & 0xFF;
      data_in >>= 8;
   }

   return FLEXCAN_SUCCESS;
}

/** Writes to a mailbox.
 * @param mb Number of the mailbox to write to [8 - 63] avalible by default.
 * @param code Mailbox Setup code.  
 * @param frame The frame to write to the mailbox. 
 * @return FLEXCAN_SUCCESS or FLEXCAN_ERROR.
 */
int FLEXCAN_mb_write(uint8_t mb, uint8_t code, FLEXCAN_frame_t frame)
{
   
   FLEXCAN_abort_mb(mb);

   /* Assume there are no extended ID message */
   FLEXCAN0_MBn_ID(mb) = FLEXCAN_MB_ID_IDSTD(frame.id);

   FLEXCAN0_MBn_WORD0(mb) = (frame.data[0]<<24)|(frame.data[1]<<16)|(frame.data[2]<<8)|frame.data[3];
   FLEXCAN0_MBn_WORD1(mb) = (frame.data[4]<<24)|(frame.data[5]<<16)|(frame.data[6]<<8)|frame.data[7];

   /* Assume there are never any Extended ID mEsagesg */
   /* TODO: */

    FLEXCAN0_MBn_CS(mb) = FLEXCAN_MB_CS_CODE(code)
                              | FLEXCAN_MB_CS_LENGTH(frame.dlc);
   
   return FLEXCAN_SUCCESS;
}

/** Reads a mailbox.
 * @param mb Number of the mailbox to read [8-63] avalible with fifo en.
 * @code Pointer to copy code value into.
 * @param timestamp Pointer to value for read timestamp to be copied into.
 * @param frame Pointer to frame to copy data into.
 * @return FLEXCAN_SUCCESS or FLEXCAN_ERROR.
 */
int FLEXCAN_mb_read(uint8_t mb, uint8_t * code, uint16_t *timestamp, FLEXCAN_frame_t* frame)
{
   
   return FLEXCAN_SUCCESS;
}

/** Register a callback for when a RX mailbox is filled.
 * @param mb Number of the mailbox to register.
 * @param cb Callback function to register.
 * @return FLEXCAN_SUCCESS or FLEXCAN_ERROR.
 */
int FLEXCAN_mb_reg_callback(uint8_t mb, FLEXCAN_callback_t cb)
{
   NVIC_ENABLE_IRQ(IRQ_CAN_MESSAGE);
   FLEXCAN0_IMASK1 |= (1 << mb);
   isr_table[mb] = cb;
   return FLEXCAN_SUCCESS;
}

/** Disable callback for the specific mailbox.
 * @param mb The Mailbox to disable.
 * @return FLEXCAN_SUCCESS or FLEXCAN_ERROR.
 */
int FLEXCAN_mb_unreg_callback(uint8_t mb)
{

   FLEXCAN0_IMASK1 &= ~(1 << mb);
   isr_table[mb] = NULL;
   return FLEXCAN_SUCCESS;

}

/** Register callback function for FIFO half full.
 * @param cb Callback Function.
 * @return FLEXCAN_SUCCESS or FLEXCAN_ERROR.
 */
int FLEXCAN_fifo_reg_callback(FLEXCAN_callback_t cb)
{
   FLEXCAN_mb_reg_callback(FLEXCAN_INT_FIFO_AVALIBLE, cb);
   return FLEXCAN_SUCCESS;
}

/** Disable the callback for the FIFO.
 * @return FLEXCAN_SUCCESS or FLEXCAN_ERROR.
 */
int FLEXCAN_fifo_unreg_callback(void)
{
   FLEXCAN_mb_unreg_callback(FLEXCAN_INT_FIFO_AVALIBLE);
   return FLEXCAN_SUCCESS;
}

/** Checks to see if there are any messages in the FIFO.
 * @return The number of messages in the FIFO.
 */
int FLEXCAN_fifo_avalible()
{
   return (FLEXCAN0_IFLAG1 & FLEXCAN_IMASK1_BUF5M);
}

/** Read a message from the fifo.
 * @param frame Pointer to the frame to copy the data into.
 * @return FLEXCAN_SUCCESS or FLEXCAN_ERROR.
 */
int FLEXCAN_fifo_read(FLEXCAN_frame_t * frame)
{
   if(!FLEXCAN_fifo_avalible())
   {
      return FLEXCAN_FIFO_EMPTY;
   }

   FLEXCAN_read_frame(FLEXCAN_FIFO_MB, frame);
   
   /* Clear the flag */
   FLEXCAN0_IFLAG1 |= FLEXCAN_IMASK1_BUF5M;
   return FLEXCAN_SUCCESS;

}

int FLEXCAN_write(FLEXCAN_frame_t frame, FLEXCAN_tx_option_t option)
{
   int mb = FLEXCAN_TX_BASE_MB + 1;

   /* Assume MB 10 For now */ 
   FLEXCAN_mb_write(mb, FLEXCAN_MB_CODE_TX_ONCE, frame);

   switch(option)
   {
      case TX_ONCE:                 /* TX Once */
         if(FLEXCAN_abort_mb(mb) != FLEXCAN_TX_ABORTED)
            return FLEXCAN_TX_SUCCESS;

         else
            return FLEXCAN_TX_ABORTED;
         
         break;

      case TX_BEST_EFFORT:          /* TX Until DEAD */
         /* Just Let it go and return */
         break;

      default:
         break;

   }
   return FLEXCAN_SUCCESS;
}

int FLEXCAN_reset(void)
{

   uint32_t imask_1;

   /* Store some configurations before reseting */
   imask_1 = FLEXCAN0_IMASK1;

   FLEXCAN0_MCR |= FLEXCAN_MCR_SOFT_RST;

   /* Wait for reset to process */
   while(FLEXCAN0_MCR & FLEXCAN_MCR_SOFT_RST)
      ;
   
   FLEXCAN_init(config_g);
   FLEXCAN0_IMASK1 = imask_1;
   return FLEXCAN_SUCCESS;
}

int FLEXCAN_abort_mb(uint8_t mb)
{

   int return_val = FLEXCAN_TX_ABORTED;
   /* Clear the corresponding IFLAG */

   /* Check Corresponding IFLAG (Clears if Asserted) */
   if(mb < 32)
      FLEXCAN0_IFLAG1 |= (1<<mb);
   else
      FLEXCAN0_IFLAG2 |= (1<<(mb-32));

   /* Writes the ABORT */
   FLEXCAN0_MBn_CS(mb) = FLEXCAN_MB_CS_CODE(FLEXCAN_MB_CODE_TX_ABORT);

   /* Waits for the IFLAG again this will show that the message was tx or aborted*/ 
   if(mb < 32)
   {
      while(!(FLEXCAN0_IFLAG1 & (1<<mb)))
         ;
   }
   else
   {
      while(!(FLEXCAN0_IFLAG2 & (1<<(mb - 32))))
         ;
   }

   /* Reads code to see if it was aborted */
   if(FLEXCAN0_MBn_CS(mb) != FLEXCAN_MB_CS_CODE(FLEXCAN_MB_CODE_TX_ABORT))
      return_val = FLEXCAN_TX_SUCCESS;

   if(mb < 32)
      FLEXCAN0_IFLAG1 |= (1<<mb);
   else
      FLEXCAN0_IFLAG2 |= (1<<(mb-32));

   return return_val;

}

uint32_t FLEXCAN_filter_a(uint8_t rtr, uint8_t ide, uint32_t ext_id)
{
   uint32_t filt_a = 0;
   if(rtr)
   {
      filt_a |= (1<<31);
   }

   if(ide)
   {
      filt_a |= (1<<30);
      filt_a |= ((ext_id & 0x3fffffff) << 1);
   }
   else
   {
      filt_a |= ((ext_id & 0x0007FFF)  << 19);
   }
   return filt_a;
}

/** Forms a filter mask in format B. Format B contains filter for 2 standard ID.
 *  @param rtr_a
 *  @param rtr_b
 *  @param ide_a
 *  @param ide_b
 *  @param id_a Filters only on upper (MSB) 14-bit arbitration ID.
 *  @param id_b Filters only on upper (MSB) 14-bit arbitration ID.
 *  @return Properly formatted acceptance filter. 
 */
uint32_t FLEXCAN_filter_b(uint8_t rtr_a, 
                           uint8_t rtr_b,
                           uint8_t ide_a, 
                           uint8_t ide_b, 
                           uint16_t id_a, 
                           uint16_t id_b)
{
   uint16_t filt_1 = 0;
   uint16_t filt_2 = 0;

   /* Form first part of filter */
   if(rtr_a)
   {
      filt_1 |= (1<<15);
   }

   if(ide_a)
   {
      filt_1 |= (1<<14);
      filt_1 |= (id_a & 0x1fff);
   }
   else 
   {
      filt_1 |= ((id_a & 0x7FFF) << 3);
   }

   /* Form second part of filter */
   if(rtr_b)
   {
      filt_2 |= (1<<15);
   }

   if(ide_b)
   {
      filt_2 |= (1<<14);
      filt_2 |= (id_a & 0x1fff);
   }

   else
   {
      filt_2 |= ((id_a & 0x7FFF) << 3);
   }

   return (filt_1 << 16) | filt_2;
}

/** Forms a filter mask in format B. Filters on 4 (upper) 8-bit ID segments. 
 * @param id Array of 8-Bit ID slices. 
 * @param len Length of the ID array (MAX - 4).
 * @return Properly formatted acceptance filter. 
 */
uint32_t FLEXCAN_filter_c(uint8_t * id, uint8_t len)
{
   uint32_t filt_c = 0;   
   uint8_t i;
   for(i = 0; i < len; i++)
   {
      filt_c |= id[i] << ((8 * i) - 1);
   }
   return filt_c;
}

int FLEXCAN_status(FLEXCAN_status_t *status)
{
   status->tx_err_cnt = FLEXCAN_ECR_TX_ERR_COUNTER(FLEXCAN0_ECR);
   status->rx_err_cnt = FLEXCAN_ECR_RX_ERR_COUNTER(FLEXCAN0_ECR);

   /* First read gets all of the errors */
   status->errors    = FLEXCAN0_ESR1;

   status->tx_wrn    = status->errors & FLEXCAN_ESR_TX_WRN;
   status->rx_wrn    = status->errors & FLEXCAN_ESR_RX_WRN;
   status->flt_conf  = FLEXCAN_ESR_get_fault_code(status->errors);
   
   return FLEXCAN_SUCCESS;
}

