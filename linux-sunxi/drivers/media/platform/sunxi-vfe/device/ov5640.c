/*
 * A V4L2 driver for ov5640 cameras.
 *
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/videodev2.h>
#include <linux/clk.h>
#include <media/v4l2-device.h>
#include <media/v4l2-mediabus.h>
#include <linux/io.h>


#include "camera.h"
#include "sensor_helper.h"


MODULE_AUTHOR("raymonxiu");
MODULE_DESCRIPTION("A low-level driver for ov5640 sensors");
MODULE_LICENSE("GPL");

#define AF_WIN_NEW_COORD
/* for internel driver debug */
#define DEV_DBG_EN      0
#if (DEV_DBG_EN == 1)
#define vfe_dev_dbg(x, arg...) printk("[OV5640]"x, ##arg)
#else
#define vfe_dev_dbg(x, arg...)
#endif
#define vfe_dev_err(x, arg...) printk("[OV5640]"x, ##arg)
#define vfe_dev_print(x, arg...) printk("[OV5640]"x, ##arg)

#define CAP_BDG 0
#if (CAP_BDG == 1)
#define vfe_dev_cap_dbg(x, arg...) printk("[OV5640_CAP_DBG]"x, ##arg)
#else
#define vfe_dev_cap_dbg(x, arg...)
#endif

#define LOG_ERR_RET(x)  { \
			  int ret;  \
			  ret = x; \
			  if (ret < 0) {\
			    vfe_dev_err("error at %s\n", __func__);  \
			    return ret; \
			  } \
			}

/* define module timing */
#define MCLK              (24*1000*1000)
static int MCLK_DIV = 1;
#ifdef CONFIG_ARCH_SUN9IW1P1
static int A80_VERSION;
#endif

/* #define FPGA_VER */

#define VREF_POL          V4L2_MBUS_VSYNC_ACTIVE_LOW
#define HREF_POL          V4L2_MBUS_HSYNC_ACTIVE_HIGH
#define CLK_POL           V4L2_MBUS_PCLK_SAMPLE_RISING

#define V4L2_IDENT_SENSOR 0x5640

#define SENSOR_NAME "ov5640"

#ifdef _FLASH_FUNC_
#include "../flash_light/flash.h"
static unsigned int to_flash;
static unsigned int flash_auto_level = 0x1c;
#endif
#define CONTINUEOUS_AF
/* #define AUTO_FPS */
#define DENOISE_LV_AUTO
#define SHARPNESS 0x18

#ifdef AUTO_FPS
/* #define AF_FAST */
#endif

#ifndef DENOISE_LV_AUTO
#define DENOISE_LV 0x8
#endif

#define AE_CW 1

unsigned int night_mode;
unsigned int Nfrms = 1;
unsigned int cap_manual_gain = 0x10;
#define CAP_GAIN_CAL 0/* 0--auto limit frames;1--manual fixed gain */
#define CAP_MULTI_FRAMES
#ifdef CAP_MULTI_FRAMES
#define MAX_FRM_CAP 4
#else
#define MAX_FRM_CAP 1
#endif


/*
 * Our nominal (default) frame rate.
 */
#define SENSOR_FRAME_RATE 30

/*
 * The ov5640 sits on i2c with ID 0x78
 */
#define I2C_ADDR 0x78

/* static struct delayed_work sensor_s_ae_ratio_work; */
static struct v4l2_subdev *glb_sd;

/*
 * Information we maintain about a known sensor.
 */
struct sensor_format_struct;  /* coming later */

struct cfg_array { /* coming later */
	struct regval_list *regs;
	int size;
};

static inline struct sensor_info *to_state(struct v4l2_subdev *sd)
{
	return container_of(sd, struct sensor_info, sd);
}


/*
 * The default register settings
 *
 */

#if 1
static struct regval_list sensor_default_regs[] = {
    /* VGA(YUV) 30fps */
{0x3103, 0x11},
{0x3008, 0x82},
{REG_DLY, 0x1e},
{0x3008, 0x42},
{0x3103, 0x03},
{0x3017, 0xff},
{0x3018, 0xff},
{0x3034, 0x1a},
{0x3035, 0x11},
{0x3036, 0x46},
{0x3037, 0x13},
{0x3108, 0x01},
{0x3630, 0x36},
{0x3631, 0x0e},
{0x3632, 0xe2},
{0x3633, 0x12},
{0x3621, 0xe0},
{0x3704, 0xa0},
{0x3703, 0x5a},
{0x3715, 0x78},
{0x3717, 0x01},
{0x370b, 0x60},
{0x3705, 0x1a},
{0x3905, 0x02},
{0x3906, 0x10},
{0x3901, 0x0a},
{0x3731, 0x12},
{0x3600, 0x08},
{0x3601, 0x33},
{0x302d, 0x60},
{0x3620, 0x52},
{0x371b, 0x20},
{0x471c, 0x50},
{0x3a13, 0x43},
{0x3a18, 0x00},
{0x3a19, 0xf8},
{0x3635, 0x13},
{0x3636, 0x03},
{0x3634, 0x40},
{0x3622, 0x01},
{0x3c01, 0x34},
{0x3c04, 0x28},
{0x3c05, 0x98},
{0x3c06, 0x00},
{0x3c07, 0x08},
{0x3c08, 0x00},
{0x3c09, 0x1c},
{0x3c0a, 0x9c},
{0x3c0b, 0x40},
{0x3820, 0x41},
{0x3821, 0x07},
{0x3814, 0x31},
{0x3815, 0x31},
{0x3800, 0x00},
{0x3801, 0x00},
{0x3802, 0x00},
{0x3803, 0x04},
{0x3804, 0x0a},
{0x3805, 0x3f},
{0x3806, 0x07},
{0x3807, 0x9b},
{0x3808, 0x02},
{0x3809, 0x80},
{0x380a, 0x01},
{0x380b, 0xe0},
{0x380c, 0x07},
{0x380d, 0x68},
{0x380e, 0x03},
{0x380f, 0xd8},
{0x3810, 0x00},
{0x3811, 0x10},
{0x3812, 0x00},
{0x3813, 0x06},
{0x3618, 0x00},
{0x3612, 0x29},
{0x3708, 0x64},
{0x3709, 0x52},
{0x370c, 0x03},
{0x3a02, 0x03},
{0x3a03, 0xd8},
{0x3a08, 0x01},
{0x3a09, 0x27},
{0x3a0a, 0x00},
{0x3a0b, 0xf6},
{0x3a0e, 0x03},
{0x3a0d, 0x04},
{0x3a14, 0x03},
{0x3a15, 0xd8},
{0x4001, 0x02},
{0x4004, 0x02},
{0x3000, 0x00},
{0x3002, 0x1c},
{0x3004, 0xff},
{0x3006, 0xc3},
{0x300e, 0x58},
{0x302e, 0x00},
{0x4300, 0x30},
{0x501f, 0x00},
{0x4713, 0x03},
{0x4407, 0x04},
{0x440e, 0x00},
{0x460b, 0x35},
{0x460c, 0x22},
{0x4837, 0x22},
{0x3824, 0x02},
{0x5000, 0xa7},
{0x5001, 0xa3},
{0x5180, 0xff},
{0x5181, 0xf2},
{0x5182, 0x00},
{0x5183, 0x14},
{0x5184, 0x25},
{0x5185, 0x24},
{0x5186, 0x09},
{0x5187, 0x09},
{0x5188, 0x09},
{0x5189, 0x75},
{0x518a, 0x54},
{0x518b, 0xe0},
{0x518c, 0xb2},
{0x518d, 0x42},
{0x518e, 0x3d},
{0x518f, 0x56},
{0x5190, 0x46},
{0x5191, 0xf8},
{0x5192, 0x04},
{0x5193, 0x70},
{0x5194, 0xf0},
{0x5195, 0xf0},
{0x5196, 0x03},
{0x5197, 0x01},
{0x5198, 0x04},
{0x5199, 0x12},
{0x519a, 0x04},
{0x519b, 0x00},
{0x519c, 0x06},
{0x519d, 0x82},
{0x519e, 0x38},
{0x5381, 0x1e},
{0x5382, 0x5b},
{0x5383, 0x08},
{0x5384, 0x0a},
{0x5385, 0x7e},
{0x5386, 0x88},
{0x5387, 0x7c},
{0x5388, 0x6c},
{0x5389, 0x10},
{0x538a, 0x01},
{0x538b, 0x98},
{0x5300, 0x08},
{0x5301, 0x30},
{0x5302, 0x10},
{0x5303, 0x00},
{0x5304, 0x08},
{0x5305, 0x30},
{0x5306, 0x08},
{0x5307, 0x16},
{0x5309, 0x08},
{0x530a, 0x30},
{0x530b, 0x04},
{0x530c, 0x06},
{0x5480, 0x01},
{0x5481, 0x08},
{0x5482, 0x14},
{0x5483, 0x28},
{0x5484, 0x51},
{0x5485, 0x65},
{0x5486, 0x71},
{0x5487, 0x7d},
{0x5488, 0x87},
{0x5489, 0x91},
{0x548a, 0x9a},
{0x548b, 0xaa},
{0x548c, 0xb8},
{0x548d, 0xcd},
{0x548e, 0xdd},
{0x548f, 0xea},
{0x5490, 0x1d},
{0x5580, 0x02},
{0x5583, 0x40},
{0x5584, 0x10},
{0x5589, 0x10},
{0x558a, 0x00},
{0x558b, 0xf8},
{0x5800, 0x23},
{0x5801, 0x14},
{0x5802, 0x0f},
{0x5803, 0x0f},
{0x5804, 0x12},
{0x5805, 0x26},
{0x5806, 0x0c},
{0x5807, 0x08},
{0x5808, 0x05},
{0x5809, 0x05},
{0x580a, 0x08},
{0x580b, 0x0d},
{0x580c, 0x08},
{0x580d, 0x03},
{0x580e, 0x00},
{0x580f, 0x00},
{0x5810, 0x03},
{0x5811, 0x09},
{0x5812, 0x07},
{0x5813, 0x03},
{0x5814, 0x00},
{0x5815, 0x01},
{0x5816, 0x03},
{0x5817, 0x08},
{0x5818, 0x0d},
{0x5819, 0x08},
{0x581a, 0x05},
{0x581b, 0x06},
{0x581c, 0x08},
{0x581d, 0x0e},
{0x581e, 0x29},
{0x581f, 0x17},
{0x5820, 0x11},
{0x5821, 0x11},
{0x5822, 0x15},
{0x5823, 0x28},
{0x5824, 0x46},
{0x5825, 0x26},
{0x5826, 0x08},
{0x5827, 0x26},
{0x5828, 0x64},
{0x5829, 0x26},
{0x582a, 0x24},
{0x582b, 0x22},
{0x582c, 0x24},
{0x582d, 0x24},
{0x582e, 0x06},
{0x582f, 0x22},
{0x5830, 0x40},
{0x5831, 0x42},
{0x5832, 0x24},
{0x5833, 0x26},
{0x5834, 0x24},
{0x5835, 0x22},
{0x5836, 0x22},
{0x5837, 0x26},
{0x5838, 0x44},
{0x5839, 0x24},
{0x583a, 0x26},
{0x583b, 0x28},
{0x583c, 0x42},
{0x583d, 0xce},
{0x5025, 0x00},
{0x3a0f, 0x30},
{0x3a10, 0x28},
{0x3a1b, 0x30},
{0x3a1e, 0x26},
{0x3a11, 0x60},
{0x3a1f, 0x14},
{0x3008, 0x02},
};

#else
static struct regval_list sensor_default_regs[] = {
	/* VGA(YUV) 7.5fps */
{0x3103, 0x11},
{0x3008, 0x82},
{REG_DLY, 0x1e},
{0x3008, 0x42},
{0x3103, 0x03},
{0x3017, 0xff},
{0x3018, 0xff},
{0x3034, 0x1a},
{0x3035, 0x11},
{0x3036, 0x46},
{0x3037, 0x13},
{0x3108, 0x01},
{0x3630, 0x36},
{0x3631, 0x0e},
{0x3632, 0xe2},
{0x3633, 0x12},
{0x3621, 0xe0},
{0x3704, 0xa0},
{0x3703, 0x5a},
{0x3715, 0x78},
{0x3717, 0x01},
{0x370b, 0x60},
{0x3705, 0x1a},
{0x3905, 0x02},
{0x3906, 0x10},
{0x3901, 0x0a},
{0x3731, 0x12},
{0x3600, 0x08},
{0x3601, 0x33},
{0x302d, 0x60},
{0x3620, 0x52},
{0x371b, 0x20},
{0x471c, 0x50},
{0x3a13, 0x43},
{0x3a18, 0x00},
{0x3a19, 0xf8},
{0x3635, 0x13},
{0x3636, 0x03},
{0x3634, 0x40},
{0x3622, 0x01},
{0x3c01, 0x34},
{0x3c04, 0x28},
{0x3c05, 0x98},
{0x3c06, 0x00},
{0x3c07, 0x08},
{0x3c08, 0x00},
{0x3c09, 0x1c},
{0x3c0a, 0x9c},
{0x3c0b, 0x40},
/*{0x3820, 0x41},*/
/*{0x3821, 0x07},*/
{0x3814, 0x31},
{0x3815, 0x31},
{0x3800, 0x00},
{0x3801, 0x00},
{0x3802, 0x00},
{0x3803, 0x04},
{0x3804, 0x0a},
{0x3805, 0x3f},
{0x3806, 0x07},
{0x3807, 0x9b},
{0x3808, 0x02},
{0x3809, 0x80},
{0x380a, 0x01},
{0x380b, 0xe0},
{0x380c, 0x07},
{0x380d, 0x68},
{0x380e, 0x03},
{0x380f, 0xd8},
{0x3810, 0x00},
{0x3811, 0x10},
{0x3812, 0x00},
{0x3813, 0x06},
{0x3618, 0x00},
{0x3612, 0x29},
{0x3708, 0x64},
{0x3709, 0x52},
{0x370c, 0x03},
{0x3a02, 0x03},
{0x3a03, 0xd8},
{0x3a08, 0x01},
{0x3a09, 0x27},
{0x3a0a, 0x00},
{0x3a0b, 0xf6},
{0x3a0e, 0x03},
{0x3a0d, 0x04},
{0x3a14, 0x03},
{0x3a15, 0xd8},
{0x4001, 0x02},
{0x4004, 0x02},
{0x3000, 0x00},
{0x3002, 0x1c},
{0x3004, 0xff},
{0x3006, 0xc3},
{0x300e, 0x58},
{0x302e, 0x00},
{0x4300, 0x30},
{0x501f, 0x00},
{0x4713, 0x03},
{0x4407, 0x04},
{0x440e, 0x00},
{0x460b, 0x35},
{0x460c, 0x22},
{0x4837, 0x22},
{0x3824, 0x02},
{0x5000, 0xa7},
{0x5001, 0xa3},
{0x5180, 0xff},
{0x5181, 0xf2},
{0x5182, 0x00},
{0x5183, 0x14},
{0x5184, 0x25},
{0x5185, 0x24},
{0x5186, 0x09},
{0x5187, 0x09},
{0x5188, 0x09},
{0x5189, 0x75},
{0x518a, 0x54},
{0x518b, 0xe0},
{0x518c, 0xb2},
{0x518d, 0x42},
{0x518e, 0x3d},
{0x518f, 0x56},
{0x5190, 0x46},
{0x5191, 0xf8},
{0x5192, 0x04},
{0x5193, 0x70},
{0x5194, 0xf0},
{0x5195, 0xf0},
{0x5196, 0x03},
{0x5197, 0x01},
{0x5198, 0x04},
{0x5199, 0x12},
{0x519a, 0x04},
{0x519b, 0x00},
{0x519c, 0x06},
{0x519d, 0x82},
{0x519e, 0x38},
{0x5381, 0x1e},
{0x5382, 0x5b},
{0x5383, 0x08},
{0x5384, 0x0a},
{0x5385, 0x7e},
{0x5386, 0x88},
{0x5387, 0x7c},
{0x5388, 0x6c},
{0x5389, 0x10},
{0x538a, 0x01},
{0x538b, 0x98},
{0x5300, 0x08},
{0x5301, 0x30},
{0x5302, 0x10},
{0x5303, 0x00},
{0x5304, 0x08},
{0x5305, 0x30},
{0x5306, 0x08},
{0x5307, 0x16},
{0x5309, 0x08},
{0x530a, 0x30},
{0x530b, 0x04},
{0x530c, 0x06},
{0x5480, 0x01},
{0x5481, 0x08},
{0x5482, 0x14},
{0x5483, 0x28},
{0x5484, 0x51},
{0x5485, 0x65},
{0x5486, 0x71},
{0x5487, 0x7d},
{0x5488, 0x87},
{0x5489, 0x91},
{0x548a, 0x9a},
{0x548b, 0xaa},
{0x548c, 0xb8},
{0x548d, 0xcd},
{0x548e, 0xdd},
{0x548f, 0xea},
{0x5490, 0x1d},
{0x5580, 0x02},
{0x5583, 0x40},
{0x5584, 0x10},
{0x5589, 0x10},
{0x558a, 0x00},
{0x558b, 0xf8},
{0x5800, 0x23},
{0x5801, 0x14},
{0x5802, 0x0f},
{0x5803, 0x0f},
{0x5804, 0x12},
{0x5805, 0x26},
{0x5806, 0x0c},
{0x5807, 0x08},
{0x5808, 0x05},
{0x5809, 0x05},
{0x580a, 0x08},
{0x580b, 0x0d},
{0x580c, 0x08},
{0x580d, 0x03},
{0x580e, 0x00},
{0x580f, 0x00},
{0x5810, 0x03},
{0x5811, 0x09},
{0x5812, 0x07},
{0x5813, 0x03},
{0x5814, 0x00},
{0x5815, 0x01},
{0x5816, 0x03},
{0x5817, 0x08},
{0x5818, 0x0d},
{0x5819, 0x08},
{0x581a, 0x05},
{0x581b, 0x06},
{0x581c, 0x08},
{0x581d, 0x0e},
{0x581e, 0x29},
{0x581f, 0x17},
{0x5820, 0x11},
{0x5821, 0x11},
{0x5822, 0x15},
{0x5823, 0x28},
{0x5824, 0x46},
{0x5825, 0x26},
{0x5826, 0x08},
{0x5827, 0x26},
{0x5828, 0x64},
{0x5829, 0x26},
{0x582a, 0x24},
{0x582b, 0x22},
{0x582c, 0x24},
{0x582d, 0x24},
{0x582e, 0x06},
{0x582f, 0x22},
{0x5830, 0x40},
{0x5831, 0x42},
{0x5832, 0x24},
{0x5833, 0x26},
{0x5834, 0x24},
{0x5835, 0x22},
{0x5836, 0x22},
{0x5837, 0x26},
{0x5838, 0x44},
{0x5839, 0x24},
{0x583a, 0x26},
{0x583b, 0x28},
{0x583c, 0x42},
{0x583d, 0xce},
{0x5025, 0x00},
{0x3a0f, 0x30},
{0x3a10, 0x28},
{0x3a1b, 0x30},
{0x3a1e, 0x26},
{0x3a11, 0x60},
{0x3a1f, 0x14},
{0x3008, 0x02},

{0x3035, 0x41},
};
#endif

static struct regval_list sensor_1080p_regs[] = {
/* OV5640_1080P_YUV 10fps */
{0x3103, 0x11},
{0x3008, 0x82},
{REG_DLY, 0x1e},
{0x3008, 0x42},
{0x3103, 0x03},
{0x3017, 0xff},
{0x3018, 0xff},
{0x3034, 0x18},/*1a*/
{0x3035, 0x21},
{0x3036, 0x50},/*69*/
{0x3037, 0x13},
{0x3108, 0x01},
{0x3630, 0x2e},
{0x3632, 0xe2},
{0x3633, 0x23},
{0x3621, 0xe0},
{0x3704, 0xa0},
{0x3703, 0x5a},
{0x3715, 0x78},
{0x3717, 0x01},
{0x370b, 0x60},
{0x3705, 0x1a},
{0x3905, 0x02},
{0x3906, 0x10},
{0x3901, 0x0a},
{0x3731, 0x12},
{0x3600, 0x08},
{0x3601, 0x33},
{0x302d, 0x60},
{0x3620, 0x52},
{0x371b, 0x20},
{0x471c, 0x50},
{0x3a13, 0x43},
{0x3a18, 0x00},
{0x3a19, 0xf8},
{0x3635, 0x1c},
{0x3634, 0x40},
{0x3622, 0x01},
{0x3c01, 0x34},
{0x3c04, 0x28},
{0x3c05, 0x98},
{0x3c06, 0x00},
{0x3c07, 0x07},
{0x3c08, 0x00},
{0x3c09, 0x1c},
{0x3c0a, 0x9c},
{0x3c0b, 0x40},
{0x3820, 0x41},
{0x3821, 0x06},
{0x3814, 0x11},
{0x3815, 0x11},
{0x3800, 0x01},
{0x3801, 0x50},
{0x3802, 0x01},
{0x3803, 0xb2},
{0x3804, 0x08},
{0x3805, 0xef},
{0x3806, 0x05},
{0x3807, 0xf2},
{0x3808, 0x07},
{0x3809, 0x80},
{0x380a, 0x04},
{0x380b, 0x38},
{0x380c, 0x09},
{0x380d, 0xc4},
{0x380e, 0x04},
{0x380f, 0x60},
{0x3810, 0x00},
{0x3811, 0x10},
{0x3812, 0x00},
{0x3813, 0x04},
{0x3618, 0x04},
{0x3612, 0x2b},
{0x3708, 0x64},/*62*/
{0x3709, 0x12},
{0x370c, 0x00},
{0x3a02, 0x04},
{0x3a03, 0x60},
{0x3a08, 0x01},
{0x3a09, 0x50},
{0x3a0a, 0x01},
{0x3a0b, 0x18},
{0x3a0e, 0x03},
{0x3a0d, 0x04},
{0x3a14, 0x04},
{0x3a15, 0x60},
{0x4001, 0x02},
{0x4004, 0x06},
{0x3000, 0x00},
{0x3002, 0x1c},
{0x3004, 0xff},
{0x3006, 0xc3},
{0x300e, 0x58},
{0x302e, 0x00},
{0x4300, 0x30},
{0x501f, 0x00},
{0x4713, 0x02},
{0x4407, 0x04},
{0x460b, 0x37},
{0x460c, 0x20},
{0x3824, 0x04},
{0x5000, 0xa7},
{0x5001, 0x83},
{0x5180, 0xff},
{0x5181, 0xf2},
{0x5182, 0x00},
{0x5183, 0x14},
{0x5184, 0x25},
{0x5185, 0x24},
{0x5186, 0x09},
{0x5187, 0x09},
{0x5188, 0x09},
{0x5189, 0x75},
{0x518a, 0x54},
{0x518b, 0xe0},
{0x518c, 0xb2},
{0x518d, 0x42},
{0x518e, 0x3d},
{0x518f, 0x56},
{0x5190, 0x46},
{0x5191, 0xf8},
{0x5192, 0x04},
{0x5193, 0x70},
{0x5194, 0xf0},
{0x5195, 0xf0},
{0x5196, 0x03},
{0x5197, 0x01},
{0x5198, 0x04},
{0x5199, 0x12},
{0x519a, 0x04},
{0x519b, 0x00},
{0x519c, 0x06},
{0x519d, 0x82},
{0x519e, 0x38},
{0x5381, 0x1c},
{0x5382, 0x5a},
{0x5383, 0x06},
{0x5384, 0x0a},
{0x5385, 0x7e},
{0x5386, 0x88},
{0x5387, 0x7c},
{0x5388, 0x6c},
{0x5389, 0x10},
{0x538a, 0x01},
{0x538b, 0x98},
{0x5300, 0x08},
{0x5301, 0x30},
{0x5302, 0x10},
{0x5303, 0x00},
{0x5304, 0x08},
{0x5305, 0x30},
{0x5306, 0x08},
{0x5307, 0x16},
{0x5309, 0x08},
{0x530a, 0x30},
{0x530b, 0x04},
{0x530c, 0x06},
{0x5480, 0x01},
{0x5481, 0x08},
{0x5482, 0x14},
{0x5483, 0x28},
{0x5484, 0x51},
{0x5485, 0x65},
{0x5486, 0x71},
{0x5487, 0x7d},
{0x5488, 0x87},
{0x5489, 0x91},
{0x548a, 0x9a},
{0x548b, 0xaa},
{0x548c, 0xb8},
{0x548d, 0xcd},
{0x548e, 0xdd},
{0x548f, 0xea},
{0x5490, 0x1d},
{0x5580, 0x02},
{0x5583, 0x40},
{0x5584, 0x10},
{0x5589, 0x10},
{0x558a, 0x00},
{0x558b, 0xf8},
{0x5800, 0x23},
{0x5801, 0x14},
{0x5802, 0x0f},
{0x5803, 0x0f},
{0x5804, 0x12},
{0x5805, 0x26},
{0x5806, 0x0c},
{0x5807, 0x08},
{0x5808, 0x05},
{0x5809, 0x05},
{0x580a, 0x08},
{0x580b, 0x0d},
{0x580c, 0x08},
{0x580d, 0x03},
{0x580e, 0x00},
{0x580f, 0x00},
{0x5810, 0x03},
{0x5811, 0x09},
{0x5812, 0x07},
{0x5813, 0x03},
{0x5814, 0x00},
{0x5815, 0x01},
{0x5816, 0x03},
{0x5817, 0x08},
{0x5818, 0x0d},
{0x5819, 0x08},
{0x581a, 0x05},
{0x581b, 0x06},
{0x581c, 0x08},
{0x581d, 0x0e},
{0x581e, 0x29},
{0x581f, 0x17},
{0x5820, 0x11},
{0x5821, 0x11},
{0x5822, 0x15},
{0x5823, 0x28},
{0x5824, 0x46},
{0x5825, 0x26},
{0x5826, 0x08},
{0x5827, 0x26},
{0x5828, 0x64},
{0x5829, 0x26},
{0x582a, 0x24},
{0x582b, 0x22},
{0x582c, 0x24},
{0x582d, 0x24},
{0x582e, 0x06},
{0x582f, 0x22},
{0x5830, 0x40},
{0x5831, 0x42},
{0x5832, 0x24},
{0x5833, 0x26},
{0x5834, 0x24},
{0x5835, 0x22},
{0x5836, 0x22},
{0x5837, 0x26},
{0x5838, 0x44},
{0x5839, 0x24},
{0x583a, 0x26},
{0x583b, 0x28},
{0x583c, 0x42},
{0x583d, 0xce},
{0x5025, 0x00},
{0x3a0f, 0x30},
{0x3a10, 0x28},
{0x3a1b, 0x30},
{0x3a1e, 0x26},
{0x3a11, 0x60},
{0x3a1f, 0x14},
{0x3035, 0x21},
{0x3008, 0x02},
};

#ifdef AUTO_FPS
/* auto framerate mode */
static struct regval_list sensor_auto_fps_mode[] = {

};

/* auto framerate mode */
static struct regval_list sensor_fix_fps_mode[] = {

};
#endif
/* misc */
static struct regval_list sensor_oe_disable_regs[] = {

};

static struct regval_list sensor_oe_enable_regs[] = {

};

static struct regval_list sensor_sw_stby_on_regs[] = {

};

static struct regval_list sensor_sw_stby_off_regs[] = {

};

static unsigned char sensor_af_fw_regs[] = {

};

/*
 * The white balance settings
 * Here only tune the R G B channel gain.
 * The white balance enalbe bit is modified in sensor_s_autowb and sensor_s_wb
 */
static struct regval_list sensor_wb_manual[] = {

};

static struct regval_list sensor_wb_auto_regs[] = {
	/* advanced awb */

};

static struct regval_list sensor_wb_incandescence_regs[] = {

};

static struct regval_list sensor_wb_fluorescent_regs[] = {
	/* ri guang deng */

};

static struct regval_list sensor_wb_tungsten_regs[] = {
	/* wu si deng */

};

static struct regval_list sensor_wb_horizon[] = {
/* null */
};

static struct regval_list sensor_wb_daylight_regs[] = {
	/* tai yang guang */

};

static struct regval_list sensor_wb_flash[] = {
/* null */
};

static struct regval_list sensor_wb_cloud_regs[] = {

};

static struct regval_list sensor_wb_shade[] = {
/* null */
};

static struct cfg_array sensor_wb[] = {
	{
		.regs = sensor_wb_manual,             /* V4L2_WHITE_BALANCE_MANUAL */
		.size = ARRAY_SIZE(sensor_wb_manual),
	},
	{
		.regs = sensor_wb_auto_regs,          /* V4L2_WHITE_BALANCE_AUTO */
		.size = ARRAY_SIZE(sensor_wb_auto_regs),
	},
	{
		.regs = sensor_wb_incandescence_regs, /* V4L2_WHITE_BALANCE_INCANDESCENT */
		.size = ARRAY_SIZE(sensor_wb_incandescence_regs),
	},
	{
		.regs = sensor_wb_fluorescent_regs,   /* V4L2_WHITE_BALANCE_FLUORESCENT */
		.size = ARRAY_SIZE(sensor_wb_fluorescent_regs),
	},
	{
		.regs = sensor_wb_tungsten_regs,      /* V4L2_WHITE_BALANCE_FLUORESCENT_H */
		.size = ARRAY_SIZE(sensor_wb_tungsten_regs),
	},
	{
		.regs = sensor_wb_horizon,            /* V4L2_WHITE_BALANCE_HORIZON */
		.size = ARRAY_SIZE(sensor_wb_horizon),
	},
	{
		.regs = sensor_wb_daylight_regs,      /* V4L2_WHITE_BALANCE_DAYLIGHT */
		.size = ARRAY_SIZE(sensor_wb_daylight_regs),
	},
	{
		.regs = sensor_wb_flash,              /* V4L2_WHITE_BALANCE_FLASH */
		.size = ARRAY_SIZE(sensor_wb_flash),
	},
	{
		.regs = sensor_wb_cloud_regs,         /* V4L2_WHITE_BALANCE_CLOUDY */
		.size = ARRAY_SIZE(sensor_wb_cloud_regs),
	},
	{
		.regs = sensor_wb_shade,              /* V4L2_WHITE_BALANCE_SHADE */
		.size = ARRAY_SIZE(sensor_wb_shade),
	},
};


/*
 * The color effect settings
 */
static struct regval_list sensor_colorfx_none_regs[] = {

};

static struct regval_list sensor_colorfx_bw_regs[] = {

};

static struct regval_list sensor_colorfx_sepia_regs[] = {

};

static struct regval_list sensor_colorfx_negative_regs[] = {

};

static struct regval_list sensor_colorfx_emboss_regs[] = {

};

static struct regval_list sensor_colorfx_sketch_regs[] = {

};

static struct regval_list sensor_colorfx_sky_blue_regs[] = {

};

static struct regval_list sensor_colorfx_grass_green_regs[] = {

};

static struct regval_list sensor_colorfx_skin_whiten_regs[] = {
/* NULL */
};

static struct regval_list sensor_colorfx_vivid_regs[] = {
/* NULL */
};

static struct regval_list sensor_colorfx_aqua_regs[] = {
/* null */
};

static struct regval_list sensor_colorfx_art_freeze_regs[] = {
/* null */
};

static struct regval_list sensor_colorfx_silhouette_regs[] = {
/* null */
};

static struct regval_list sensor_colorfx_solarization_regs[] = {
/* null */
};

static struct regval_list sensor_colorfx_antique_regs[] = {
/* null */
};

static struct regval_list sensor_colorfx_set_cbcr_regs[] = {
/* null */
};

static struct cfg_array sensor_colorfx[] = {
	{
		.regs = sensor_colorfx_none_regs,         /* V4L2_COLORFX_NONE = 0, */
		.size = ARRAY_SIZE(sensor_colorfx_none_regs),
	},
	{
		.regs = sensor_colorfx_bw_regs,           /* V4L2_COLORFX_BW   = 1, */
		.size = ARRAY_SIZE(sensor_colorfx_bw_regs),
	},
	{
		.regs = sensor_colorfx_sepia_regs,        /* V4L2_COLORFX_SEPIA  = 2, */
		.size = ARRAY_SIZE(sensor_colorfx_sepia_regs),
	},
	{
		.regs = sensor_colorfx_negative_regs,     /* V4L2_COLORFX_NEGATIVE = 3, */
		.size = ARRAY_SIZE(sensor_colorfx_negative_regs),
	},
	{
		.regs = sensor_colorfx_emboss_regs,       /* V4L2_COLORFX_EMBOSS = 4, */
		.size = ARRAY_SIZE(sensor_colorfx_emboss_regs),
	},
	{
		.regs = sensor_colorfx_sketch_regs,       /* V4L2_COLORFX_SKETCH = 5, */
		.size = ARRAY_SIZE(sensor_colorfx_sketch_regs),
	},
	{
		.regs = sensor_colorfx_sky_blue_regs,     /* V4L2_COLORFX_SKY_BLUE = 6, */
		.size = ARRAY_SIZE(sensor_colorfx_sky_blue_regs),
	},
	{
		.regs = sensor_colorfx_grass_green_regs,  /* V4L2_COLORFX_GRASS_GREEN = 7, */
		.size = ARRAY_SIZE(sensor_colorfx_grass_green_regs),
	},
	{
		.regs = sensor_colorfx_skin_whiten_regs,  /* V4L2_COLORFX_SKIN_WHITEN = 8, */
		.size = ARRAY_SIZE(sensor_colorfx_skin_whiten_regs),
	},
	{
		.regs = sensor_colorfx_vivid_regs,        /* V4L2_COLORFX_VIVID = 9, */
		.size = ARRAY_SIZE(sensor_colorfx_vivid_regs),
	},
	{
		.regs = sensor_colorfx_aqua_regs,         /* V4L2_COLORFX_AQUA = 10, */
		.size = ARRAY_SIZE(sensor_colorfx_aqua_regs),
	},
	{
		.regs = sensor_colorfx_art_freeze_regs,   /* V4L2_COLORFX_ART_FREEZE = 11, */
		.size = ARRAY_SIZE(sensor_colorfx_art_freeze_regs),
	},
	{
		.regs = sensor_colorfx_silhouette_regs,   /* V4L2_COLORFX_SILHOUETTE = 12, */
		.size = ARRAY_SIZE(sensor_colorfx_silhouette_regs),
	},
	{
		.regs = sensor_colorfx_solarization_regs, /* V4L2_COLORFX_SOLARIZATION = 13, */
		.size = ARRAY_SIZE(sensor_colorfx_solarization_regs),
	},
	{
		.regs = sensor_colorfx_antique_regs,      /* V4L2_COLORFX_ANTIQUE = 14, */
		.size = ARRAY_SIZE(sensor_colorfx_antique_regs),
	},
	{
		.regs = sensor_colorfx_set_cbcr_regs,     /* V4L2_COLORFX_SET_CBCR = 15, */
		.size = ARRAY_SIZE(sensor_colorfx_set_cbcr_regs),
	},
};


#if 1
static struct regval_list sensor_sharpness_auto_regs[] = {

};
#endif
#if 1
static struct regval_list sensor_denoise_auto_regs[] = {

};
#endif

/*
 * The brightness setttings
 */
static struct regval_list sensor_brightness_neg4_regs[] = {
/* NULL */
};

static struct regval_list sensor_brightness_neg3_regs[] = {
/* NULL */
};

static struct regval_list sensor_brightness_neg2_regs[] = {
/* NULL */
};

static struct regval_list sensor_brightness_neg1_regs[] = {
/* NULL */
};

static struct regval_list sensor_brightness_zero_regs[] = {
/* NULL */
};

static struct regval_list sensor_brightness_pos1_regs[] = {
/* NULL */
};

static struct regval_list sensor_brightness_pos2_regs[] = {
/* NULL */
};

static struct regval_list sensor_brightness_pos3_regs[] = {
/* NULL */
};

static struct regval_list sensor_brightness_pos4_regs[] = {
/* NULL */
};

static struct cfg_array sensor_brightness[] = {
	{
		.regs = sensor_brightness_neg4_regs,
		.size = ARRAY_SIZE(sensor_brightness_neg4_regs),
	},
	{
		.regs = sensor_brightness_neg3_regs,
		.size = ARRAY_SIZE(sensor_brightness_neg3_regs),
	},
	{
		.regs = sensor_brightness_neg2_regs,
		.size = ARRAY_SIZE(sensor_brightness_neg2_regs),
	},
	{
		.regs = sensor_brightness_neg1_regs,
		.size = ARRAY_SIZE(sensor_brightness_neg1_regs),
	},
	{
		.regs = sensor_brightness_zero_regs,
		.size = ARRAY_SIZE(sensor_brightness_zero_regs),
	},
	{
		.regs = sensor_brightness_pos1_regs,
		.size = ARRAY_SIZE(sensor_brightness_pos1_regs),
	},
	{
		.regs = sensor_brightness_pos2_regs,
		.size = ARRAY_SIZE(sensor_brightness_pos2_regs),
	},
	{
		.regs = sensor_brightness_pos3_regs,
		.size = ARRAY_SIZE(sensor_brightness_pos3_regs),
	},
	{
		.regs = sensor_brightness_pos4_regs,
		.size = ARRAY_SIZE(sensor_brightness_pos4_regs),
	},
};

/*
 * The contrast setttings
 */
static struct regval_list sensor_contrast_neg4_regs[] = {
/* NULL */
};

static struct regval_list sensor_contrast_neg3_regs[] = {
/* NULL */
};

static struct regval_list sensor_contrast_neg2_regs[] = {
/* NULL */
};

static struct regval_list sensor_contrast_neg1_regs[] = {
/* NULL */
};

static struct regval_list sensor_contrast_zero_regs[] = {
/* NULL */
};

static struct regval_list sensor_contrast_pos1_regs[] = {
/* NULL */
};

static struct regval_list sensor_contrast_pos2_regs[] = {
/* NULL */
};

static struct regval_list sensor_contrast_pos3_regs[] = {
/* NULL */
};

static struct regval_list sensor_contrast_pos4_regs[] = {
/* NULL */
};

static struct cfg_array sensor_contrast[] = {
	{
		.regs = sensor_contrast_neg4_regs,
		.size = ARRAY_SIZE(sensor_contrast_neg4_regs),
	},
	{
		.regs = sensor_contrast_neg3_regs,
		.size = ARRAY_SIZE(sensor_contrast_neg3_regs),
	},
	{
		.regs = sensor_contrast_neg2_regs,
		.size = ARRAY_SIZE(sensor_contrast_neg2_regs),
	},
	{
		.regs = sensor_contrast_neg1_regs,
		.size = ARRAY_SIZE(sensor_contrast_neg1_regs),
	},
	{
		.regs = sensor_contrast_zero_regs,
		.size = ARRAY_SIZE(sensor_contrast_zero_regs),
	},
	{
		.regs = sensor_contrast_pos1_regs,
		.size = ARRAY_SIZE(sensor_contrast_pos1_regs),
	},
	{
		.regs = sensor_contrast_pos2_regs,
		.size = ARRAY_SIZE(sensor_contrast_pos2_regs),
	},
	{
		.regs = sensor_contrast_pos3_regs,
		.size = ARRAY_SIZE(sensor_contrast_pos3_regs),
	},
	{
		.regs = sensor_contrast_pos4_regs,
		.size = ARRAY_SIZE(sensor_contrast_pos4_regs),
	},
};

/*
 * The saturation setttings
 */
static struct regval_list sensor_saturation_neg4_regs[] = {
/* NULL */
};

static struct regval_list sensor_saturation_neg3_regs[] = {
/* NULL */
};

static struct regval_list sensor_saturation_neg2_regs[] = {
/* NULL */
};

static struct regval_list sensor_saturation_neg1_regs[] = {
/* NULL */
};

static struct regval_list sensor_saturation_zero_regs[] = {
/* NULL */
};

static struct regval_list sensor_saturation_pos1_regs[] = {
/* NULL */
};

static struct regval_list sensor_saturation_pos2_regs[] = {
/* NULL */
};

static struct regval_list sensor_saturation_pos3_regs[] = {
/* NULL */
};

static struct regval_list sensor_saturation_pos4_regs[] = {
/* NULL */
};

static struct cfg_array sensor_saturation[] = {
	{
		.regs = sensor_saturation_neg4_regs,
		.size = ARRAY_SIZE(sensor_saturation_neg4_regs),
	},
	{
		.regs = sensor_saturation_neg3_regs,
		.size = ARRAY_SIZE(sensor_saturation_neg3_regs),
	},
	{
		.regs = sensor_saturation_neg2_regs,
		.size = ARRAY_SIZE(sensor_saturation_neg2_regs),
	},
	{
		.regs = sensor_saturation_neg1_regs,
		.size = ARRAY_SIZE(sensor_saturation_neg1_regs),
	},
	{
		.regs = sensor_saturation_zero_regs,
		.size = ARRAY_SIZE(sensor_saturation_zero_regs),
	},
	{
		.regs = sensor_saturation_pos1_regs,
		.size = ARRAY_SIZE(sensor_saturation_pos1_regs),
	},
	{
		.regs = sensor_saturation_pos2_regs,
		.size = ARRAY_SIZE(sensor_saturation_pos2_regs),
	},
	{
		.regs = sensor_saturation_pos3_regs,
		.size = ARRAY_SIZE(sensor_saturation_pos3_regs),
	},
	{
		.regs = sensor_saturation_pos4_regs,
		.size = ARRAY_SIZE(sensor_saturation_pos4_regs),
	},
};

/*
 * The exposure target setttings
 */
#if 0
static struct regval_list sensor_ev_neg4_regs[] = {
	{0x3a0f, 0x10},
	{0x3a10, 0x08},
	{0x3a1b, 0x10},
	{0x3a1e, 0x08},
	{0x3a11, 0x20},
	{0x3a1f, 0x10},
};

static struct regval_list sensor_ev_neg3_regs[] = {
	{0x3a0f, 0x18},
	{0x3a10, 0x10},
	{0x3a1b, 0x18},
	{0x3a1e, 0x10},
	{0x3a11, 0x30},
	{0x3a1f, 0x10},
};

static struct regval_list sensor_ev_neg2_regs[] = {
	{0x3a0f, 0x20},
	{0x3a10, 0x18},
	{0x3a1b, 0x20},
	{0x3a1e, 0x18},
	{0x3a11, 0x41},
	{0x3a1f, 0x10},
};
static struct regval_list sensor_ev_neg1_regs[] = {
	{0x3a0f, 0x30},
	{0x3a10, 0x28},
	{0x3a1b, 0x30},
	{0x3a1e, 0x28},
	{0x3a11, 0x51},
	{0x3a1f, 0x10},
	/* {REG_TERM,VAL_TERM}, */
};

static struct regval_list sensor_ev_zero_regs[] = {
	{0x3a0f, 0x38},
	{0x3a10, 0x30},
	{0x3a1b, 0x38},
	{0x3a1e, 0x30},
	{0x3a11, 0x61},
	{0x3a1f, 0x10},
	/* {REG_TERM,VAL_TERM}, */
};

static struct regval_list sensor_ev_pos1_regs[] = {
	{0x3a0f, 0x48},
	{0x3a10, 0x40},
	{0x3a1b, 0x48},
	{0x3a1e, 0x40},
	{0x3a11, 0x80},
	{0x3a1f, 0x20},
	/* {REG_TERM,VAL_TERM}, */
};

static struct regval_list sensor_ev_pos2_regs[] = {
	{0x3a0f, 0x50},
	{0x3a10, 0x48},
	{0x3a1b, 0x50},
	{0x3a1e, 0x48},
	{0x3a11, 0x90},
	{0x3a1f, 0x20},
	/* {REG_TERM,VAL_TERM}, */
};

static struct regval_list sensor_ev_pos3_regs[] = {
	{0x3a0f, 0x58},
	{0x3a10, 0x50},
	{0x3a1b, 0x58},
	{0x3a1e, 0x50},
	{0x3a11, 0x91},
	{0x3a1f, 0x20},
	/* {REG_TERM,VAL_TERM}, */
};

static struct regval_list sensor_ev_pos4_regs[] = {
	{0x3a0f, 0x60},
	{0x3a10, 0x58},
	{0x3a1b, 0x60},
	{0x3a1e, 0x58},
	{0x3a11, 0xa0},
	{0x3a1f, 0x20},
	/* {REG_TERM,VAL_TERM}, */
};
#else
static struct regval_list sensor_ev_neg4_regs[] = {

};

static struct regval_list sensor_ev_neg3_regs[] = {

};

static struct regval_list sensor_ev_neg2_regs[] = {

	/* {REG_TERM,VAL_TERM}, */
};

static struct regval_list sensor_ev_neg1_regs[] = {

};

static struct regval_list sensor_ev_zero_regs[] = {

};

static struct regval_list sensor_ev_pos1_regs[] = {

};

static struct regval_list sensor_ev_pos2_regs[] = {

};

static struct regval_list sensor_ev_pos3_regs[] = {

};

static struct regval_list sensor_ev_pos4_regs[] = {

};
#endif

static struct cfg_array sensor_ev[] = {
	{
		.regs = sensor_ev_neg4_regs,
		.size = ARRAY_SIZE(sensor_ev_neg4_regs),
	},
	{
		.regs = sensor_ev_neg3_regs,
		.size = ARRAY_SIZE(sensor_ev_neg3_regs),
	},
	{
		.regs = sensor_ev_neg2_regs,
		.size = ARRAY_SIZE(sensor_ev_neg2_regs),
	},
	{
		.regs = sensor_ev_neg1_regs,
		.size = ARRAY_SIZE(sensor_ev_neg1_regs),
	},
	{
		.regs = sensor_ev_zero_regs,
		.size = ARRAY_SIZE(sensor_ev_zero_regs),
	},
	{
		.regs = sensor_ev_pos1_regs,
		.size = ARRAY_SIZE(sensor_ev_pos1_regs),
	},
	{
		.regs = sensor_ev_pos2_regs,
		.size = ARRAY_SIZE(sensor_ev_pos2_regs),
	},
	{
		.regs = sensor_ev_pos3_regs,
		.size = ARRAY_SIZE(sensor_ev_pos3_regs),
	},
	{
		.regs = sensor_ev_pos4_regs,
		.size = ARRAY_SIZE(sensor_ev_pos4_regs),
	},
};

/*
 * Here we'll try to encapsulate the changes for just the output
 * video format.
 *
 */


static struct regval_list sensor_fmt_yuv422_yuyv[] = {
	{0x4300, 0x30},
};

static struct regval_list sensor_fmt_yuv422_yvyu[] = {
	{0x4300, 0x31},
};

static struct regval_list sensor_fmt_yuv422_vyuy[] = {
	{0x4300, 0x33},
};

static struct regval_list sensor_fmt_yuv422_uyvy[] = {
	{0x4300, 0x32},
};

static struct regval_list ae_average_tbl[] = {
	/* Whole Image Average */
	{0x5688, 0x11}, /* Zone 1/Zone 0 weight */
	{0x5689, 0x11}, /* Zone 3/Zone 2 weight */
	{0x569a, 0x11}, /* Zone 5/Zone 4 weight */
	{0x569b, 0x11}, /* Zone 7/Zone 6 weight */
	{0x569c, 0x11}, /* Zone 9/Zone 8 weight */
	{0x569d, 0x11}, /* Zone b/Zone a weight */
	{0x569e, 0x11}, /* Zone d/Zone c weight */
	{0x569f, 0x11}, /* Zone f/Zone e weight */
};

static struct regval_list ae_centerweight_tbl[] = {
	/* Whole Image Center More weight */
	{0x5688, 0x62},
	{0x5689, 0x26},
	{0x568a, 0xe6},
	{0x568b, 0x6e},
	{0x568c, 0xea},
	{0x568d, 0xae},
	{0x568e, 0xa6},
	{0x568f, 0x6a},
};


static data_type current_lum = 0xff;
static data_type sensor_get_lum(struct v4l2_subdev *sd)
{
	sensor_read(sd, 0x56a1, &current_lum);
	vfe_dev_cap_dbg("check luminance=0x%x\n", current_lum);

	return current_lum;
}

/* stuff about exposure when capturing image and video*/
static int sensor_s_denoise_value(struct v4l2_subdev *sd, data_type value);
data_type ogain, oexposurelow, oexposuremid, oexposurehigh;
unsigned int preview_exp_line, preview_fps;
unsigned long preview_pclk;

static unsigned int cal_cap_gain(data_type prv_gain, data_type lum)
{
	unsigned int gain_ret = 0x18;

	vfe_dev_cap_dbg("current_lum=0x%x\n", lum);

	if (current_lum > 0xa0) {
		if (ogain > 0x40)
			gain_ret = 0x20;
		else if (ogain > 0x20)
			gain_ret = 0x18;
		else
			gain_ret = 0x10;
	} else if (current_lum > 0x80) {
		if (ogain > 0x40)
			gain_ret = 0x30;
		else if (ogain > 0x20)
			gain_ret = 0x28;
		else
			gain_ret = 0x20;
	} else if (current_lum > 0x40) {
		if (ogain > 0x60)
			gain_ret = ogain/3;
		else if (ogain > 0x40)
			gain_ret = ogain/2;
		else
			gain_ret = ogain;
	} else if (current_lum > 0x20) {
		if (ogain > 0x60)
			gain_ret = ogain/6;
		else if (ogain > 0x20)
			gain_ret = ogain/2;
		else
			gain_ret = ogain;
	} else {
		vfe_dev_cap_dbg("low_light=0x%x\n", lum);
		if (ogain > 0xf0)
			gain_ret = 0x10;
		else if (ogain > 0xe0)
			gain_ret = 0x14;
		else
			gain_ret = 0x18;
	}

	if (gain_ret < 0x10)
		gain_ret = 0x10;

	vfe_dev_cap_dbg("gain return=0x%x\n", gain_ret);
	return gain_ret;
}

static int sensor_set_capture_exposure(struct v4l2_subdev *sd)
{
	unsigned long lines_10ms;
	unsigned int capture_expLines;
	unsigned int preview_explines;
	unsigned long previewExposure;
	unsigned long capture_Exposure;
	unsigned long capture_exposure_gain;
	unsigned long capture_gain;
	data_type gain, exposurelow, exposuremid, exposurehigh;
	unsigned int cap_vts = 0;
	unsigned int cap_vts_diff = 0;
	unsigned int bd_step = 1;
	unsigned int capture_fps;
	data_type rdval;
	struct sensor_info *info = to_state(sd);

#ifndef FPGA_VER
	capture_fps = 75/MCLK_DIV;
#else
	capture_fps = 37;
#endif

	vfe_dev_dbg("sensor_set_capture_exposure\n");
	preview_fps = preview_fps*10;

	if (info->low_speed == 1) {
		capture_fps = capture_fps/2;
	}

	preview_explines = preview_exp_line;/* 984; */
	capture_expLines = 1968;
	if (info->band_filter == V4L2_CID_POWER_LINE_FREQUENCY_60HZ)
		lines_10ms = capture_fps * capture_expLines * 1000/12000;
	else
		lines_10ms = capture_fps * capture_expLines * 1000/10000;
	previewExposure = ((unsigned int)(oexposurehigh))<<12;
	previewExposure += ((unsigned int)oexposuremid)<<4;
	previewExposure += (oexposurelow >> 4);
	vfe_dev_cap_dbg("previewExposure=0x%x\n", previewExposure);

	if (0 == preview_explines || 0 == lines_10ms)
		return 0;

	if (preview_explines == 0 || preview_fps == 0)
		return -EFAULT;

	if (night_mode == 0) {
		capture_Exposure = (1*(previewExposure*(capture_fps)*(capture_expLines))/
							(((preview_explines)*(preview_fps))));
		vfe_dev_cap_dbg("cal from prv: capture_Exposure=0x%x\n", capture_Exposure);
	} else {
		capture_Exposure = (night_mode*(previewExposure*(capture_fps)*(capture_expLines))/
							(((preview_explines)*(preview_fps))));
	}
	vfe_dev_dbg("capture_Exposure=0x%lx\n", capture_Exposure);

	if (CAP_GAIN_CAL == 0) {
		capture_gain = (unsigned long)cal_cap_gain(ogain, current_lum);
		vfe_dev_cap_dbg("auto_limit_frames_mode: ogain=0x%x, capture_gain=0x%x\n", ogain, capture_gain);
		capture_Exposure = capture_Exposure*ogain/capture_gain;
		vfe_dev_cap_dbg("auto_limit_frames_mode: capture_Exposure=0x%x\n", capture_Exposure);
		capture_exposure_gain = capture_Exposure*capture_gain;
		vfe_dev_cap_dbg("auto_limit_frames_mode: capture_exposure_gain=0x%x\n", capture_exposure_gain);

		if (capture_Exposure > Nfrms*capture_expLines) {
			vfe_dev_cap_dbg("auto_limit_frames_mode: longer than %d frames\n", Nfrms);
			capture_gain = capture_exposure_gain/(Nfrms*capture_expLines);
			vfe_dev_cap_dbg("auto_limit_frames_mode: exceed %d frames\n", Nfrms);
			vfe_dev_cap_dbg("auto_limit_frames_mode: re cal capture_gain = 0x%x\n", capture_gain);
			capture_Exposure = Nfrms*capture_expLines;
		}
		if (capture_gain > 0xf8)
			capture_gain = 0xf8;
	} else {/* manual_gain_mode */
		vfe_dev_cap_dbg("manual_gain_mode: before capture_Exposure=0x%x\n", capture_Exposure);
		capture_gain = cap_manual_gain;
		vfe_dev_cap_dbg("manual_gain_mode: capture_gain=0x%x\n", capture_gain);
		capture_Exposure = capture_Exposure*ogain/capture_gain;
		vfe_dev_cap_dbg("manual_gain_mode: after capture_Exposure=0x%x\n", capture_Exposure);
	}

	/* banding */
	/* capture_Exposure = capture_Exposure * 1000; */
	if (capture_Exposure*1000  > lines_10ms) {
		vfe_dev_cap_dbg("lines_10ms=0x%x\n", lines_10ms);
		bd_step = capture_Exposure*1000 / lines_10ms;
		vfe_dev_cap_dbg("bd_step=0x%x\n", bd_step);
		/* capture_Exposure =bd_step * lines_10ms; */
	}
	/* capture_Exposure = capture_Exposure / 1000; */

	if (capture_Exposure == 0)
		capture_Exposure = 1;

	vfe_dev_dbg("capture_Exposure = 0x%lx\n", capture_Exposure);

	if ((1000*capture_Exposure-bd_step*lines_10ms)*16 > lines_10ms) {
		vfe_dev_cap_dbg("(1000*capture_Exposure-bd_step*lines_10ms)*16=%d\n", 16*(1000*capture_Exposure-bd_step*lines_10ms));
		capture_gain = capture_exposure_gain/capture_Exposure;
		vfe_dev_cap_dbg("after banding re cal capture_gain = 0x%x\n", capture_gain);
	}

	if (capture_Exposure > 1968) {
		cap_vts = capture_Exposure;
		cap_vts_diff = capture_Exposure-1968;
		vfe_dev_cap_dbg("cap_vts =%d, cap_vts_diff=%d\n", cap_vts, cap_vts_diff);
	} else {
		cap_vts = 1968;
		cap_vts_diff = 0;
	}
	/* capture_Exposure=1968; */
	exposurelow = ((data_type)capture_Exposure)<<4;
	exposuremid = (data_type)(capture_Exposure >> 4) & 0xff;
	exposurehigh = (data_type)(capture_Exposure >> 12);
	gain = (data_type) capture_gain;

	sensor_read(sd, 0x3503, &rdval);
	vfe_dev_dbg("capture:agc/aec:0x%x,gain:0x%x,exposurelow:0x%x,exposuremid:0x%x,exposurehigh:0x%x\n",\
		  rdval, gain, exposurelow, exposuremid, exposurehigh);

#ifdef DENOISE_LV_AUTO
	sensor_s_denoise_value(sd, 1 + gain*gain/0x100);
#else
	sensor_s_denoise_value(sd, DENOISE_LV);
#endif

	sensor_write(sd, 0x380e, (data_type)(cap_vts>>8));
	sensor_write(sd, 0x380f, (data_type)(cap_vts));
	sensor_write(sd, 0x350c, (data_type)((cap_vts_diff)>>8));
	sensor_write(sd, 0x350d, (data_type)(cap_vts_diff));

	sensor_write(sd, 0x350b, gain);
	sensor_write(sd, 0x3502, exposurelow);
	sensor_write(sd, 0x3501, exposuremid);
	sensor_write(sd, 0x3500, exposurehigh);

	return 0;
}
static int sensor_get_pclk(struct v4l2_subdev *sd)
{
	unsigned long pclk;
	data_type pre_div, mul, sys_div, pll_rdiv, bit_div, sclk_rdiv;

	sensor_read(sd, 0x3037, &pre_div);
	pre_div = pre_div & 0x0f;

	if (pre_div == 0)
		pre_div = 1;

	sensor_read(sd, 0x3036, &mul);
	if (mul >= 128)
		mul = mul/2*2;

	sensor_read(sd, 0x3035, &sys_div);
	sys_div = (sys_div & 0xf0) >> 4;

	sensor_read(sd, 0x3037, &pll_rdiv);
	pll_rdiv = (pll_rdiv & 0x10) >> 4;
	pll_rdiv = pll_rdiv + 1;

	sensor_read(sd, 0x3034, &bit_div);
	bit_div = (bit_div & 0x0f);

	sensor_read(sd, 0x3108, &sclk_rdiv);
	sclk_rdiv = (sclk_rdiv & 0x03);
	sclk_rdiv = sclk_rdiv << sclk_rdiv;

	vfe_dev_dbg("pre_div = %d,mul = %d,sys_div = %d,pll_rdiv = %d,sclk_rdiv = %d\n",\
	      pre_div, mul, sys_div, pll_rdiv, sclk_rdiv);

	if ((pre_div && sys_div && pll_rdiv && sclk_rdiv) == 0)
		return -EFAULT;

	if (bit_div == 8)
		pclk = MCLK/MCLK_DIV / pre_div * mul / sys_div / pll_rdiv / 2 / sclk_rdiv;
	else if (bit_div == 10)
		pclk = MCLK/MCLK_DIV / pre_div * mul / sys_div / pll_rdiv * 2 / 5 / sclk_rdiv;
	else
		pclk = MCLK/MCLK_DIV / pre_div * mul / sys_div / pll_rdiv / 1 / sclk_rdiv;

	vfe_dev_dbg("pclk = %ld\n", pclk);

	preview_pclk = pclk;
	return 0;
}

static int sensor_get_fps(struct v4l2_subdev *sd)
{
	data_type vts_low, vts_high, hts_low, hts_high, vts_extra_high, vts_extra_low;
	unsigned long vts, hts, vts_extra;

	sensor_read(sd, 0x380c, &hts_high);
	sensor_read(sd, 0x380d, &hts_low);
	sensor_read(sd, 0x380e, &vts_high);
	sensor_read(sd, 0x380f, &vts_low);
	sensor_read(sd, 0x350c, &vts_extra_high);
	sensor_read(sd, 0x350d, &vts_extra_low);

	hts = hts_high * 256 + hts_low;
	vts = vts_high * 256 + vts_low;
	vts_extra = vts_extra_high * 256 + vts_extra_low;

	if ((hts && (vts+vts_extra)) == 0)
		return -EFAULT;

	if (sensor_get_pclk(sd))
		vfe_dev_err("get pclk error!\n");

	preview_fps = preview_pclk / ((vts_extra+vts) * hts);
	vfe_dev_dbg("preview fps = %d\n", preview_fps);

	return 0;
}

static int sensor_get_preview_exposure(struct v4l2_subdev *sd)
{
	data_type vts_low, vts_high, vts_extra_high, vts_extra_low;
	unsigned long vts, vts_extra;

	sensor_read(sd, 0x350b, &ogain);
	sensor_read(sd, 0x3502, &oexposurelow);
	sensor_read(sd, 0x3501, &oexposuremid);
	sensor_read(sd, 0x3500, &oexposurehigh);
	sensor_read(sd, 0x380e, &vts_high);
	sensor_read(sd, 0x380f, &vts_low);
	sensor_read(sd, 0x350c, &vts_extra_high);
	sensor_read(sd, 0x350d, &vts_extra_low);

	vts = vts_high * 256 + vts_low;
	vts_extra = vts_extra_high * 256 + vts_extra_low;
	preview_exp_line = vts + vts_extra;

	vfe_dev_dbg("preview_exp_line = %d\n", preview_exp_line);
	vfe_dev_dbg("preview:gain:0x%x,exposurelow:0x%x,exposuremid:0x%x,exposurehigh:0x%x\n",\
		      ogain, oexposurelow, oexposuremid, oexposurehigh);

	return 0;
}

static int sensor_set_preview_exposure(struct v4l2_subdev *sd)
{
	data_type rdval;

	sensor_read(sd, 0x3503, &rdval);
	vfe_dev_dbg("preview:agc/aec:0x%x,gain:0x%x,exposurelow:0x%x,exposuremid:0x%x,exposurehigh:0x%x\n",
		      rdval, ogain, oexposurelow, oexposuremid, oexposurehigh);

	sensor_write(sd, 0x350b, ogain);
	sensor_write(sd, 0x3502, oexposurelow);
	sensor_write(sd, 0x3501, oexposuremid);
	sensor_write(sd, 0x3500, oexposurehigh);
	return 0;
}

#ifdef _FLASH_FUNC_
void check_to_flash(struct v4l2_subdev *sd)
{
	struct sensor_info *info = to_state(sd);

	if (info->flash_mode == V4L2_FLASH_LED_MODE_FLASH) {
		to_flash = 1;
	} else if (info->flash_mode == V4L2_FLASH_LED_MODE_AUTO) {
		sensor_get_lum(sd);
		if (current_lum < flash_auto_level)
			to_flash = 1;
		else
			to_flash = 0;
	} else
		to_flash = 0;

	vfe_dev_dbg("to_flash=%d\n", to_flash);
}
#endif

/* stuff about auto focus */

static int sensor_download_af_fw(struct v4l2_subdev *sd)
{
	int ret, cnt;
	data_type rdval;
	int reload_cnt = 0;

	struct regval_list af_fw_reset_reg[] = {
		{0x3000, 0x20},
	};
	struct regval_list af_fw_start_reg[] = {
		{0x3022, 0x00},
		{0x3023, 0x00},
		{0x3024, 0x00},
		{0x3025, 0x00},
		{0x3026, 0x00},
		{0x3027, 0x00},
		{0x3028, 0x00},
		{0x3029, 0x7f},
		{0x3000, 0x00},
	};

	/* reset sensor MCU */
	ret = sensor_write_array(sd, af_fw_reset_reg, ARRAY_SIZE(af_fw_reset_reg));
	if (ret < 0) {
		vfe_dev_err("reset sensor MCU error\n");
		return ret;
	}
	/* download af fw */
	ret = cci_write_a16_d8_continuous_helper(sd, 0x8000, sensor_af_fw_regs, ARRAY_SIZE(sensor_af_fw_regs));
	if (ret < 0) {
		vfe_dev_err("download af fw error\n");
		return ret;
	}
	/* start af firmware */
	ret = sensor_write_array(sd, af_fw_start_reg, ARRAY_SIZE(af_fw_start_reg));
	if (ret < 0) {
		vfe_dev_err("start af firmware error\n");
		return ret;
	}

	msleep(10);
	/* check the af firmware status */
	rdval = 0xff;
	cnt = 0;
recheck_af_fw:
	while (rdval != 0x70) {
		ret = sensor_read(sd, 0x3029, &rdval);
		if (ret < 0) {
			vfe_dev_err("sensor check the af firmware status err !\n");
			return ret;
		}
		cnt++;
		if (cnt > 3) {
			vfe_dev_err("AF firmware check status time out !\n");
			reload_cnt++;
			if (reload_cnt <= 2) {
				vfe_dev_err("AF firmware check status retry cnt = %d!\n", reload_cnt);
				vfe_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
				usleep_range(10000, 12000);
				vfe_gpio_write(sd, PWDN, CSI_GPIO_LOW);
				usleep_range(10000, 12000);
				goto recheck_af_fw;
			}
			return -EFAULT;
		}
		usleep_range(5000, 10000);
	}
	vfe_dev_print("AF firmware check status complete, 0x3029 = 0x%x\n", rdval);
	return 0;
}

static int sensor_g_single_af(struct v4l2_subdev *sd)
{
	data_type rdval;
	struct sensor_info *info = to_state(sd);

	if (info->focus_status != 1)
		return V4L2_AUTO_FOCUS_STATUS_IDLE;
	rdval = 0xff;
	LOG_ERR_RET(sensor_read(sd, 0x3029, &rdval))
	if (rdval == 0x10) {
		int ret = 0;

		info->focus_status = 0; /* idle */
		sensor_read(sd, 0x3028, &rdval);
		if (rdval == 0) {
			vfe_dev_print("Single AF focus fail, 0x3028 = 0x%x\n", rdval);
			ret = V4L2_AUTO_FOCUS_STATUS_FAILED;
		} else {
			vfe_dev_dbg("Single AF focus ok, 0x3028 = 0x%x\n", rdval);
			ret = V4L2_AUTO_FOCUS_STATUS_REACHED;
		}
#ifdef _FLASH_FUNC_
		if (info->flash_mode != V4L2_FLASH_LED_MODE_NONE) {
			vfe_dev_print("shut flash when af fail/ok\n");
			io_set_flash_ctrl(sd, SW_CTRL_FLASH_OFF);
		}
#endif
		return ret;
	} else if (rdval == 0x70) {
		info->focus_status = 0;
#ifdef _FLASH_FUNC_
		if (info->flash_mode != V4L2_FLASH_LED_MODE_NONE) {
			vfe_dev_print("shut flash when af idle 2\n");
			io_set_flash_ctrl(sd, SW_CTRL_FLASH_OFF);
		}
#endif
		return V4L2_AUTO_FOCUS_STATUS_IDLE;
	} else if (rdval == 0x00) {
		info->focus_status = 1;
		return V4L2_AUTO_FOCUS_STATUS_BUSY;
	}

	return V4L2_AUTO_FOCUS_STATUS_BUSY;
}

static int sensor_g_contin_af(struct v4l2_subdev *sd)
{
	data_type rdval;
	struct sensor_info *info = to_state(sd);

	rdval = 0xff;
	LOG_ERR_RET(sensor_read(sd, 0x3029, &rdval))

	if (rdval == 0x20 || rdval == 0x10) {
		info->focus_status = 0; /* idle */
		sensor_read(sd, 0x3028, &rdval);
		if (rdval == 0)
			return V4L2_AUTO_FOCUS_STATUS_FAILED;
		else
			return V4L2_AUTO_FOCUS_STATUS_REACHED;

	} else if (rdval == 0x00) {
		info->focus_status = 1;/* busy */
		return V4L2_AUTO_FOCUS_STATUS_BUSY;
	} else {/* if(rdval==0x70) */
		info->focus_status = 0;/* idle */
		return V4L2_AUTO_FOCUS_STATUS_IDLE;
	}
}

static int sensor_g_af_status(struct v4l2_subdev *sd)
{
	int ret = 0;
	struct sensor_info *info = to_state(sd);

	if (info->auto_focus == 1)
		ret = sensor_g_contin_af(sd);
	else
		ret = sensor_g_single_af(sd);

	return ret;
}

static int sensor_g_3a_lock(struct v4l2_subdev *sd)
{
	struct sensor_info *info = to_state(sd);

	return ((info->auto_focus == 0)?V4L2_LOCK_FOCUS :  ~V4L2_LOCK_FOCUS |
	       (info->autowb == 0)?V4L2_LOCK_WHITE_BALANCE :  ~V4L2_LOCK_WHITE_BALANCE |
	       (~V4L2_LOCK_EXPOSURE));
}

static int sensor_s_init_af(struct v4l2_subdev *sd)
{
	int ret;
	struct sensor_info *info = to_state(sd);

	ret = sensor_download_af_fw(sd);
	if (ret == 0)
		info->af_first_flag = 0;
	return ret;
}

static int sensor_s_single_af(struct v4l2_subdev *sd)
{
	int ret;
	struct sensor_info *info = to_state(sd);
	data_type rdval = 0xff;
	unsigned int cnt = 0;

	vfe_dev_print("sensor_s_single_af\n");

	info->focus_status = 0; /* idle */

	sensor_write(sd, 0x3023, 0x01);

	ret = sensor_write(sd, 0x3022, 0x03);
	if (ret < 0) {
		vfe_dev_err("sensor tigger single af err !\n");
		return ret;
	}

	while (rdval != 0 && cnt < 10) {
		usleep_range(1000, 1200);
		ret = sensor_read(sd, 0x3023, &rdval);
		cnt++;
	}
	if (cnt > 10)
		vfe_dev_dbg("set single af timeout\n");

#ifdef _FLASH_FUNC_
	if (info->flash_mode != V4L2_FLASH_LED_MODE_NONE) {
		check_to_flash(sd);
		if (to_flash == 1) {
			vfe_dev_print("open torch when start single af\n");
			io_set_flash_ctrl(sd, SW_CTRL_TORCH_ON);
		}
	}
#endif

	info->focus_status = 1; /* busy */
	info->auto_focus = 0;
	return 0;
}

static int sensor_s_continueous_af(struct v4l2_subdev *sd, int value)
{
	struct sensor_info *info = to_state(sd);

	vfe_dev_print("sensor_s_continueous_af[0x%x]\n", value);
	if (info->focus_status == 1) {
		vfe_dev_err("continous focus not accepted when single focus\n");
		return -1;
	}
	if ((info->auto_focus == value)) {
		vfe_dev_dbg("already in same focus mode\n");
		return 0;
	}

	if (value == 1) {
		LOG_ERR_RET(sensor_write(sd, 0x3022, 0x04))
		LOG_ERR_RET(sensor_write(sd, 0x3022, 0x80))
		info->auto_focus = 1;
	} else {
		LOG_ERR_RET(sensor_write(sd, 0x3022, 0x06))/* pause af */
		info->auto_focus = 0;
	}
	return 0;
}

static int sensor_s_pause_af(struct v4l2_subdev *sd)
{
	/* pause af poisition */
	vfe_dev_print("sensor_s_pause_af\n");

	LOG_ERR_RET(sensor_write(sd, 0x3022, 0x06))

	/* msleep(5); */
	return 0;
}

static int sensor_s_release_af(struct v4l2_subdev *sd)
{
	/* release focus */
	vfe_dev_print("sensor_s_release_af\n");

	/* release single af */
	LOG_ERR_RET(sensor_write(sd, 0x3022, 0x08))
	return 0;
}

#if 1
static int sensor_s_relaunch_af_zone(struct v4l2_subdev *sd)
{
	/* relaunch defalut af zone */
	vfe_dev_print("sensor_s_relaunch_af_zone\n");
	LOG_ERR_RET(sensor_write(sd, 0x3023, 0x01))
	LOG_ERR_RET(sensor_write(sd, 0x3022, 0x80))

	usleep_range(5000, 6000);
	return 0;
}
#endif

static int sensor_s_af_zone(struct v4l2_subdev *sd, struct v4l2_win_coordinate *win_c)
{
	struct sensor_info *info = to_state(sd);
	int ret;

	int x1, y1, x2, y2;
	unsigned int xc, yc;
	unsigned int prv_x, prv_y;

	vfe_dev_print("sensor_s_af_zone\n");

	if (info->width == 0 || info->height == 0) {
		vfe_dev_err("current width or height is zero!\n");
		return -EINVAL;
	}

	prv_x = (int)info->width;
	prv_y = (int)info->height;

	x1 = win_c->x1;
	y1 = win_c->y1;
	x2 = win_c->x2;
	y2 = win_c->y2;

#ifdef AF_WIN_NEW_COORD
	xc = prv_x*((unsigned int)(2000+x1+x2)/2)/2000;
	yc = (prv_y*((unsigned int)(2000+y1+y2)/2)/2000);
#else
	xc = (x1+x2)/2;
	yc = (y1+y2)/2;
#endif

	vfe_dev_dbg("af zone input xc=%d,yc=%d\n", xc, yc);

	if (x1 > x2 || y1 > y2 ||
		 xc > info->width || yc > info->height) {
		vfe_dev_dbg("invalid af win![%d,%d][%d,%d] prv[%d/%d]\n", x1, y1, x2, y2, prv_x, prv_y);
		return -EINVAL;
	}

	if (info->focus_status == 1) /* can not set af zone when focus is busy */
	return 0;

	xc = (xc * 80 * 2 / info->width + 1) / 2;
	if ((info->width == HD720_WIDTH && info->height == HD720_HEIGHT) || \
	 (info->width == HD1080_WIDTH && info->height == HD1080_HEIGHT)) {
		yc = (yc * 45 * 2 / info->height + 1) / 2;
	} else {
		yc = (yc * 60 * 2 / info->height + 1) / 2;
	}

	vfe_dev_dbg("af zone after xc=%d,yc=%d\n", xc, yc);

	/* set x center */
	ret = sensor_write(sd, 0x3024, xc);
	if (ret < 0) {
		vfe_dev_err("sensor_s_af_zone_xc error!\n");
		return ret;
	}
	/* set y center */
	ret = sensor_write(sd, 0x3025, yc);
	if (ret < 0) {
		vfe_dev_err("sensor_s_af_zone_yc error!\n");
		return ret;
	}

	ret = sensor_write(sd, 0x3023, 0x01);
	/* set af zone */
	ret |= sensor_write(sd, 0x3022, 0x81);
	if (ret < 0) {
		vfe_dev_err("sensor_s_af_zone error!\n");
		return ret;
	}
	/* msleep(5); */
	sensor_s_relaunch_af_zone(sd);
	return 0;
}

static int sensor_s_3a_lock(struct v4l2_subdev *sd, int value)
{
	int ret;

	value =  !((value&V4L2_LOCK_FOCUS)>>2);
	if (value == 0)
		ret = sensor_s_pause_af(sd);
	else
		ret = sensor_s_relaunch_af_zone(sd);

	return ret;
}

#if 1
static int sensor_s_sharpness_auto(struct v4l2_subdev *sd)
{
	data_type rdval;

	sensor_read(sd, 0x5308, &rdval);
	sensor_write(sd, 0x5308, rdval&0xbf);
	return sensor_write_array(sd, sensor_sharpness_auto_regs, ARRAY_SIZE(sensor_sharpness_auto_regs));
}
#endif

static int sensor_s_sharpness_value(struct v4l2_subdev *sd, data_type value)
{
	data_type rdval;

	sensor_read(sd, 0x5308, &rdval);
	sensor_write(sd, 0x5308, rdval|0x40);
	return sensor_write(sd, 0x5302, value);
}

#if 1
static int sensor_s_denoise_auto(struct v4l2_subdev *sd)
{
	data_type rdval;

	sensor_read(sd, 0x5308, &rdval);
	sensor_write(sd, 0x5308, rdval&0xef);
	return sensor_write_array(sd, sensor_denoise_auto_regs, ARRAY_SIZE(sensor_denoise_auto_regs));
}
#endif

static int sensor_s_denoise_value(struct v4l2_subdev *sd, data_type value)
{
	data_type rdval;

	sensor_read(sd, 0x5308, &rdval);
	sensor_write(sd, 0x5308, rdval|0x10);
	return sensor_write(sd, 0x5306, value);
}

/* *********************************************begin of ******************************************** */

static int sensor_g_hflip(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);
	data_type rdval;

	LOG_ERR_RET(sensor_read(sd, 0x3821, &rdval))

	rdval &= (1<<1);
	rdval >>= 1;

	*value = rdval;

	info->hflip = *value;
	return 0;
}

static int sensor_s_hflip(struct v4l2_subdev *sd, int value)
{
	struct sensor_info *info = to_state(sd);
	data_type rdval;

	if (info->hflip == value)
	return 0;

	LOG_ERR_RET(sensor_read(sd, 0x3821, &rdval))

	switch (value) {
	case 0:
		rdval &= 0xf9;
		break;
	case 1:
		rdval |= 0x06;
		break;
	default:
		return -EINVAL;
	}

	LOG_ERR_RET(sensor_write(sd, 0x3821, rdval))

	usleep_range(10000, 12000);
	info->hflip = value;
	return 0;
}

static int sensor_g_vflip(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);
	data_type rdval;

	LOG_ERR_RET(sensor_read(sd, 0x3820, &rdval))

	rdval &= (1<<1);
	*value = rdval;
	rdval >>= 1;

	info->vflip = *value;
	return 0;
}

static int sensor_s_vflip(struct v4l2_subdev *sd, int value)
{
	struct sensor_info *info = to_state(sd);
	data_type rdval;

	if (info->vflip == value)
	return 0;

	LOG_ERR_RET(sensor_read(sd, 0x3820, &rdval))

	switch (value) {
	case 0:
		rdval &= 0xf9;
		break;
	case 1:
		rdval |= 0x06;
		break;
	default:
		return -EINVAL;
	}

	LOG_ERR_RET(sensor_write(sd, 0x3820, rdval))

	usleep_range(10000, 12000);
	info->vflip = value;
	return 0;
}

static int sensor_g_autogain(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);
	data_type rdval;

	LOG_ERR_RET(sensor_read(sd, 0x3503, &rdval))

	if ((rdval&0x02) == 0x02)
		*value = 0;
	else
		*value = 1;

	info->autogain = *value;
	return 0;
}

static int sensor_s_autogain(struct v4l2_subdev *sd, int value)
{
	struct sensor_info *info = to_state(sd);
	data_type rdval;

	LOG_ERR_RET(sensor_read(sd, 0x3503, &rdval))

	switch (value) {
	case 0:
		rdval |= 0x02;
		break;
	case 1:
		rdval &= 0xfd;
		break;
	default:
		return -EINVAL;
	}

	LOG_ERR_RET(sensor_write(sd, 0x3503, rdval))

	info->autogain = value;
	return 0;
}

static int sensor_g_autoexp(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);
	data_type rdval;

	LOG_ERR_RET(sensor_read(sd, 0x3503, &rdval))

	if ((rdval&0x01) == 0x01)
		*value = V4L2_EXPOSURE_MANUAL;
	 else
		*value = V4L2_EXPOSURE_AUTO;

	info->autoexp = *value;
	return 0;
}

static int sensor_s_autoexp(struct v4l2_subdev *sd,
					enum v4l2_exposure_auto_type value)
{
	struct sensor_info *info = to_state(sd);
	data_type rdval;

	LOG_ERR_RET(sensor_read(sd, 0x3503, &rdval))

	switch (value) {
	case V4L2_EXPOSURE_AUTO:
		rdval &= 0xfe;
		break;
	case V4L2_EXPOSURE_MANUAL:
		rdval |= 0x01;
		break;
	case V4L2_EXPOSURE_SHUTTER_PRIORITY:
		return -EINVAL;
	case V4L2_EXPOSURE_APERTURE_PRIORITY:
		return -EINVAL;
	default:
		return -EINVAL;
	}

	LOG_ERR_RET(sensor_write(sd, 0x3503, rdval))

		/* msleep(10); */
	info->autoexp = value;
	return 0;
}

static int sensor_g_autowb(struct v4l2_subdev *sd, int *value)
{
	struct sensor_info *info = to_state(sd);
	data_type rdval;

	LOG_ERR_RET(sensor_read(sd, 0x3406, &rdval))

	rdval &= (1<<1);
	rdval = rdval>>1;   /* 0x3406 bit0 is awb enable */

	*value = (rdval == 1)?0:1;
	info->autowb = *value;
	return 0;
}

static int sensor_s_autowb(struct v4l2_subdev *sd, int value)
{
	struct sensor_info *info = to_state(sd);
	data_type rdval;

	if (info->autowb == value)
		return 0;

	LOG_ERR_RET(sensor_write_array(sd, sensor_wb_auto_regs, ARRAY_SIZE(sensor_wb_auto_regs)))
	LOG_ERR_RET(sensor_read(sd, 0x3406, &rdval))

	switch (value) {
	case 0:
		rdval |= 0x01;
		break;
	case 1:
		rdval &= 0xfe;
		break;
	default:
		break;
	}

	LOG_ERR_RET(sensor_write(sd, 0x3406, rdval))

	/* msleep(10); */
	info->autowb = value;
	return 0;
}

static int sensor_g_hue(struct v4l2_subdev *sd, __s32 *value)
{
	return -EINVAL;
}

static int sensor_s_hue(struct v4l2_subdev *sd, int value)
{
	return -EINVAL;
}

static int sensor_g_gain(struct v4l2_subdev *sd, __s32 *value)
{
	return -EINVAL;
}

static int sensor_s_gain(struct v4l2_subdev *sd, int value)
{
	return -EINVAL;
}

static int sensor_g_band_filter(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);
	data_type rdval;

	LOG_ERR_RET(sensor_read(sd, 0x3a00, &rdval))

	if ((rdval & (1<<5)) == (1<<5))
		info->band_filter = V4L2_CID_POWER_LINE_FREQUENCY_DISABLED;
	else {
		LOG_ERR_RET(sensor_read(sd, 0x3c00, &rdval))
		if ((rdval & (1<<2)) == (1<<2))
			info->band_filter = V4L2_CID_POWER_LINE_FREQUENCY_50HZ;
		else
			info->band_filter = V4L2_CID_POWER_LINE_FREQUENCY_60HZ;
	}
	return 0;
}

static int sensor_s_band_filter(struct v4l2_subdev *sd,
					enum v4l2_power_line_frequency value)
{
	struct sensor_info *info = to_state(sd);
	data_type rdval;

	if (info->band_filter == value)
		return 0;

	switch (value) {
	case V4L2_CID_POWER_LINE_FREQUENCY_DISABLED:
		LOG_ERR_RET(sensor_read(sd, 0x3a00, &rdval))
		LOG_ERR_RET(sensor_write(sd, 0x3a00, rdval&0xdf))
		break;
	case V4L2_CID_POWER_LINE_FREQUENCY_50HZ:
		LOG_ERR_RET(sensor_write(sd, 0x3c00, 0x04))
		LOG_ERR_RET(sensor_write(sd, 0x3c01, 0x80))
		LOG_ERR_RET(sensor_read(sd, 0x3a00, &rdval))
		LOG_ERR_RET(sensor_write(sd, 0x3a00, rdval|0x20))
		break;
	case V4L2_CID_POWER_LINE_FREQUENCY_60HZ:
		LOG_ERR_RET(sensor_write(sd, 0x3c00, 0x00))
		LOG_ERR_RET(sensor_write(sd, 0x3c01, 0x80))
		LOG_ERR_RET(sensor_read(sd, 0x3a00, &rdval))
		LOG_ERR_RET(sensor_write(sd, 0x3a00, rdval|0x20))
		break;
	case V4L2_CID_POWER_LINE_FREQUENCY_AUTO:
		break;
	default:
		break;
	}
	/* msleep(10); */
	info->band_filter = value;
	return 0;
}

/* *********************************************end of ******************************************** */

static int sensor_g_brightness(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);

	*value = info->brightness;
	return 0;
}

static int sensor_s_brightness(struct v4l2_subdev *sd, int value)
{
	struct sensor_info *info = to_state(sd);

	if (info->brightness == value)
		return 0;

	if (value < -4 || value > 4)
		return -ERANGE;

	LOG_ERR_RET(sensor_write_array(sd, sensor_brightness[value+4].regs, sensor_brightness[value+4].size))

	info->brightness = value;
	return 0;
}

static int sensor_g_contrast(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);

	*value = info->contrast;
	return 0;
}

static int sensor_s_contrast(struct v4l2_subdev *sd, int value)
{
	struct sensor_info *info = to_state(sd);

	if (info->contrast == value)
		return 0;

	if (value < -4 || value > 4)
		return -ERANGE;

	LOG_ERR_RET(sensor_write_array(sd, sensor_contrast[value+4].regs, sensor_contrast[value+4].size))

	info->contrast = value;
	return 0;
}

static int sensor_g_saturation(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);

	*value = info->saturation;
	return 0;
}

static int sensor_s_saturation(struct v4l2_subdev *sd, int value)
{
	struct sensor_info *info = to_state(sd);

	if (info->saturation == value)
		return 0;

	if (value < -4 || value > 4)
		return -ERANGE;

	LOG_ERR_RET(sensor_write_array(sd, sensor_saturation[value+4].regs, sensor_saturation[value+4].size))

	info->saturation = value;
	return 0;
}

static int sensor_g_exp_bias(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);

	*value = info->exp_bias;
	return 0;
}

static int sensor_s_exp_bias(struct v4l2_subdev *sd, int value)
{
	struct sensor_info *info = to_state(sd);

	if (info->exp_bias == value)
		return 0;

	if (value < -4 || value > 4)
		return -ERANGE;

	sensor_write(sd, 0x3503, 0x07);
	sensor_get_preview_exposure(sd);
	sensor_write(sd, 0x3503, 0x00);

	LOG_ERR_RET(sensor_write_array(sd, sensor_ev[value+4].regs, sensor_ev[value+4].size))

	info->exp_bias = value;
	return 0;
}

static int sensor_g_wb(struct v4l2_subdev *sd, int *value)
{
	struct sensor_info *info = to_state(sd);
	enum v4l2_auto_n_preset_white_balance *wb_type = (enum v4l2_auto_n_preset_white_balance *)value;

	*wb_type = info->wb;

	return 0;
}

static int sensor_s_wb(struct v4l2_subdev *sd,
				enum v4l2_auto_n_preset_white_balance value)
{
	struct sensor_info *info = to_state(sd);

	if (info->capture_mode == V4L2_MODE_IMAGE)
		return 0;

	if (info->wb == value)
		return 0;
	LOG_ERR_RET(sensor_write_array(sd, sensor_wb[value].regs, sensor_wb[value].size))

	if (value == V4L2_WHITE_BALANCE_AUTO)
		info->autowb = 1;
	else
		info->autowb = 0;

	info->wb = value;
	return 0;
}

static int sensor_g_colorfx(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);
	enum v4l2_colorfx *clrfx_type = (enum v4l2_colorfx *)value;

	*clrfx_type = info->clrfx;
	return 0;
}

static int sensor_s_colorfx(struct v4l2_subdev *sd,  enum v4l2_colorfx value)
{
	struct sensor_info *info = to_state(sd);

	if (info->clrfx == value)
		return 0;

	LOG_ERR_RET(sensor_write_array(sd, sensor_colorfx[value].regs, sensor_colorfx[value].size))

	info->clrfx = value;
	return 0;
}

static int sensor_g_flash_mode(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);
	enum v4l2_flash_led_mode *flash_mode = (enum v4l2_flash_led_mode *)value;

	*flash_mode = info->flash_mode;
	return 0;
}

static int sensor_s_flash_mode(struct v4l2_subdev *sd,
						enum v4l2_flash_led_mode value)
{
	struct sensor_info *info = to_state(sd);

	vfe_dev_dbg("sensor_s_flash_mode[%d]!\n", value);

	info->flash_mode = value;
	return 0;
}

/*
 * Stuff that knows about the sensor.
 */

static int sensor_power(struct v4l2_subdev *sd, int on)
{
	cci_lock(sd);
	switch (on) {
	case CSI_SUBDEV_STBY_ON:
		vfe_dev_dbg("CSI_SUBDEV_STBY_ON\n");
		vfe_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		usleep_range(30000, 31000);
		vfe_set_mclk(sd, OFF);
		break;
	case CSI_SUBDEV_STBY_OFF:
		vfe_dev_dbg("CSI_SUBDEV_STBY_OFF\n");
		vfe_set_mclk_freq(sd, MCLK);
		vfe_set_mclk(sd, ON);
		usleep_range(30000, 31000);
		vfe_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		usleep_range(10000, 12000);
		break;
	case CSI_SUBDEV_PWR_ON:
		vfe_dev_dbg("CSI_SUBDEV_PWR_ON\n");
		vfe_gpio_set_status(sd, PWDN, 1);
		vfe_gpio_set_status(sd, RESET, 1);
		vfe_gpio_set_status(sd, POWER_EN, 1);
		usleep_range(10000, 12000);

		vfe_gpio_write(sd, PWDN, 0);
		vfe_gpio_write(sd, RESET, CSI_GPIO_LOW);
		vfe_gpio_write(sd, POWER_EN, CSI_GPIO_HIGH);




		usleep_range(20000, 22000);
		vfe_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		usleep_range(10000, 12000);
		vfe_set_mclk_freq(sd, MCLK);
		vfe_set_mclk(sd, ON);
		usleep_range(10000, 12000);
		vfe_gpio_write(sd, RESET, CSI_GPIO_LOW);
		usleep_range(30000, 31000);
		vfe_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		usleep_range(30000, 31000);
		break;
	case CSI_SUBDEV_PWR_OFF:
		vfe_dev_dbg("CSI_SUBDEV_PWR_OFF\n");
		usleep_range(10000, 12000);
		vfe_gpio_write(sd, RESET, CSI_GPIO_LOW);
		usleep_range(10000, 12000);
		vfe_set_mclk(sd, OFF);
		vfe_gpio_write(sd, POWER_EN, CSI_GPIO_LOW);




		usleep_range(10000, 12000);

		usleep_range(10000, 12000);
		vfe_gpio_set_status(sd, POWER_EN, 0);
		vfe_gpio_set_status(sd, RESET, 0);

		break;
	default:
		return -EINVAL;
	}
	cci_unlock(sd);
	return 0;
}

static int sensor_reset(struct v4l2_subdev *sd, u32 val)
{
	switch (val) {
	case 0:
		vfe_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		usleep_range(10000, 12000);
		break;
	case 1:
		vfe_gpio_write(sd, RESET, CSI_GPIO_LOW);
		usleep_range(10000, 12000);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sensor_detect(struct v4l2_subdev *sd)
{
	data_type rdval;

	LOG_ERR_RET(sensor_read(sd, 0x300a, &rdval))

	if (rdval != 0x56)
		return -ENODEV;
	LOG_ERR_RET(sensor_read(sd, 0x300b, &rdval))
	if (rdval != 0x40)
		return -ENODEV;
	return 0;
}

static int sensor_init(struct v4l2_subdev *sd, u32 val)
{
	int ret;

	struct sensor_info *info = to_state(sd);
#ifdef _FLASH_FUNC_
	struct vfe_dev *dev = (struct vfe_dev *)dev_get_drvdata(sd->v4l2_dev->dev);
#endif

	vfe_dev_dbg("sensor_init 0x%x\n", val);

	/*Make sure it is a target sensor*/
	ret = sensor_detect(sd);
	if (ret) {
		vfe_dev_err("chip found is not an target chip.\n");
		return ret;
	}

	vfe_get_standby_mode(sd, &info->stby_mode);

	if ((info->stby_mode == HW_STBY || info->stby_mode == SW_STBY) \
		    && info->init_first_flag == 0) {
		vfe_dev_print("stby_mode and init_first_flag = 0\n");
		return 0;
	}
	ogain = 0x28;
	oexposurelow = 0x00;
	oexposuremid = 0x3d;
	oexposurehigh = 0x00;
	info->focus_status = 0;
	info->low_speed = 0;
	info->width = 0;
	info->height = 0;
	info->brightness = 0;
	info->contrast = 0;
	info->saturation = 0;
	info->hue = 0;
	info->hflip = 0;
	info->vflip = 0;
	info->gain = 0;
	info->autogain = 1;
	info->exp_bias = 0;
	info->autoexp = 1;
	info->autowb = 1;
	info->wb = V4L2_WHITE_BALANCE_AUTO;
	info->clrfx = V4L2_COLORFX_NONE;
	info->band_filter = V4L2_CID_POWER_LINE_FREQUENCY_50HZ;

	/* info->af_ctrl = V4L2_AF_RELEASE; */
	info->tpf.numerator = 1;
	info->tpf.denominator = 30;    /* 30fps */

	ret = sensor_write_array(sd, sensor_default_regs, ARRAY_SIZE(sensor_default_regs));
	if (ret < 0) {
		vfe_dev_err("write sensor_default_regs error\n");
		return ret;
	}

	sensor_s_band_filter(sd, V4L2_CID_POWER_LINE_FREQUENCY_50HZ);

	if (info->stby_mode == 0)
		info->init_first_flag = 0;

	info->preview_first_flag = 1;
	night_mode = 0;
	Nfrms = MAX_FRM_CAP;

	if (1 == AE_CW)
		sensor_write_array(sd, ae_centerweight_tbl, ARRAY_SIZE(ae_centerweight_tbl));
	else
		sensor_write_array(sd, ae_average_tbl, ARRAY_SIZE(ae_average_tbl));

#ifdef _FLASH_FUNC_
	if (dev->flash_used == 1)
		sunxi_flash_info_init(dev->flash_sd);
#endif
	return 0;

}

static int sensor_g_exif(struct v4l2_subdev *sd, struct sensor_exif_attribute *exif)
{
	int ret = 0;/* , gain_val, exp_val; */

	exif->fnumber = 220;
	exif->focal_length = 180;
	exif->brightness = 125;
	exif->flash_fire = 0;
	exif->iso_speed = 200;
	exif->exposure_time_num = 1;
	exif->exposure_time_den = 15;
	return ret;
}
static void sensor_s_af_win(struct v4l2_subdev *sd, struct v4l2_win_setting *af_win)
{
	sensor_s_af_zone(sd, &af_win->coor[0]);
}
static void sensor_s_ae_win(struct v4l2_subdev *sd, struct v4l2_win_setting *ae_win)
{

}
static long sensor_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	int ret = 0;

	switch (cmd) {
	case GET_SENSOR_EXIF:
		sensor_g_exif(sd, (struct sensor_exif_attribute *)arg);
		break;
	case SET_AUTO_FOCUS_WIN:
		sensor_s_af_win(sd, (struct v4l2_win_setting *)arg);
		break;
	case SET_AUTO_EXPOSURE_WIN:
		sensor_s_ae_win(sd, (struct v4l2_win_setting *)arg);
		break;
	default:
		return -EINVAL;
	}
	return ret;
}


/*
 * Store information about the video data format.
 */
static struct sensor_format_struct {
	__u8 *desc;
	/* __u32 pixelformat; */
	u32 mbus_code;/* linux-3.0 */
	struct regval_list *regs;
	int	regs_size;
	int bpp;   /* Bytes per pixel */
} sensor_formats[] = {
	{
		.desc		= "YUYV 4:2:2",
		.mbus_code	= MEDIA_BUS_FMT_YUYV8_2X8,/* linux-3.0 */
		.regs		= sensor_fmt_yuv422_yuyv,
		.regs_size = ARRAY_SIZE(sensor_fmt_yuv422_yuyv),
		.bpp		= 2,
	},
	{
		.desc		= "YVYU 4:2:2",
		.mbus_code	= MEDIA_BUS_FMT_YVYU8_2X8,/* linux-3.0 */
		.regs		= sensor_fmt_yuv422_yvyu,
		.regs_size = ARRAY_SIZE(sensor_fmt_yuv422_yvyu),
		.bpp		= 2,
	},
	{
		.desc		= "UYVY 4:2:2",
		.mbus_code	= MEDIA_BUS_FMT_UYVY8_2X8,/* linux-3.0 */
		.regs		= sensor_fmt_yuv422_uyvy,
		.regs_size = ARRAY_SIZE(sensor_fmt_yuv422_uyvy),
		.bpp		= 2,
	},
	{
		.desc		= "VYUY 4:2:2",
		.mbus_code	= MEDIA_BUS_FMT_VYUY8_2X8,/* linux-3.0 */
		.regs		= sensor_fmt_yuv422_vyuy,
		.regs_size = ARRAY_SIZE(sensor_fmt_yuv422_vyuy),
		.bpp		= 2,
	},
/* { */
/* .desc   = "Raw RGB Bayer", */
/* .mbus_code  = V4L2_MBUS_FMT_SBGGR8_1X8, */
/* .regs     = sensor_fmt_raw, */
/* .regs_size = ARRAY_SIZE(sensor_fmt_raw), */
/* .bpp    = 1 */
/* }, */
};
#define N_FMTS ARRAY_SIZE(sensor_formats)



/*
 * Then there is the issue of window sizes.  Try to capture the info here.
 */


static struct sensor_win_size sensor_win_sizes[] = {
  /* 1080P */
  {
    .width      = HD1080_WIDTH,
    .height     = HD1080_HEIGHT,
    .hoffset    = 0,
    .voffset    = 0,
    .regs       = sensor_1080p_regs,
    .regs_size  = ARRAY_SIZE(sensor_1080p_regs),
    .set_size   = NULL,
  },
	/* VGA */
	{
		.width      = VGA_WIDTH,
		.height     = VGA_HEIGHT,
		.hoffset    = 0,
		.voffset    = 0,
		.regs       = NULL,
		.regs_size  = NULL,
		.set_size   = NULL,
	},
};

#define N_WIN_SIZES (ARRAY_SIZE(sensor_win_sizes))

static int sensor_enum_code(struct v4l2_subdev *sd, struct v4l2_subdev_pad_config *cfg,
		 struct v4l2_subdev_mbus_code_enum *code)
{
	if (code->index >= N_FMTS)
		return -EINVAL;

	code->code = sensor_formats[code->index].mbus_code;
	return 0;
}

static int sensor_enum_frame_size(struct v4l2_subdev *sd,
				struct v4l2_subdev_pad_config *cfg,
				struct v4l2_subdev_frame_size_enum *fse)
{
	if (fse->index > N_WIN_SIZES-1)
		return -EINVAL;

	fse->min_width = sensor_win_sizes[fse->index].width;
	fse->max_width = fse->min_width;
	fse->min_height = sensor_win_sizes[fse->index].height;
	fse->max_height = fse->min_height;

	return 0;
}


static int sensor_try_fmt_internal(struct v4l2_subdev *sd,
					struct v4l2_mbus_framefmt *fmt,
					struct sensor_format_struct **ret_fmt,
					struct sensor_win_size **ret_wsize)
{
	int index;
	struct sensor_win_size *wsize;

	for (index = 0; index < N_FMTS; index++)
		if (sensor_formats[index].mbus_code == fmt->code)
			break;

	if (index >= N_FMTS)
		return -EINVAL;

	if (ret_fmt != NULL)
		*ret_fmt = sensor_formats + index;

	/*
	* Fields: the sensor devices claim to be progressive.
	*/
	fmt->field = V4L2_FIELD_NONE;

	/*
	* Round requested image size down to the nearest
	* we support, but not below the smallest.
	*/
	for (wsize = sensor_win_sizes; wsize < sensor_win_sizes + N_WIN_SIZES; wsize++)
		if (fmt->width >= wsize->width && fmt->height >= wsize->height)
			break;

	if (wsize >= sensor_win_sizes + N_WIN_SIZES)
		wsize--;   /* Take the smallest one */
	if (ret_wsize != NULL)
		*ret_wsize = wsize;
	/*
	* Note the size we'll actually handle.
	*/
	fmt->width = wsize->width;
	fmt->height = wsize->height;

	return 0;
}

static int sensor_get_fmt(struct v4l2_subdev *sd,
			struct v4l2_subdev_pad_config *cfg,
			struct v4l2_subdev_format *fmat)
{
	struct v4l2_mbus_framefmt *fmt = &fmat->format;

	return sensor_try_fmt_internal(sd, fmt, NULL, NULL);
}

static int sensor_g_mbus_config(struct v4l2_subdev *sd,
				struct v4l2_mbus_config *cfg)
{
	cfg->type = V4L2_MBUS_PARALLEL;
	cfg->flags = V4L2_MBUS_MASTER | VREF_POL | HREF_POL | CLK_POL;

	return 0;
}

/*
 * Set a format.
 */
static int sensor_set_fmt(struct v4l2_subdev *sd,
		struct v4l2_subdev_pad_config *cfg,
		struct v4l2_subdev_format *fmat)
{
	int ret;
	struct v4l2_mbus_framefmt *fmt = &fmat->format;
	struct sensor_format_struct *sensor_fmt;
	struct sensor_win_size *wsize;
	struct sensor_info *info = to_state(sd);

	vfe_dev_dbg("sensor_set_fmt\n");

	ret = sensor_try_fmt_internal(sd, fmt, &sensor_fmt, &wsize);
	if (ret)
		return ret;

	sensor_write_array(sd, sensor_fmt->regs, sensor_fmt->regs_size);

	/* printk("wsize->regs_size=%d\n", wsize->regs_size); */
	if (wsize->regs)
		LOG_ERR_RET(sensor_write_array(sd, wsize->regs, wsize->regs_size))

	if (wsize->set_size)
		LOG_ERR_RET(wsize->set_size(sd))

	info->fmt = sensor_fmt;
	info->width = wsize->width;
	info->height = wsize->height;
	vfe_dev_print("s_fmt set width = %d, height = %d\n", wsize->width, wsize->height);

sensor_get_pclk(sd);
vfe_dev_print(" ov5640 pclk %d\n", preview_pclk);

	return 0;
}

/*
 * Implement G/S_PARM.  There is a "high quality" mode we could try
 * to do someday; for now, we just do the frame rate tweak.
 */
static int sensor_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	struct v4l2_captureparm *cp = &parms->parm.capture;
	struct sensor_info *info = to_state(sd);

	if (parms->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	memset(cp, 0, sizeof(struct v4l2_captureparm));
	cp->capability = V4L2_CAP_TIMEPERFRAME;
	cp->capturemode = info->capture_mode;

	cp->timeperframe.numerator = info->tpf.numerator;
	cp->timeperframe.denominator = info->tpf.denominator;

	return 0;
}

static int sensor_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	struct v4l2_captureparm *cp = &parms->parm.capture;
	struct v4l2_fract *tpf = &cp->timeperframe;
	struct sensor_info *info = to_state(sd);
	data_type div;

	vfe_dev_dbg("sensor_s_parm\n");

	if (parms->type != V4L2_BUF_TYPE_VIDEO_CAPTURE) {
		vfe_dev_dbg("parms->type!=V4L2_BUF_TYPE_VIDEO_CAPTURE\n");
		return -EINVAL;
	}

	if (info->tpf.numerator == 0) {
		vfe_dev_dbg("info->tpf.numerator == 0\n");
		return -EINVAL;
	}

	info->capture_mode = cp->capturemode;

	if (info->capture_mode == V4L2_MODE_IMAGE) {
		vfe_dev_dbg("capture mode is not video mode,can not set frame rate!\n");
		return 0;
	}

	if (tpf->numerator == 0 || tpf->denominator == 0) {
		tpf->numerator = 1;
		tpf->denominator = SENSOR_FRAME_RATE;/* Reset to full rate */
		vfe_dev_err("sensor frame rate reset to full rate!\n");
	}

	div = SENSOR_FRAME_RATE/(tpf->denominator/tpf->numerator);
	if (div > 15 || div == 0) {
		vfe_dev_print("SENSOR_FRAME_RATE=%d\n", SENSOR_FRAME_RATE);
		vfe_dev_print("tpf->denominator=%d\n", tpf->denominator);
		vfe_dev_print("tpf->numerator=%d\n", tpf->numerator);
		return -EINVAL;
	}

	vfe_dev_dbg("set frame rate %d\n", tpf->denominator/tpf->numerator);

	info->tpf.denominator = SENSOR_FRAME_RATE;
	info->tpf.numerator = div;

	if (info->tpf.denominator/info->tpf.numerator < 30)
		info->low_speed = 1;

	return 0;
}


/*
 * Code for dealing with controls.
 * fill with different sensor module
 * different sensor module has different settings here
 * if not support the follow function ,retrun -EINVAL
 */

/* *********************************************begin of ******************************************** */
static int sensor_queryctrl(struct v4l2_subdev *sd,
		struct v4l2_queryctrl *qc)
{
	/* Fill in min, max, step and default value for these controls. */
	/* see include/linux/videodev2.h for details */
	/* see sensor_s_parm and sensor_g_parm for the meaning of value */

	switch (qc->id) {
/* case V4L2_CID_BRIGHTNESS: */
/* return v4l2_ctrl_query_fill(qc, -4, 4, 1, 1); */
/* case V4L2_CID_CONTRAST: */
/* return v4l2_ctrl_query_fill(qc, -4, 4, 1, 1); */
/* case V4L2_CID_SATURATION: */
/* return v4l2_ctrl_query_fill(qc, -4, 4, 1, 1); */
/* case V4L2_CID_HUE: */
/* return v4l2_ctrl_query_fill(qc, -180, 180, 5, 0); */
	case V4L2_CID_VFLIP:
	case V4L2_CID_HFLIP:
		return v4l2_ctrl_query_fill(qc, 0, 1, 1, 0);
/* case V4L2_CID_GAIN: */
/* return v4l2_ctrl_query_fill(qc, 0, 255, 1, 128); */
/* case V4L2_CID_AUTOGAIN: */
/* return v4l2_ctrl_query_fill(qc, 0, 1, 1, 1); */
	case V4L2_CID_EXPOSURE:
	case V4L2_CID_AUTO_EXPOSURE_BIAS:
		return v4l2_ctrl_query_fill(qc, -4, 4, 1, 0);
	case V4L2_CID_EXPOSURE_AUTO:
		return v4l2_ctrl_query_fill(qc, 0, 1, 1, 0);
	case V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE:
		return v4l2_ctrl_query_fill(qc, 0, 9, 1, 1);
	case V4L2_CID_AUTO_WHITE_BALANCE:
		return v4l2_ctrl_query_fill(qc, 0, 1, 1, 1);
	case V4L2_CID_COLORFX:
		return v4l2_ctrl_query_fill(qc, 0, 15, 1, 0);
	case V4L2_CID_FLASH_LED_MODE:
		return v4l2_ctrl_query_fill(qc, 0, 4, 1, 0);

	case V4L2_CID_3A_LOCK:
		return v4l2_ctrl_query_fill(qc, 0, V4L2_LOCK_FOCUS, 1, 0);
/* case V4L2_CID_AUTO_FOCUS_RANGE: */
/* return v4l2_ctrl_query_fill(qc, 0, 0, 0, 0);//only auto */
	case V4L2_CID_AUTO_FOCUS_INIT:
	case V4L2_CID_AUTO_FOCUS_RELEASE:
	case V4L2_CID_AUTO_FOCUS_START:
	case V4L2_CID_AUTO_FOCUS_STOP:
	case V4L2_CID_AUTO_FOCUS_STATUS:
		return v4l2_ctrl_query_fill(qc, 0, 0, 0, 0);
	case V4L2_CID_FOCUS_AUTO:
		return v4l2_ctrl_query_fill(qc, 0, 1, 1, 0);
  }
  return -EINVAL;
}


static int sensor_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	switch (ctrl->id) {
	case V4L2_CID_BRIGHTNESS:
		return sensor_g_brightness(sd, &ctrl->value);
	case V4L2_CID_CONTRAST:
		return sensor_g_contrast(sd, &ctrl->value);
	case V4L2_CID_SATURATION:
		return sensor_g_saturation(sd, &ctrl->value);
	case V4L2_CID_HUE:
		return sensor_g_hue(sd, &ctrl->value);
	case V4L2_CID_VFLIP:
		return sensor_g_vflip(sd, &ctrl->value);
	case V4L2_CID_HFLIP:
		return sensor_g_hflip(sd, &ctrl->value);
	case V4L2_CID_GAIN:
		return sensor_g_gain(sd, &ctrl->value);
	case V4L2_CID_AUTOGAIN:
		return sensor_g_autogain(sd, &ctrl->value);
	case V4L2_CID_EXPOSURE:
	case V4L2_CID_AUTO_EXPOSURE_BIAS:
		return sensor_g_exp_bias(sd, &ctrl->value);
	case V4L2_CID_EXPOSURE_AUTO:
		return sensor_g_autoexp(sd, &ctrl->value);
	case V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE:
		return sensor_g_wb(sd, &ctrl->value);
	case V4L2_CID_AUTO_WHITE_BALANCE:
		return sensor_g_autowb(sd, &ctrl->value);
	case V4L2_CID_COLORFX:
		return sensor_g_colorfx(sd, &ctrl->value);
	case V4L2_CID_FLASH_LED_MODE:
		return sensor_g_flash_mode(sd, &ctrl->value);
	case V4L2_CID_POWER_LINE_FREQUENCY:
		return sensor_g_band_filter(sd, &ctrl->value);

	case V4L2_CID_3A_LOCK:
		return sensor_g_3a_lock(sd);
/* case V4L2_CID_AUTO_FOCUS_RANGE: */
/* ctrl->value=0;//only auto */
/* return 0; */
/* case V4L2_CID_AUTO_FOCUS_INIT: */
/* case V4L2_CID_AUTO_FOCUS_RELEASE: */
/* case V4L2_CID_AUTO_FOCUS_START: */
/* case V4L2_CID_AUTO_FOCUS_STOP: */
	case V4L2_CID_AUTO_FOCUS_STATUS:
		return sensor_g_af_status(sd);
/* case V4L2_CID_FOCUS_AUTO: */
	}
	return -EINVAL;
}

static int sensor_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	switch (ctrl->id) {
	case V4L2_CID_BRIGHTNESS:
		return sensor_s_brightness(sd, ctrl->value);
	case V4L2_CID_CONTRAST:
		return sensor_s_contrast(sd, ctrl->value);
	case V4L2_CID_SATURATION:
		return sensor_s_saturation(sd, ctrl->value);
	case V4L2_CID_HUE:
		return sensor_s_hue(sd, ctrl->value);
	case V4L2_CID_VFLIP:
		return sensor_s_vflip(sd, ctrl->value);
	case V4L2_CID_HFLIP:
		return sensor_s_hflip(sd, ctrl->value);
	case V4L2_CID_GAIN:
		return sensor_s_gain(sd, ctrl->value);
	case V4L2_CID_AUTOGAIN:
		return sensor_s_autogain(sd, ctrl->value);
	case V4L2_CID_EXPOSURE:
	case V4L2_CID_AUTO_EXPOSURE_BIAS:
		return sensor_s_exp_bias(sd, ctrl->value);
	case V4L2_CID_EXPOSURE_AUTO:
		return sensor_s_autoexp(sd, (enum v4l2_exposure_auto_type) ctrl->value);
	case V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE:
		return sensor_s_wb(sd, (enum v4l2_auto_n_preset_white_balance) ctrl->value);
	case V4L2_CID_AUTO_WHITE_BALANCE:
		return sensor_s_autowb(sd, ctrl->value);
	case V4L2_CID_COLORFX:
		return sensor_s_colorfx(sd, (enum v4l2_colorfx) ctrl->value);
	case V4L2_CID_FLASH_LED_MODE:
		return sensor_s_flash_mode(sd, (enum v4l2_flash_led_mode) ctrl->value);
	case V4L2_CID_POWER_LINE_FREQUENCY:
		return sensor_s_band_filter(sd, (enum v4l2_power_line_frequency) ctrl->value);

	case V4L2_CID_3A_LOCK:
		return sensor_s_3a_lock(sd, ctrl->value);
		/* case V4L2_CID_AUTO_FOCUS_RANGE: */
		/* return 0; */
	case V4L2_CID_AUTO_FOCUS_INIT:
		return sensor_s_init_af(sd);
	case V4L2_CID_AUTO_FOCUS_RELEASE:
		return sensor_s_release_af(sd);
	case V4L2_CID_AUTO_FOCUS_START:
		return sensor_s_single_af(sd);
	case V4L2_CID_AUTO_FOCUS_STOP:
		return sensor_s_pause_af(sd);
		/* case V4L2_CID_AUTO_FOCUS_STATUS: */
	case V4L2_CID_FOCUS_AUTO:
		return sensor_s_continueous_af(sd, ctrl->value);
	default:
		return -EINVAL;
	}
	return -EINVAL;
}

/* ----------------------------------------------------------------------- */
static const struct v4l2_subdev_core_ops sensor_core_ops = {
	.g_ctrl = sensor_g_ctrl,
	.s_ctrl = sensor_s_ctrl,
	.queryctrl = sensor_queryctrl,
	.reset = sensor_reset,
	.init = sensor_init,
	.s_power = sensor_power,
	.ioctl = sensor_ioctl,
};

static const struct v4l2_subdev_video_ops sensor_video_ops = {
	.s_parm = sensor_s_parm,
	.g_parm = sensor_g_parm,
	.g_mbus_config = sensor_g_mbus_config,
};

static const struct v4l2_subdev_pad_ops sensor_pad_ops = {
	.enum_mbus_code = sensor_enum_code,
	.enum_frame_size = sensor_enum_frame_size,
	.get_fmt = sensor_get_fmt,
	.set_fmt = sensor_set_fmt,
};

static const struct v4l2_subdev_ops sensor_ops = {
	.core = &sensor_core_ops,
	.video = &sensor_video_ops,
	.pad = &sensor_pad_ops,
};

/* ----------------------------------------------------------------------- */
static struct cci_driver cci_drv = {
	.name = SENSOR_NAME,
	.addr_width = CCI_BITS_16,
	.data_width = CCI_BITS_8,
};

static int sensor_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct sensor_info *info;

	info = kzalloc(sizeof(struct sensor_info), GFP_KERNEL);
	if (info == NULL)
		return -ENOMEM;
	sd = &info->sd;
	glb_sd = sd;
	cci_dev_probe_helper(sd, client, &sensor_ops, &cci_drv);
	info->fmt = &sensor_formats[0];
	info->af_first_flag = 1;
	info->init_first_flag = 1;
	info->auto_focus = 0;
	return 0;
}


static int sensor_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd;

	sd = cci_dev_remove_helper(client, &cci_drv);
	printk("sensor_remove ov5640 sd = %p!\n", sd);
	kfree(to_state(sd));
	return 0;
}

static const struct i2c_device_id sensor_id[] = {
	{ SENSOR_NAME, 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, sensor_id);

static const struct of_device_id sernsor_match[] = {
		{ .compatible = "allwinner,sensor_ov5640", },
		{},
};


static struct i2c_driver sensor_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = SENSOR_NAME,
		.of_match_table = sernsor_match,
	},
	.probe = sensor_probe,
	.remove = sensor_remove,
	.id_table = sensor_id,
};
static __init int init_sensor(void)
{
#ifdef CONFIG_ARCH_SUN9IW1P1
	A80_VERSION = sunxi_get_soc_ver();/* SUN9IW1P1_REV_B */
	if (A80_VERSION >= SUN9IW1P1_REV_B)
		MCLK_DIV = 1;
	else
		MCLK_DIV = 2;
	printk("A80_VERSION = %d , SUN9IW1P1_REV_B = %d, MCLK_DIV = %d\n", A80_VERSION, SUN9IW1P1_REV_B, MCLK_DIV);
#else
	MCLK_DIV = 1;
#endif
	return cci_dev_init_helper(&sensor_driver);
}

static __exit void exit_sensor(void)
{
	cci_dev_exit_helper(&sensor_driver);
}

module_init(init_sensor);
module_exit(exit_sensor);
