/*
 * AD9548 SPI Network Clock Generator/Synchronizer
 *
 * Copyright 2012 Analog Devices Inc.
 *
 * Licensed under the GPL-2.
 */

#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/spi/spi.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/delay.h>
#include <linux/io.h>

#include <linux/iio/iio.h>

#define AD_READ		(1 << 15)
#define AD_WRITE	(0 << 15)
#define AD_CNT(x)	(((x) - 1) << 13)
#define AD_ADDR(x)	((x) & 0xFFF)
#define WAIT_B		0xFFFF
#define CHIPID_AD9548	0x48

static const unsigned short ad9548_regs[][2] = {
	{0x0000, 0x30}, /* Reset */
	{0x0000, 0x10},
	{0x0000, 0x10},
	{0x0000, 0x10},
	{0x0000, 0x10},
	{0x0000, 0x10},
	{0x0100, 0x18}, /* System clock */
	{0x0101, 0x28},
	{0x0102, 0x45},
	{0x0103, 0x43},
	{0x0104, 0xDE},
	{0x0105, 0x13},
	{0x0106, 0x01},
	{0x0107, 0x00},
	{0x0108, 0x00},
	{0x0005, 0x01}, /* I/O Update */
	{0x0A02, 0x01}, /* Calibrate sysem clock */
	{0x0005, 0x01},
	{WAIT_B, 0x00},
	{0x0A02, 0x00},
	{0x0005, 0x01},
	{0x0208, 0x00}, /* IRQ Pin Output Mode */
	{0x0209, 0x00}, /* IRQ Masks */
	{0x020A, 0x00},
	{0x020B, 0x00},
	{0x020C, 0x00},
	{0x020D, 0x00},
	{0x020E, 0x00},
	{0x020F, 0x00},
	{0x0210, 0x00},
	{0x0211, 0x00}, /* Watchdog timer */
	{0x0212, 0x00},
	{0x0213, 0xFF}, /* Auxiliary DAC */
	{0x0214, 0x01},
	{0x0300, 0x29}, /* DPLL */
	{0x0301, 0x5C},
	{0x0302, 0x8F},
	{0x0303, 0xC2},
	{0x0304, 0xF5},
	{0x0305, 0x28},
	{0x0307, 0x00},
	{0x0308, 0x00},
	{0x0309, 0x00},
	{0x030A, 0xFF},
	{0x030B, 0xFF},
	{0x030C, 0xFF},
	{0x030D, 0x00},
	{0x030E, 0x00},
	{0x030F, 0x00},
	{0x0310, 0x00},
	{0x0311, 0x00},
	{0x0312, 0x00},
	{0x0313, 0x00},
	{0x0314, 0xE8},
	{0x0315, 0x03},
	{0x0316, 0x00},
	{0x0317, 0x00},
	{0x0318, 0x30},
	{0x0319, 0x75},
	{0x031A, 0x00},
	{0x031B, 0x00},
	{0x0306, 0x01}, /* Update TW */
	{0x0400, 0x0C}, /* Clock distribution output */
	{0x0401, 0x03},
	{0x0402, 0x00},
	{0x0403, 0x02},
	{0x0404, 0x04},
	{0x0405, 0x08},
	{0x0406, 0x03},
	{0x0407, 0x03},
	{0x0408, 0x03},
	{0x0409, 0x00},
	{0x040A, 0x00},
	{0x040B, 0x00},
	{0x040C, 0x03},
	{0x040D, 0x00},
	{0x040E, 0x00},
	{0x040F, 0x00},
	{0x0410, 0x00},
	{0x0411, 0x00},
	{0x0412, 0x00},
	{0x0413, 0x00},
	{0x0414, 0x00},
	{0x0415, 0x00},
	{0x0416, 0x00},
	{0x0417, 0x00},
	{0x0500, 0xFE}, /* Reference inputs */
	{0x0501, 0x00},
	{0x0502, 0x00},
	{0x0503, 0x08},
	{0x0504, 0x00},
	{0x0505, 0x00},
	{0x0506, 0x00},
	{0x0507, 0x00},
	{0x0600, 0x00}, /* Profiles are 0x0600-0x07FF */
	{0x0601, 0x55}, /* Profile 0 */
	{0x0602, 0xA0}, /* 30MHz input from FPGA, 122.880MHz output clock */
	{0x0603, 0xFC},
	{0x0604, 0x01},
	{0x0605, 0x00},
	{0x0606, 0x00},
	{0x0607, 0x00},
	{0x0608, 0xE8},
	{0x0609, 0x03},
	{0x060A, 0x00},
	{0x060B, 0xE8},
	{0x060C, 0x03},
	{0x060D, 0x00},
	{0x060E, 0x88},
	{0x060F, 0x13},
	{0x0610, 0x88},
	{0x0611, 0x13},
	{0x0612, 0x0E},
	{0x0613, 0xB2},
	{0x0614, 0x08},
	{0x0615, 0x82},
	{0x0616, 0x62},
	{0x0617, 0x42},
	{0x0618, 0xD8},
	{0x0619, 0x47},
	{0x061A, 0x21},
	{0x061B, 0xCB},
	{0x061C, 0xC4},
	{0x061D, 0x05},
	{0x061E, 0x7F},
	{0x061F, 0x00},
	{0x0620, 0x00},
	{0x0621, 0x00},
	{0x0622, 0x0B},
	{0x0623, 0x02},
	{0x0624, 0x00},
	{0x0625, 0x00},
	{0x0626, 0x26},
	{0x0627, 0xB0},
	{0x0628, 0x00},
	{0x0629, 0x10},
	{0x062A, 0x27},
	{0x062B, 0x20},
	{0x062C, 0x44},
	{0x062D, 0xF4},
	{0x062E, 0x01},
	{0x062F, 0x00},
	{0x0630, 0x20},
	{0x0631, 0x44},
	{0x0005, 0x01}, /* I/O Update */
	{0x0A0E, 0x01}, /* Force validation timeout */
	{0x0005, 0x01}, /* I/O Update */
	{0x0A02, 0x02}, /* Sync distribution */
	{0x0005, 0x01},
	{0x0A02, 0x00},
	{0x0005, 0x01},
};

static int ad9548_read(struct spi_device *spi, unsigned reg)
{
	unsigned char buf[3];
	int ret;
	u16 cmd;

	cmd = AD_READ | AD_CNT(1) | AD_ADDR(reg);
	buf[0] = cmd >> 8;
	buf[1] = cmd & 0xFF;


	ret = spi_write_then_read(spi, &buf[0], 2, &buf[2], 1);
	if (ret < 0)
		return ret;

	return buf[2];
}

static int ad9548_write(struct spi_device *spi,
			 unsigned reg, unsigned val)
{
	unsigned char buf[3];
	int ret;
	u16 cmd;

	cmd = AD_WRITE | AD_CNT(1) | AD_ADDR(reg);
	buf[0] = cmd >> 8;
	buf[1] = cmd & 0xFF;
	buf[2] = val;

	ret = spi_write(spi, buf, 3);
	if (ret < 0)
		return ret;

	return 0;
}

static int ad9548_probe(struct spi_device *spi)
{
	int i, ret, timeout;

	ret = ad9548_read(spi, 0x3);
	if (ret < 0)
		return ret;
	if (ret != CHIPID_AD9548) {
		dev_err(&spi->dev, "Unrecognized CHIP_ID 0x%X\n", ret);
 		return -ENODEV;
	}

	for (i = 0; i < ARRAY_SIZE(ad9548_regs); i++)
		switch (ad9548_regs[i][0]) {
		case WAIT_B:
			timeout = 100;
			do {
				ret = ad9548_read(spi, 0xD01);
				if (ret < 0)
					return ret;
				if (ret & BIT(0))
					break;
				mdelay(1);
			} while (timeout--);

			if (timeout <= 0)
				return -ETIMEDOUT;
			break;
		default:
			ret = ad9548_write(spi, ad9548_regs[i][0],
					   ad9548_regs[i][1]);
			if (ret < 0)
				return ret;
			break;
		}

	spi_set_drvdata(spi, NULL);
	dev_info(&spi->dev, "Rev. 0x%X probed\n", ad9548_read(spi, 0x2));

	return 0;
}

static int ad9548_remove(struct spi_device *spi)
{
	spi_set_drvdata(spi, NULL);

	return 0;
}

static const struct spi_device_id ad9548_id[] = {
	{"ad9548", 0},
	{}
};
MODULE_DEVICE_TABLE(spi, ad9548_id);

static struct spi_driver ad9548_driver = {
	.driver = {
		.name	= "ad9548",
		.owner	= THIS_MODULE,
	},
	.probe		= ad9548_probe,
	.remove		= ad9548_remove,
	.id_table	= ad9548_id,
};
module_spi_driver(ad9548_driver);

MODULE_AUTHOR("Michael Hennerich <hennerich@blackfin.uclinux.org>");
MODULE_DESCRIPTION("Analog Devices AD9548");
MODULE_LICENSE("GPL v2");
