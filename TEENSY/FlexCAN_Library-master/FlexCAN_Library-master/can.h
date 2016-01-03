/* @description Redesigned CAN library for the FLEXCAN module in the Teensy 3.1.
 * @file can.h
 * @author hngiannopoulos
 */
#ifndef _CAN_H_
#define _CAN_H_

#include <Arduino.h>
#include "kinetis_flexcan.h"

#define FLEXCAN_SUCCESS       0     /*!< */
#define FLEXCAN_ERROR         1     /*!< */
#define FLEXCAN_FIFO_EMPTY    0     /*!< */
#define FLEXCAN_TX_ABORTED    2     /*!< */
#define FLEXCAN_TX_SUCCESS    0     /*!< */
#define FLEXCAN_MB_OUT_OF_BOUNDS 3  /*!< */

#define FLEXCAN_NUM_FIFO_FILTERS    8     /*!< Number of FIFO Filters used 16 - (RFFN = 1) Avalible MB start at 10. See FLEXCAN_CTRL2 */

#define FLEXCAN_FIFO_MB             0        /*!< Mailbox used for the FIFO.         */
#define FLEXCAN_RX_BASE_MB          8        /*!< Lowest MB used for RX callbacks.   */ 
#define FLEXCAN_RX_MB_WIDTH         5        /*!< Number of Mailboxes allocated for reception Callbacks. */
#define FLEXCAN_TX_BASE_MB          (FLEXCAN_RX_BASE_MB + FLEXCAN_RX_MB_WIDTH + 1)     /*!< Base mailbox for tx. */
#define FLEXCAN_TX_MB_WIDTH         (FLEXCAN_MAX_MB - FLEXCAN_TX_BASE_MB) /*!< Number of mailboxes allocated for transmission. */
#define FLEXCAN_MAX_MB              15

/* Interrupts */
#define FLEXCAN_INT_FIFO_OVERFLOW   7  /*!< Interrupt Number in IFLAGS1 that represents FIFO Overflow. */
#define FLEXCAN_INT_FIFO_WARNING    6  /*!< Interrupt Number in IFLAGS1 that represents FIFO Warning (Half Full). */
#define FLEXCAN_INT_FIFO_AVALIBLE   5  /*!< Interrupt Number in IFLAGS1 for data in the FIFO. */

#define FLEXCAN_ID_FILT_TYPE 0      // A - (00) ONE FULL (Extended) ID per table element 
                                    // B - (01) Two full standard IDS per filter table element.
                                    // C - (10) Four partial (8-Bit) Standard IDS per table element.
                                    // D - (11) All Frames Rejected

#define FLEXCAN_FILTER_A_MSK_RTR (1<<31)
#define FLEXCAN_FILTER_A_MSK_IDE (1<<30)
#define FLEXCAN_FILTER_A_MSK_ID  (0x3FFFE)

#define FLEXCAN_FILTER_B_MSK_RTR_A (1<<31)
#define FLEXCAN_FILTER_B_MSK_IDE_A (1<<30)
#define FLEXCAN_FILTER_B_MSK_ID_A  (0x3F00)
#define FLEXCAN_FILTER_B_MSK_RTR_B (1<<15)
#define FLEXCAN_FILTER_B_MSK_IDE_B (1<<14)
#define FLEXCAN_FILTER_B_MSK_ID_B  (0x003F)

/** Struct to hold a full FLEXCAN message.
 *
 */
typedef struct {
   uint8_t srr;      /*!< substute remote request: 1 - extended frame 0 - standard frame. */
   uint8_t ide;      /*!< ID Extended Bit: 1 - extended frame 0 - standard frame. */
   uint8_t rtr;      /*!< remote transmission request bit. */
   uint8_t dlc;      /*!< data length code [0 - 8]. */
   uint32_t id;      /*!< ID standard (bits 11:0) */
   uint8_t data[8];  /*!< Data payload up to 8 bits */
} FLEXCAN_frame_t;

/** Configuration Struct for the FLEXCAN hardware.
 * 
 */
typedef struct {
   uint8_t presdiv;  /*!< Prescale division factor. */
   uint8_t propseg;  /*!< Prop Seg length. */
   uint8_t rjw;      /*!< Sychronization Jump Width*/
   uint8_t pseg_1;   /*!< Phase 1 length */
   uint8_t pseg_2;   /*!< Phase 2 length */

} FLEXCAN_config_t;

/** A Struct for the state of the FLEXCAN Hardware. 
 *
 */
typedef struct {
   uint8_t tx_err_cnt;  /*!< Transmit Error Counter. */
   uint8_t rx_err_cnt;  /*!< Recieve Error Counter. */
   uint8_t errors;      /*!< Various error bits See FLEXCAN docs. */
   uint8_t tx_wrn;      /*!< Transmit Warning Error (TX_CNT > 96). */
   uint8_t rx_wrn;      /*!< Recieve Warning Error (RX_CNT > 96). */
   uint8_t flt_conf;    /*!< 00 - Error Active 01 - Error Passive  1X- Bus Off. */
} FLEXCAN_status_t;


/** An enum for FLEXCAN transmit Options.
 *
 */
typedef enum tx_option
{
   TX_ONCE,             /*!< Transmits Message only once (Aborts after any retry). */
   TX_BEST_EFFORT,      /*!< Transmits until bus_off error state is acchieved. */
   TX_UNTIL_TX_WARN,    /*!< Transmits until TX Error Count >= 96 */
   TX_UNTIL_ERR_PASSIVE /*!< Transmits until the controller is in error passive state. */

}FLEXCAN_tx_option_t;

typedef void (*FLEXCAN_rx_callback)(FLEXCAN_frame_t *);
typedef void (*FLEXCAN_tx_callback)(FLEXCAN_frame_t *);
typedef void (*FLEXCAN_fifo_callback)(FLEXCAN_frame_t *);
typedef void (*FLEXCAN_callback_t)(uint8_t mb );



// TODO: Warning Interrupts
// TODO: Status struct.
// TODO: Bus off interrupt

/** Sets a Filter for the FIFO.
 * @param n The number of the filter slot to use. (Default 0-7).
 * @param filter see FLEXCAN_filter_a, FLEXCAN_filter_b, FLEXCAN_filter_c.
 * @param mask
 */
int FLEXCAN_set_fifo_filter(uint8_t n, uint32_t filter, uint32_t mask);

/** @description Forms a filter mask in format A. 
 *  @param rtr 
 *  @param ide
 *  @param ext_id Full 29-Bit identifier.
 *  @return Properly formated acceptance filter.
 */
uint32_t FLEXCAN_filter_a(uint8_t rtr, uint8_t ide, uint32_t ext_id);

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
                           uint16_t id_b);

/** Forms a filter mask in format B. Filters on 4 (upper) 8-bit ID segments. 
 * @param id Array of 8-Bit ID slices. 
 * @param len Length of the ID array (MAX - 4).
 * @return Properly formatted acceptance filter. 
 */
uint32_t FLEXCAN_filter_c(uint8_t * id, uint8_t len);

/** Initiallized the FLEXCAN hardware with the specified baud.
 * @param config 
 * @see FLEXCAN_config_t
 * @return FLEXCAN_SUCCESS or FLEXCAN_ERROR
 */
int FLEXCAN_init(FLEXCAN_config_t config);

/** Deitilize the CAN hardware.
 * @return FLEXCAN_SUCCESS or FLEXCAN_ERROR
 */
int FLEXCAN_deinit(void);

/** Reads a frame from a specified mailbox.
 * @param mb The mailbox to read from.
 * @param frame Pointer to a struct to copy data into.
 * @return FLEXCAN_SUCCESS or FLEXCAN_ERROR.
 */
int FLEXCAN_read_frame(uint8_t mb, FLEXCAN_frame_t *frame);

/** Writes to a mailbox.
 * @param mb Number of the mailbox to write to [8 - 63] avalible by default.
 * @param code Mailbox Setup code.  
 * @param frame The frame to write to the mailbox. 
 * @return FLEXCAN_SUCCESS or FLEXCAN_ERROR.
 */
int FLEXCAN_mb_write(uint8_t mb, uint8_t code, FLEXCAN_frame_t frame);

/** Reads a mailbox.
 * @param mb Number of the mailbox to read [8-63] avalible by default.
 * @param code FLEXCAN Mailbox C/S Code to write into mailbox.
 * @param frame Pointer to frame to copy data into.
 * @return FLEXCAN_SUCCESS or FLEXCAN_ERROR.
 */
int FLEXCAN_mb_read(uint8_t mb, uint8_t * code, uint16_t *timestamp, FLEXCAN_frame_t* frame);

/** Register a callback for when a RX mailbox is filled.
 * @param mb Number of the mailbox to register.
 * @param cb Callback function to register.
 * @return FLEXCAN_SUCCESS or FLEXCAN_ERROR.
 */
int FLEXCAN_mb_reg_callback(uint8_t mb, FLEXCAN_callback_t cb);

/** Disable callback for the specific mailbox.
 * @param mb The Mailbox to disable.
 * @return FLEXCAN_SUCCESS or FLEXCAN_ERROR.
 */
int FLEXCAN_mb_unreg_callback(uint8_t mb);

/** Register callback function for FIFO half full.
 * @param cb Callback Function.
 * @return FLEXCAN_SUCCESS or FLEXCAN_ERROR.
 */
int FLEXCAN_fifo_reg_callback(FLEXCAN_callback_t cb);

/** Disable the callback for the FIFO.
 * @return FLEXCAN_SUCCESS or FLEXCAN_ERROR.
 */
int FLEXCAN_fifo_unreg_callback(void);

/** Checks to see if there are any messages in the FIFO.
 * @return The number of messages in the FIFO.
 */
int FLEXCAN_fifo_avalible();

/** Read a message from the fifo.
 * @param frame Pointer to the frame to copy the data into.
 * @return FLEXCAN_SUCCESS or FLEXCAN_ERROR.
 */
int FLEXCAN_fifo_read(FLEXCAN_frame_t * frame);

/** Attempts to abort transmission for a selected Mailbox.
 * @param The mailbox to try and abort.
 * @return FLEXCAN_SUCCESS If the mailbox was aborted, FLEXCAN_EROR otherwise.
 */
int FLEXCAN_abort_mb(uint8_t mb);

/** Writes a message over FLEXCAN Hardware.
 * @param frame 
 * @param option
 * @return FLEXCAN_TX_SUCCESS or FLEXCAN_TX_ABORTED.
 */
int FLEXCAN_write(FLEXCAN_frame_t frame, FLEXCAN_tx_option_t option);

/** Gets detailed FLEXCAN and writes back to pointer to status struct.
 * @param The pointer to store the status information.
 * @return FLEXCAN_SUCCESS or FLEXCAN_ERROR.
 */
int FLEXCAN_status(FLEXCAN_status_t * status);

/** Resets FLEXCAN Hardware.
 * @return FLEXCAN_SUCCESS or FLEXCAN_ERROR.
 */
int FLEXCAN_reset(void);

#endif 

