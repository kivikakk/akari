// This file is part of Akari.
// Copyright 2010 Arlen Cuss
// 
// Akari is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// Akari is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with Akari.  If not, see <http://www.gnu.org/licenses/>.

#ifndef RTL_HPP
#define RTL_HPP

// These defines were taken from GPL'd code from "sanos",
// which in turn was taken by code written by Donald Becker.

#define ETH_ZLEN  60 // Min. octets in frame sans FCS

// The user-configurable values

// Maximum events (Rx packets, etc.) to handle at each interrupt

static int max_interrupt_work = 20;

// Maximum number of multicast addresses to filter (vs. Rx-all-multicast).
// The RTL chips use a 64 element hash table based on the Ethernet CRC.  It
// is efficient to update the hardware filter, but recalculating the table
// for a long filter list is painful

static int multicast_filter_limit = 32;

// Operational parameters that are set at compile time

// Maximum size of the in-memory receive ring (smaller if no memory)

#define RX_BUF_LEN_IDX  2     // 0=8K, 1=16K, 2=32K, 3=64K

// Size of the Tx bounce buffers -- must be at least (mtu+14+4)

#define TX_BUF_SIZE 1536

// PCI Tuning Parameters
// Threshold is bytes transferred to chip before transmission starts

#define TX_FIFO_THRESH 256  // In bytes, rounded down to 32 byte units

// The following settings are log_2(bytes)-4:  0 = 16 bytes .. 6 = 1024

#define RX_FIFO_THRESH  4   // Rx buffer level before first PCI xfer
#define RX_DMA_BURST    4   // Maximum PCI burst, '4' is 256 bytes
#define TX_DMA_BURST    4   // Calculate as 16 << val

// Operational parameters that usually are not changed
// Time in ticks before concluding the transmitter is hung

#define TX_TIMEOUT  (6*HZ)

// Allocation size of Rx buffers with full-sized Ethernet frames.
// This is a cross-driver value that is not a limit,
// but a way to keep a consistent allocation size among drivers.

#define PKT_BUF_SZ    1536

// Number of Tx descriptor registers

#define NUM_TX_DESC 4

// Symbolic offsets to registers

enum RTL8139_registers 
{
  MAC0             = 0x00,       // Ethernet hardware address
  MAR0             = 0x08,       // Multicast filter
  TxStatus0        = 0x10,       // Transmit status (Four 32bit registers)
  TxAddr0          = 0x20,       // Tx descriptors (also four 32bit)
  RxBuf            = 0x30, 
  RxEarlyCnt       = 0x34, 
  RxEarlyStatus    = 0x36,
  ChipCmd          = 0x37,
  RxBufPtr         = 0x38,
  RxBufAddr        = 0x3A,
  IntrMask         = 0x3C,
  IntrStatus       = 0x3E,
  TxConfig         = 0x40,
  RxConfig         = 0x44,
  Timer            = 0x48,        // A general-purpose counter
  RxMissed         = 0x4C,        // 24 bits valid, write clears
  Cfg9346          = 0x50, 
  Config0          = 0x51, 
  Config1          = 0x52,
  FlashReg         = 0x54, 
  GPPinData        = 0x58, 
  GPPinDir         = 0x59, 
  MII_SMI          = 0x5A, 
  HltClk           = 0x5B,
  MultiIntr        = 0x5C, 
  TxSummary        = 0x60,
  MII_BMCR         = 0x62, 
  MII_BMSR         = 0x64, 
  NWayAdvert       = 0x66, 
  NWayLPAR         = 0x68,
  NWayExpansion    = 0x6A,
  
  // Undocumented registers, but required for proper operation
  FIFOTMS          = 0x70,        // FIFO Control and test
  CSCR             = 0x74,        // Chip Status and Configuration Register
  PARA78           = 0x78, 
  PARA7c           = 0x7c,        // Magic transceiver parameter register
};

enum ChipCmdBits 
{
  RxBufEmpty = 0x01,
  CmdTxEnb   = 0x04,
  CmdRxEnb   = 0x08,
  CmdReset   = 0x10,
};

// Interrupt register bits

enum IntrStatusBits 
{
  RxOK       = 0x0001,
  RxErr      = 0x0002, 
  TxOK       = 0x0004, 
  TxErr      = 0x0008,
  RxOverflow = 0x0010,
  RxUnderrun = 0x0020, 
  RxFIFOOver = 0x0040, 
  PCSTimeout = 0x4000,
  PCIErr     = 0x8000, 
};

enum TxStatusBits 
{
  TxHostOwns    = 0x00002000,
  TxUnderrun    = 0x00004000,
  TxStatOK      = 0x00008000,
  TxOutOfWindow = 0x20000000,
  TxAborted     = 0x40000000,
  TxCarrierLost = 0x80000000,
};

enum RxStatusBits 
{
  RxStatusOK  = 0x0001,
  RxBadAlign  = 0x0002, 
  RxCRCErr    = 0x0004,
  RxTooLong   = 0x0008, 
  RxRunt      = 0x0010, 
  RxBadSymbol = 0x0020, 
  RxBroadcast = 0x2000,
  RxPhysical  = 0x4000, 
  RxMulticast = 0x8000, 
};

// Bits in RxConfig

enum RxConfigBits
{
  AcceptAllPhys   = 0x01,
  AcceptMyPhys    = 0x02, 
  AcceptMulticast = 0x04, 
  AcceptRunt      = 0x10, 
  AcceptErr       = 0x20, 
  AcceptBroadcast = 0x08,
};

enum CSCRBits 
{
  CSCR_LinkOKBit      = 0x00400, 
  CSCR_LinkDownOffCmd = 0x003c0,
  CSCR_LinkChangeBit  = 0x00800,
  CSCR_LinkStatusBits = 0x0f000, 
  CSCR_LinkDownCmd    = 0x0f3c0,
};

// Twister tuning parameters from RealTek.
// Completely undocumented, but required to tune bad links.

#define PARA78_default  0x78fa8388
#define PARA7c_default  0xcb38de43
#define PARA7c_xxx      0xcb38de43

unsigned long param[4][4] = 
{
  {0xcb39de43, 0xcb39ce43, 0xfb38de03, 0xcb38de43},
  {0xcb39de43, 0xcb39ce43, 0xcb39ce83, 0xcb39ce83},
  {0xcb39de43, 0xcb39ce43, 0xcb39ce83, 0xcb39ce83},
  {0xbb39de43, 0xbb39ce43, 0xbb39ce83, 0xbb39ce83}
};

struct nic 
{
  dev_t devno;                          // Device number
  struct dev *dev;                      // Device block

  unsigned short iobase;                // Configured I/O base
  unsigned short irq;                   // Configured IRQ

  struct interrupt intr;                // Interrupt object for driver
  struct dpc dpc;                       // DPC for driver
  struct timer timer;                   // Media selection timer

  struct eth_addr hwaddr;               // MAC address for NIC

  struct board *board;
  int flags;
  struct stats_nic stats;
  int msg_level;
  int max_interrupt_work;

  // Receive state
  unsigned char *rx_ring;
  unsigned int cur_rx;                  // Index into the Rx buffer of next Rx pkt.
  unsigned int rx_buf_len;              // Size (8K 16K 32K or 64KB) of the Rx ring

  // Transmit state
  struct sem tx_sem;                    // Semaphore for Tx ring not full
  unsigned int cur_tx;
  unsigned int dirty_tx;
  unsigned int tx_flag;
  struct pbuf *tx_pbuf[NUM_TX_DESC];    // The saved address of a sent-in-place packet
  unsigned char *tx_buf[NUM_TX_DESC];   // Tx bounce buffers
  unsigned char *tx_bufs;               // Tx bounce buffer region
  unsigned int trans_start;

  // Receive filter state
  unsigned int rx_config;
  unsigned long mc_filter[2];           // Multicast hash filter
  int cur_rx_mode;
  int multicast_filter_limit;

  // Transceiver state
  char phys[4];                         // MII device addresses
  unsigned short advertising;           // NWay media advertisement
  char twistie, twist_row, twist_col;   // Twister tune state
  unsigned char config1;
  unsigned char full_duplex;            // Full-duplex operation requested
  unsigned char duplex_lock;
  unsigned char link_speed;
  unsigned char media2;                 // Secondary monitored media port
  unsigned char medialock;              // Don't sense media type
  unsigned char mediasense;             // Media sensing in progress
};

#endif

