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

#include "rtl.hpp"
#include <stdio.hpp>
#include <UserCalls.hpp>

u8 enetaddr[6];
int ioaddr;
unsigned int cur_rx,cur_tx;

static int read_eeprom(int location, int addr_len);
#define le16_to_cpu(x) ((u16)(x))

/* The RTL8139 can only transmit from a contiguous, aligned memory block.  */
static unsigned char tx_buffer[TX_BUF_SIZE] __attribute__((aligned(4)));
static unsigned char rx_ring[RX_BUF_LEN+16] __attribute__((aligned(4)));

struct pci_device_id {
	unsigned int vendor, device, subvendor, subdevice, klass, class_mask;
	unsigned long driver_data;
};

static struct pci_device_id supported[] = {
       {0x10EC, 0x8139},
       {0x1186, 0x1300},
       {}
};

int rtl8139_probe(int _ioaddr) {
	int i;
	int speed10, fullduplex;
	int addr_len;
	unsigned short *ap = (unsigned short *)enetaddr;

	ioaddr = _ioaddr;

	/* Bring the chip out of low-power mode. */
	AkariOutB(ioaddr + Config1, 0x00);

	addr_len = read_eeprom(0,8) == 0x8129 ? 8 : 6;
	for (i = 0; i < 3; i++)
		*ap++ = le16_to_cpu (read_eeprom(i + 7, addr_len));

	speed10 = AkariInB(ioaddr + MediaStatus) & MSRSpeed10;
	fullduplex = AkariInW(ioaddr + MII_BMCR) & BMCRDuplex;

	rtl_reset();

	if (AkariInB(ioaddr + MediaStatus) & MSRLinkFail) {
		printf("Cable not connected or other link failure\n");
		return -1 ;
	}

	return 0;
}

static int read_eeprom(int location, int addr_len)
{
	int i;
	unsigned int retval = 0;
	long ee_addr = ioaddr + Cfg9346;
	int read_cmd = location | (EE_READ_CMD << addr_len);

	AkariOutB(ee_addr, EE_ENB & ~EE_CS);
	AkariOutB(ee_addr, EE_ENB);
	eeprom_delay();

	/* Shift the read command bits out. */
	for (i = 4 + addr_len; i >= 0; i--) {
		int dataval = (read_cmd & (1 << i)) ? EE_DATA_WRITE : 0;
		AkariOutB(ee_addr, EE_ENB | dataval);
		eeprom_delay();
		AkariOutB(ee_addr, EE_ENB | dataval | EE_SHIFT_CLK);
		eeprom_delay();
	}
	AkariOutB(ee_addr, EE_ENB);
	eeprom_delay();

	for (i = 16; i > 0; i--) {
		AkariOutB(ee_addr, EE_ENB | EE_SHIFT_CLK);
		eeprom_delay();
		retval = (retval << 1) | ((AkariInB(ee_addr) & EE_DATA_READ) ? 1 : 0);
		AkariOutB(ee_addr, EE_ENB);
		eeprom_delay();
	}

	/* Terminate the EEPROM access. */
	AkariOutB(ee_addr, ~EE_CS);
	eeprom_delay();
	return retval;
}

static const unsigned int rtl8139_rx_config =
	(RX_BUF_LEN_IDX << 11) |
	(RX_FIFO_THRESH << 13) |
	(RX_DMA_BURST << 8);

static void set_rx_mode() {
	unsigned int mc_filter[2];
	int rx_mode;
	/* !IFF_PROMISC */
	rx_mode = AcceptBroadcast | AcceptMulticast | AcceptMyPhys;
	mc_filter[1] = mc_filter[0] = 0xffffffff;

	AkariOutL(ioaddr + RxConfig, rtl8139_rx_config | rx_mode);

	AkariOutL(ioaddr + MAR0 + 0, mc_filter[0]);
	AkariOutL(ioaddr + MAR0 + 4, mc_filter[1]);
}

void rtl_reset()
{
	int i;

	AkariOutB(ioaddr + ChipCmd, CmdReset);

	cur_rx = 0;
	cur_tx = 0;

	/* Give the chip 10ms to finish the reset. */
	for (i=0; i<100; ++i){
		if ((AkariInB(ioaddr + ChipCmd) & CmdReset) == 0) break;
		// TODO udelay (100); /* wait 100us */
	}


	for (i = 0; i < ETH_ALEN; i++)
		AkariOutB(ioaddr + MAC0 + i, enetaddr[i]);

	/* Must enable Tx/Rx before setting transfer thresholds! */
	AkariOutB(ioaddr + ChipCmd, CmdRxEnb | CmdTxEnb);
	AkariOutL(ioaddr + RxConfig,
			(RX_FIFO_THRESH<<13) | (RX_BUF_LEN_IDX<<11) | (RX_DMA_BURST<<8)); /* accept no frames yet!  */
	AkariOutL(ioaddr + TxConfig, (TX_DMA_BURST<<8)|0x03000000);

	/* The Linux driver changes Config1 here to use a different LED pattern
	 * for half duplex or full/autodetect duplex (for full/autodetect, the
	 * outputs are TX/RX, Link10/100, FULL, while for half duplex it uses
	 * TX/RX, Link100, Link10).  This is messy, because it doesn't match
	 * the inscription on the mounting bracket.  It should not be changed
	 * from the configuration EEPROM default, because the card manufacturer
	 * should have set that to match the card.  */

	printf("rx ring address is %X\n",(unsigned long)rx_ring);
	AkariOutL(ioaddr + RxBuf, phys_to_bus((int)rx_ring));

	/* If we add multicast support, the MAR0 register would have to be
	 * initialized to 0xffffffffffffffff (two 32 bit accesses).  Etherboot
	 * only needs broadcast (for ARP/RARP/BOOTP/DHCP) and unicast.	*/

	AkariOutB(ioaddr + ChipCmd, CmdRxEnb | CmdTxEnb);

	AkariOutL(ioaddr + RxConfig, rtl8139_rx_config);

	/* Start the chip's Tx and Rx process. */
	AkariOutL(ioaddr + RxMissed, 0);

	/* set_rx_mode */
	set_rx_mode();

	/* Disable all known interrupts by setting the interrupt mask. */
	AkariOutW(ioaddr + IntrMask, 0);
}

int rtl_transmit(volatile void *packet, int length)
{
	unsigned int status, to;
	unsigned long txstatus;
	unsigned int len = length;

	memcpy((char *)tx_buffer, (char *)packet, (int)length);

	printf("sending %d bytes\n", len);

	/* Note: RTL8139 doesn't auto-pad, send minimum payload (another 4
	 * bytes are sent automatically for the FCS, totalling to 64 bytes). */
	while (len < ETH_ZLEN) {
		tx_buffer[len++] = '\0';
	}

	AkariOutL(ioaddr + TxAddr0 + cur_tx*4, phys_to_bus((int)tx_buffer));
	AkariOutL(ioaddr + TxStatus0 + cur_tx*4,
			((TX_FIFO_THRESH<<11) & 0x003f0000) | len);

	to = ticks() + RTL_TIMEOUT;

	do {
		status = AkariInW(ioaddr + IntrStatus);
		/* Only acknlowledge interrupt sources we can properly handle
		 * here - the RxOverflow/RxFIFOOver MUST be handled in the
		 * rtl_poll() function.	 */
		AkariOutW(ioaddr + IntrStatus, status & (TxOK | TxErr | PCIErr));
		if ((status & (TxOK | TxErr | PCIErr)) != 0) break;
	} while (ticks() < to);

	txstatus = AkariInL(ioaddr + TxStatus0 + cur_tx*4);

	if (status & TxOK) {
		cur_tx = (cur_tx + 1) % NUM_TX_DESC;
		printf("tx done (%d ticks), status %hX txstatus %X\n",
			to-ticks(), status, txstatus);
		return length;
	} else {
		printf("tx timeout/error (%d ticks), status %hX txstatus %X\n",
			ticks()-to, status, txstatus);
		rtl_reset();

		return 0;
	}
}

int rtl_poll()
{
	unsigned int status;
	unsigned int ring_offs;
	unsigned int rx_size, rx_status;
	int length=0;

	if (AkariInB(ioaddr + ChipCmd) & RxBufEmpty) {
		return 0;
	}

	status = AkariInW(ioaddr + IntrStatus);
	/* See below for the rest of the interrupt acknowledges.  */
	AkariOutW(ioaddr + IntrStatus, status & ~(RxFIFOOver | RxOverflow | RxOK));

	printf("rtl_poll: int %hX ", status);

	ring_offs = cur_rx % RX_BUF_LEN;
	rx_status = (u32)(rx_ring + ring_offs);
	rx_size = rx_status >> 16;
	rx_status &= 0xffff;

	if ((rx_status & (RxBadSymbol|RxRunt|RxTooLong|RxCRCErr|RxBadAlign)) ||
	    (rx_size < ETH_ZLEN) || (rx_size > ETH_FRAME_LEN + 4)) {
		printf("rx error %hX\n", rx_status);
		rtl_reset(); /* this clears all interrupts still pending */
		return 0;
	}

	/* Received a good packet */
	length = rx_size - 4;	/* no one cares about the FCS */
	if (ring_offs+4+rx_size-4 > RX_BUF_LEN) {
		int semi_count = RX_BUF_LEN - ring_offs - 4;
		unsigned char rxdata[RX_BUF_LEN];

		memcpy(rxdata, rx_ring + ring_offs + 4, semi_count);
		memcpy(&(rxdata[semi_count]), rx_ring, rx_size-4-semi_count);

		//NetReceive(rxdata, length);
		printf("rx packet %d+%d bytes", semi_count,rx_size-4-semi_count);
	} else {
		//NetReceive(rx_ring + ring_offs + 4, length);
		printf("rx packet %d bytes", rx_size-4);
	}

	cur_rx = (cur_rx + rx_size + 4 + 3) & ~3;
	AkariOutW(ioaddr + RxBufPtr, cur_rx - 16);
	/* See RTL8139 Programming Guide V0.1 for the official handling of
	 * Rx overflow situations.  The document itself contains basically no
	 * usable information, except for a few exception handling rules.  */
	AkariOutW(ioaddr + IntrStatus, status & (RxFIFOOver | RxOverflow | RxOK));
	return length;
}

void rtl_disable()
{
	int i;

	/* reset the chip */
	AkariOutB(ioaddr + ChipCmd, CmdReset);

	/* Give the chip 10ms to finish the reset. */
	for (i=0; i<100; ++i){
		if ((AkariInB(ioaddr + ChipCmd) & CmdReset) == 0) break;
		//udelay (100); /* wait 100us */
	}
}

