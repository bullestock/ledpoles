/*
 * Neomatrix - Copyright (C) 2017 Lars Christensen
 * MIT Licensed
 *
 * W2812B I2S Driver
 *
 * Copyright 2015 <>< Charles Lohr, See LICENSE file.
 * WS2812 sender that abuses the I2S interface on the WS2812.
 * https://github.com/cnlohr/esp8266ws2812i2s
 */

#include "eagle_soc.h"
#include "slc_register.h"
#include "ws2812_i2s.hpp"
#include <stdint.h>
#include <string.h>
#include <Arduino.h>
#include <FastLED.h>

#define DIVCEIL(N, M) (((N)+((M)-1))/(M))
#define ROUNDUP(N, M) (DIVCEIL(N, M)*(M))

#if defined(WS2812_FOUR_SAMPLE) || defined(SK6812)
// Any number of pixels will give a multiple of 4 bytes output
#define PIXEL_ROUND(N) (N)
#elif defined(WS2812_THREE_SAMPLE)
// Only multiples of 4 pixels will give a multiple of 4 bytes output
#define PIXEL_ROUND(N) ROUNDUP(N, 4)
#endif

#ifdef WS2812_THREE_SAMPLE
#define WS_SAMPLESPERBIT 3
#endif

#ifdef WS2812_FOUR_SAMPLE
#define WS_SAMPLESPERBIT 4
#endif

#define WS_BYTESPERPIXEL (3 * WS_SAMPLESPERBIT)
#define WS_TOTALPIXELS PIXEL_ROUND(WS_PIXELS)
#define WS_DATABYTES (WS_TOTALPIXELS * WS_BYTESPERPIXEL)
#define WS_ZEROBYTES 32
#define WS_TOTALBYTES (WS_DATABYTES + WS_ZEROBYTES)
#define WS_MAXBLOCKSIZE 4092

#define WS_BLOCKCOUNT DIVCEIL(WS_TOTALBYTES, WS_MAXBLOCKSIZE)

static_assert(WS_TOTALBYTES % 4 == 0, "Buffers size must be multiple of 4");

extern "C" void rom_i2c_writeReg_Mask(uint8_t, int, int, int, int, int);

// Creates an I2S SR of 93,750 Hz, or 3 MHz Bitclock (.333us/sample)
// 1600000000L/(div*bestbck)
// It is likely you could speed this up a little.

#ifdef WS2812_THREE_SAMPLE
#define WS_I2S_BCK 22  // Seems to work as low as 19, but is shakey at 18.
#define WS_I2S_DIV 4
#elif defined( WS2812_FOUR_SAMPLE ) || defined(SK6812)
#define WS_I2S_BCK 17  // Seems to work as low as 14, shoddy at 13.
#define WS_I2S_DIV 4
#else
#error You need to either define WS2812_THREE_SAMPLE, WS2812_FOUR_SAMPLE or SK6812
#endif

#ifndef i2c_bbpll
#define i2c_bbpll 0x67
#define i2c_bbpll_en_audio_clock_out 4
#define i2c_bbpll_en_audio_clock_out_msb 7
#define i2c_bbpll_en_audio_clock_out_lsb 7
#define i2c_bbpll_hostid 4

#define i2c_writeReg_Mask(block, host_id, reg_add, Msb, Lsb, indata) \
  rom_i2c_writeReg_Mask(block, host_id, reg_add, Msb, Lsb, indata)
#define i2c_readReg_Mask(block, host_id, reg_add, Msb, Lsb) \
  rom_i2c_readReg_Mask(block, host_id, reg_add, Msb, Lsb)
#define i2c_writeReg_Mask_def(block, reg_add, indata) \
  i2c_writeReg_Mask(block, block##_hostid, reg_add, reg_add##_msb, reg_add##_lsb, indata)
#define i2c_readReg_Mask_def(block, reg_add) \
  i2c_readReg_Mask(block, block##_hostid, reg_add, reg_add##_msb, reg_add##_lsb)
#endif

#ifndef ETS_SLC_INUM
#define ETS_SLC_INUM       1
#endif

// From i2s_reg.h
#define DR_REG_I2S_BASE (0x60000e00)

#define I2STXFIFO (DR_REG_I2S_BASE + 0x0000)
#define I2SRXFIFO (DR_REG_I2S_BASE + 0x0004)
#define I2SCONF (DR_REG_I2S_BASE + 0x0008)
#define I2S_BCK_DIV_NUM 0x0000003F
#define I2S_BCK_DIV_NUM_S 22
#define I2S_CLKM_DIV_NUM 0x0000003F
#define I2S_CLKM_DIV_NUM_S 16
#define I2S_BITS_MOD 0x0000000F
#define I2S_BITS_MOD_S 12
#define I2S_RECE_MSB_SHIFT (BIT(11))
#define I2S_TRANS_MSB_SHIFT (BIT(10))
#define I2S_I2S_RX_START (BIT(9))
#define I2S_I2S_TX_START (BIT(8))
#define I2S_MSB_RIGHT (BIT(7))
#define I2S_RIGHT_FIRST (BIT(6))
#define I2S_RECE_SLAVE_MOD (BIT(5))
#define I2S_TRANS_SLAVE_MOD (BIT(4))
#define I2S_I2S_RX_FIFO_RESET (BIT(3))
#define I2S_I2S_TX_FIFO_RESET (BIT(2))
#define I2S_I2S_RX_RESET (BIT(1))
#define I2S_I2S_TX_RESET (BIT(0))
#define I2S_I2S_RESET_MASK 0xf

#define I2SINT_RAW (DR_REG_I2S_BASE + 0x000c)
#define I2S_I2S_TX_REMPTY_INT_RAW (BIT(5))
#define I2S_I2S_TX_WFULL_INT_RAW (BIT(4))
#define I2S_I2S_RX_REMPTY_INT_RAW (BIT(3))
#define I2S_I2S_RX_WFULL_INT_RAW (BIT(2))
#define I2S_I2S_TX_PUT_DATA_INT_RAW (BIT(1))
#define I2S_I2S_RX_TAKE_DATA_INT_RAW (BIT(0))

#define I2SINT_ST (DR_REG_I2S_BASE + 0x0010)
#define I2S_I2S_TX_REMPTY_INT_ST (BIT(5))
#define I2S_I2S_TX_WFULL_INT_ST (BIT(4))
#define I2S_I2S_RX_REMPTY_INT_ST (BIT(3))
#define I2S_I2S_RX_WFULL_INT_ST (BIT(2))
#define I2S_I2S_TX_PUT_DATA_INT_ST (BIT(1))
#define I2S_I2S_RX_TAKE_DATA_INT_ST (BIT(0))

#define I2SINT_ENA (DR_REG_I2S_BASE + 0x0014)
#define I2S_I2S_TX_REMPTY_INT_ENA (BIT(5))
#define I2S_I2S_TX_WFULL_INT_ENA (BIT(4))
#define I2S_I2S_RX_REMPTY_INT_ENA (BIT(3))
#define I2S_I2S_RX_WFULL_INT_ENA (BIT(2))
#define I2S_I2S_TX_PUT_DATA_INT_ENA (BIT(1))
#define I2S_I2S_RX_TAKE_DATA_INT_ENA (BIT(0))

#define I2SINT_CLR (DR_REG_I2S_BASE + 0x0018)
#define I2S_I2S_TX_REMPTY_INT_CLR (BIT(5))
#define I2S_I2S_TX_WFULL_INT_CLR (BIT(4))
#define I2S_I2S_RX_REMPTY_INT_CLR (BIT(3))
#define I2S_I2S_RX_WFULL_INT_CLR (BIT(2))
#define I2S_I2S_PUT_DATA_INT_CLR (BIT(1))
#define I2S_I2S_TAKE_DATA_INT_CLR (BIT(0))

#define I2STIMING (DR_REG_I2S_BASE + 0x001c)
#define I2S_TRANS_BCK_IN_INV (BIT(22))
#define I2S_RECE_DSYNC_SW (BIT(21))
#define I2S_TRANS_DSYNC_SW (BIT(20))
#define I2S_RECE_BCK_OUT_DELAY 0x00000003
#define I2S_RECE_BCK_OUT_DELAY_S 18
#define I2S_RECE_WS_OUT_DELAY 0x00000003
#define I2S_RECE_WS_OUT_DELAY_S 16
#define I2S_TRANS_SD_OUT_DELAY 0x00000003
#define I2S_TRANS_SD_OUT_DELAY_S 14
#define I2S_TRANS_WS_OUT_DELAY 0x00000003
#define I2S_TRANS_WS_OUT_DELAY_S 12
#define I2S_TRANS_BCK_OUT_DELAY 0x00000003
#define I2S_TRANS_BCK_OUT_DELAY_S 10
#define I2S_RECE_SD_IN_DELAY 0x00000003
#define I2S_RECE_SD_IN_DELAY_S 8
#define I2S_RECE_WS_IN_DELAY 0x00000003
#define I2S_RECE_WS_IN_DELAY_S 6
#define I2S_RECE_BCK_IN_DELAY 0x00000003
#define I2S_RECE_BCK_IN_DELAY_S 4
#define I2S_TRANS_WS_IN_DELAY 0x00000003
#define I2S_TRANS_WS_IN_DELAY_S 2
#define I2S_TRANS_BCK_IN_DELAY 0x00000003
#define I2S_TRANS_BCK_IN_DELAY_S 0

#define I2S_FIFO_CONF (DR_REG_I2S_BASE + 0x0020)
#define I2S_I2S_RX_FIFO_MOD 0x00000007
#define I2S_I2S_RX_FIFO_MOD_S 16
#define I2S_I2S_TX_FIFO_MOD 0x00000007
#define I2S_I2S_TX_FIFO_MOD_S 13
#define I2S_I2S_DSCR_EN (BIT(12))
#define I2S_I2S_TX_DATA_NUM 0x0000003F
#define I2S_I2S_TX_DATA_NUM_S 6
#define I2S_I2S_RX_DATA_NUM 0x0000003F
#define I2S_I2S_RX_DATA_NUM_S 0

#define I2SRXEOF_NUM (DR_REG_I2S_BASE + 0x0024)
#define I2S_I2S_RX_EOF_NUM 0xFFFFFFFF
#define I2S_I2S_RX_EOF_NUM_S 0

#define I2SCONF_SIGLE_DATA (DR_REG_I2S_BASE + 0x0028)
#define I2S_I2S_SIGLE_DATA 0xFFFFFFFF
#define I2S_I2S_SIGLE_DATA_S 0

#define I2SCONF_CHAN (DR_REG_I2S_BASE + 0x002c)
#define I2S_RX_CHAN_MOD 0x00000003
#define I2S_RX_CHAN_MOD_S 3
#define I2S_TX_CHAN_MOD 0x00000007
#define I2S_TX_CHAN_MOD_S 0

// From sdio_slv.h

struct sdio_queue {
  uint32_t blocksize : 12;
  uint32_t datalen : 12;
  uint32_t unused : 5;
  uint32_t sub_sof : 1;
  uint32_t eof : 1;
  uint32_t owner : 1;
  uint32_t buf_ptr;
  uint32_t next_link_ptr;
};

struct sdio_slave_status_element {
  uint32_t wr_busy : 1;
  uint32_t rd_empty : 1;
  uint32_t comm_cnt : 3;
  uint32_t intr_no : 3;
  uint32_t rx_length : 16;
  uint32_t res : 8;
};

union sdio_slave_status {
  struct sdio_slave_status_element elm_value;
  uint32_t word_value;
};

#define RX_AVAILIBLE 2
#define TX_AVAILIBLE 1
#define INIT_STAGE 0

#define SDIO_QUEUE_LEN 8
#define MOSI 0
#define MISO 1
#define SDIO_DATA_ERROR 6

#define SLC_INTEREST_EVENT (SLC_TX_EOF_INT_ENA | SLC_RX_EOF_INT_ENA | SLC_RX_UDF_INT_ENA | SLC_TX_DSCR_ERR_INT_ENA)
#define TRIG_TOHOST_INT() SET_PERI_REG_MASK(SLC_INTVEC_TOHOST , BIT0); \
  CLEAR_PERI_REG_MASK(SLC_INTVEC_TOHOST , BIT0)

static struct sdio_queue i2sBufDescOut[WS_BLOCKCOUNT];

static uint32_t* i2sBlock;

static uint16_t brightness = 256;

// Initialize I2S subsystem for DMA circular buffer use
void ICACHE_FLASH_ATTR ws2812_init()
{
  // Reset DMA
  SET_PERI_REG_MASK(SLC_CONF0, SLC_RXLINK_RST);
  CLEAR_PERI_REG_MASK(SLC_CONF0, SLC_RXLINK_RST);

  // Clear DMA int flags
  SET_PERI_REG_MASK(SLC_INT_CLR, 0xffffffff);
  CLEAR_PERI_REG_MASK(SLC_INT_CLR, 0xffffffff);

  // Enable and configure DMA
  CLEAR_PERI_REG_MASK(SLC_CONF0, (SLC_MODE << SLC_MODE_S));
  SET_PERI_REG_MASK(SLC_CONF0, (1 << SLC_MODE_S));
  SET_PERI_REG_MASK(SLC_RX_DSCR_CONF, SLC_INFOR_NO_REPLACE | SLC_TOKEN_NO_REPLACE);
  CLEAR_PERI_REG_MASK(SLC_RX_DSCR_CONF, SLC_RX_FILL_EN | SLC_RX_EOF_MODE | SLC_RX_FILL_MODE);

  i2sBlock = new uint32_t[WS_TOTALBYTES / 4];

  for (int i = 0; i < WS_BLOCKCOUNT; ++i) {
    i2sBufDescOut[i].owner = 1;
    i2sBufDescOut[i].eof = 1;
    i2sBufDescOut[i].sub_sof = 0;
    i2sBufDescOut[i].datalen = min(WS_MAXBLOCKSIZE, WS_TOTALBYTES - i * WS_MAXBLOCKSIZE);
    i2sBufDescOut[i].blocksize = min(WS_MAXBLOCKSIZE, WS_TOTALBYTES - i * WS_MAXBLOCKSIZE);
    i2sBufDescOut[i].buf_ptr = (uint32_t)&i2sBlock[i * WS_MAXBLOCKSIZE / 4];
    i2sBufDescOut[i].unused = 0;
    i2sBufDescOut[i].next_link_ptr = (uint32_t)&i2sBufDescOut[(i + 1) % WS_BLOCKCOUNT];
  }

  memset(i2sBlock, 0, WS_TOTALBYTES);

  CLEAR_PERI_REG_MASK(SLC_RX_LINK, SLC_RXLINK_DESCADDR_MASK);
  SET_PERI_REG_MASK(SLC_RX_LINK, ((uint32_t)&i2sBufDescOut) & SLC_RXLINK_DESCADDR_MASK);

  // Start transmission
  SET_PERI_REG_MASK(SLC_RX_LINK, SLC_RXLINK_START);

  // Init pins to i2s functions
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0RXD_U, FUNC_I2SO_DATA);

  // Enable clock to i2s subsystem
  i2c_writeReg_Mask_def(i2c_bbpll, i2c_bbpll_en_audio_clock_out, 1);

  // Reset I2S subsystem
  CLEAR_PERI_REG_MASK(I2SCONF, I2S_I2S_RESET_MASK);
  SET_PERI_REG_MASK(I2SCONF, I2S_I2S_RESET_MASK);
  CLEAR_PERI_REG_MASK(I2SCONF, I2S_I2S_RESET_MASK);

  // Select 16bits per channel (FIFO_MOD=0), no DMA access (FIFO only)
  CLEAR_PERI_REG_MASK(I2S_FIFO_CONF, I2S_I2S_DSCR_EN | (I2S_I2S_RX_FIFO_MOD << I2S_I2S_RX_FIFO_MOD_S) | (I2S_I2S_TX_FIFO_MOD << I2S_I2S_TX_FIFO_MOD_S));
  // Enable DMA in i2s subsystem
  SET_PERI_REG_MASK(I2S_FIFO_CONF, I2S_I2S_DSCR_EN);

  // trans master&rece slave,MSB shift,right_first,msb right
  CLEAR_PERI_REG_MASK(I2SCONF, I2S_TRANS_SLAVE_MOD |
                      (I2S_BITS_MOD << I2S_BITS_MOD_S) |
                      (I2S_BCK_DIV_NUM << I2S_BCK_DIV_NUM_S) |
                      (I2S_CLKM_DIV_NUM << I2S_CLKM_DIV_NUM_S));
  SET_PERI_REG_MASK(I2SCONF, I2S_RIGHT_FIRST | I2S_MSB_RIGHT | I2S_RECE_SLAVE_MOD |
                    I2S_RECE_MSB_SHIFT | I2S_TRANS_MSB_SHIFT |
                    (((WS_I2S_BCK - 1)&I2S_BCK_DIV_NUM) << I2S_BCK_DIV_NUM_S) |
                    (((WS_I2S_DIV - 1)&I2S_CLKM_DIV_NUM) << I2S_CLKM_DIV_NUM_S));

  // No idea if ints are needed...
  // clear int
  SET_PERI_REG_MASK(I2SINT_CLR, I2S_I2S_RX_WFULL_INT_CLR | I2S_I2S_PUT_DATA_INT_CLR | I2S_I2S_TAKE_DATA_INT_CLR);
  CLEAR_PERI_REG_MASK(I2SINT_CLR, I2S_I2S_RX_WFULL_INT_CLR | I2S_I2S_PUT_DATA_INT_CLR | I2S_I2S_TAKE_DATA_INT_CLR);
  // enable int
  SET_PERI_REG_MASK(I2SINT_ENA,  I2S_I2S_RX_REMPTY_INT_ENA | I2S_I2S_RX_TAKE_DATA_INT_ENA);

  // Start transmission
  SET_PERI_REG_MASK(I2SCONF, I2S_I2S_TX_START);
}

#ifdef WS2812_THREE_SAMPLE

static const uint16_t bitpatterns[16] = {
  0b100100100100, 0b100100100110, 0b100100110100, 0b100100110110,
  0b100110100100, 0b100110100110, 0b100110110100, 0b100110110110,
  0b110100100100, 0b110100100110, 0b110100110100, 0b110100110110,
  0b110110100100, 0b110110100110, 0b110110110100, 0b110110110110,
};

#elif defined(WS2812_FOUR_SAMPLE)

// Tricky, send out WS2812 bits with coded pulses, one nibble, then the other.
static const uint16_t bitpatterns[16] = {
  0b1000100010001000, 0b1000100010001110, 0b1000100011101000, 0b1000100011101110,
  0b1000111010001000, 0b1000111010001110, 0b1000111011101000, 0b1000111011101110,
  0b1110100010001000, 0b1110100010001110, 0b1110100011101000, 0b1110100011101110,
  0b1110111010001000, 0b1110111010001110, 0b1110111011101000, 0b1110111011101110,
};

#elif defined(SK6812)

// SK6812 has different timing for '1' bits, which requires 4bit samples
static const uint16_t bitpatterns[16] = {
  0b1000100010001000, 0b1000100010001100, 0b1000100011001000, 0b1000100011001100,
  0b1000110010001000, 0b1000110010001100, 0b1000110011001000, 0b1000110011001100,
  0b1100100010001000, 0b1100100010001100, 0b1100100011001000, 0b1100100011001100,
  0b1100110010001000, 0b1100110010001100, 0b1100110011001000, 0b1100110011001100,
};

#endif

void ws2812_show(const CRGB data[])
{
  uint8_t* buffer = (uint8_t*)&data[0];
  uint16_t buffersize = 3 * WS_PIXELS;
  buffersize = min(buffersize, uint16_t(WS_DATABYTES / WS_SAMPLESPERBIT));
  uint16_t bright = brightness;

  const uint8_t* buf = buffer;
  const uint8_t* const end = buffer + buffersize;

#ifdef WS2812_THREE_SAMPLE
  uint8_t *bufferpl = (uint8_t *)&i2sBlock[0];

  while (buf < end) {
    uint8_t b1 = ((*buf++) * bright) >> 8;
    uint16_t c1a = bitpatterns[b1 & 0x0f];
    uint16_t c1b = bitpatterns[b1 >> 4];
    uint8_t b2 = ((*buf++) * bright) >> 8;
    uint16_t c2a = bitpatterns[b2 & 0x0f];
    uint16_t c2b = bitpatterns[b2 >> 4];
    uint8_t b3 = ((*buf++) * bright) >> 8;
    uint16_t c3a = bitpatterns[b3 & 0x0f];
    uint16_t c3b = bitpatterns[b3 >> 4];
    uint8_t b4 = ((*buf++) * bright) >> 8;
    uint16_t c4a = bitpatterns[b4 & 0x0f];
    uint16_t c4b = bitpatterns[b4 >> 4];

#define STEP1(x) (c##x##b >> 4)
#define STEP2(x) ((c##x##b << 4) | (c##x##a>>8))
#define STEP3(x) (c##x##a & 0xff)

    *bufferpl++ = STEP1(2);
    *bufferpl++ = STEP3(1);
    *bufferpl++ = STEP2(1);
    *bufferpl++ = STEP1(1);

    *bufferpl++ = STEP2(3);
    *bufferpl++ = STEP1(3);
    *bufferpl++ = STEP3(2);
    *bufferpl++ = STEP2(2);

    *bufferpl++ = STEP3(4);
    *bufferpl++ = STEP2(4);
    *bufferpl++ = STEP1(4);
    *bufferpl++ = STEP3(3);
  }

  const uint8_t* bufend = &((uint8_t*)i2sBlock)[WS_DATABYTES];
  while (bufferpl < bufend) {
    *bufferpl++ = 0;
  }

#elif defined(WS2812_FOUR_SAMPLE) || defined(SK6812)
  uint16_t *bufferpl = (uint16_t *)&i2sBlock[0];
  while (buf < end) {
    uint8_t btosend = *buf++;
    *bufferpl++ = bitpatterns[btosend & 0x0f];
    *bufferpl++ = bitpatterns[btosend >> 4];
  }
#endif
}

void ws2812_brightness(int b)
{
  brightness = b;
}
