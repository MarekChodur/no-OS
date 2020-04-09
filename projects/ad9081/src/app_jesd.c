/***************************************************************************//**
 *   @file   app_jesd.c
 *   @brief  Application JESD setup.
 *   @author DBogdan (dragos.bogdan@analog.com)
********************************************************************************
 * Copyright 2020(c) Analog Devices, Inc.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *  - Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  - Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  - Neither the name of Analog Devices, Inc. nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *  - The use of this software may or may not infringe the patent rights
 *    of one or more patent holders.  This license does not release you
 *    from the requirement that you obtain separate licenses from these
 *    patent holders to use this software.
 *  - Use of the software either in source or binary form, must be run
 *    on or directly connected to an Analog Devices Inc. component.
 *
 * THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, NON-INFRINGEMENT,
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL ANALOG DEVICES BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, INTELLECTUAL PROPERTY RIGHTS, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/

/******************************************************************************/
/***************************** Include Files **********************************/
/******************************************************************************/

#include "axi_jesd204_rx.h"
#include "axi_jesd204_tx.h"
#include "axi_adxcvr.h"
#include "jesd204_clk.h"
#include "error.h"
#include "app_parameters.h"
#include "app_jesd.h"

/******************************************************************************/
/************************ Variables Definitions *******************************/
/******************************************************************************/
struct axi_jesd204_rx *rx_jesd;
struct axi_jesd204_tx *tx_jesd;

struct adxcvr *rx_adxcvr;
struct adxcvr *tx_adxcvr;

struct jesd204_clk rx_jesd_clk;
struct jesd204_clk tx_jesd_clk;

struct clk_hw jesd_rx_hw;
struct clk_hw jesd_tx_hw;

/******************************************************************************/
/************************** Functions Implementation **************************/
/******************************************************************************/

/**
 * @brief Application JESD setup.
 * @return SUCCESS in case of success, FAILURE otherwise.
 */
int32_t app_jesd_init(struct clk clk[2],
		      uint32_t reference_clk_khz,
		      uint32_t rx_device_clk_khz,
		      uint32_t tx_device_clk_khz,
		      uint32_t rx_lane_clk_khz,
		      uint32_t tx_lane_clk_khz)
{
	int32_t ret;

	struct jesd204_tx_init tx_jesd_init = {
		.name = "tx_jesd",
		.base = TX_JESD_BASEADDR,
		.octets_per_frame = 4,
		.frames_per_multiframe = 32,
		.converters_per_device = 8,
		.converter_resolution = 16,
		.bits_per_sample = 16,
		.high_density = false,
		.control_bits_per_sample = 0,
		.subclass = 1,
		.device_clk_khz = tx_device_clk_khz,
		.lane_clk_khz = tx_lane_clk_khz
	};

	struct jesd204_rx_init rx_jesd_init = {
		.name = "rx_jesd",
		.base = RX_JESD_BASEADDR,
		.octets_per_frame = 4,
		.frames_per_multiframe = 32,
		.subclass = 1,
		.device_clk_khz = rx_device_clk_khz,
		.lane_clk_khz = rx_lane_clk_khz
	};

	struct adxcvr_init tx_adxcvr_init = {
		"tx_adxcvr",
		TX_XCVR_BASEADDR,
		3,
		4,
		0,
		0,
		tx_lane_clk_khz,
		reference_clk_khz,
	};

	struct adxcvr_init rx_adxcvr_init = {
		.name = "rx_adxcvr",
		.base = RX_XCVR_BASEADDR,
		.sys_clk_sel = 0,
		.out_clk_sel = 4,
		.cpll_enable = 1,
		.lpm_enable = 1,
		.lane_rate_khz = rx_lane_clk_khz,
		.ref_rate_khz = reference_clk_khz,
	};

	ret = adxcvr_init(&tx_adxcvr, &tx_adxcvr_init);
	if (ret)
		return ret;
	ret = adxcvr_init(&rx_adxcvr, &rx_adxcvr_init);
	if (ret)
		return ret;

	ret = axi_jesd204_tx_init(&tx_jesd, &tx_jesd_init);
	if (ret)
		return ret;
	ret = axi_jesd204_rx_init(&rx_jesd, &rx_jesd_init);
	if (ret)
		return ret;

	rx_jesd_clk.xcvr = rx_adxcvr;
	rx_jesd_clk.jesd_rx = rx_jesd;
	tx_jesd_clk.xcvr = tx_adxcvr;
	tx_jesd_clk.jesd_tx = tx_jesd;

	jesd_rx_hw.dev = &rx_jesd_clk;
	jesd_rx_hw.dev_clk_enable = jesd204_clk_enable;
	jesd_rx_hw.dev_clk_disable = jesd204_clk_disable;
	jesd_rx_hw.dev_clk_set_rate = jesd204_clk_set_rate;

	jesd_tx_hw.dev = &tx_jesd_clk;
	jesd_tx_hw.dev_clk_enable = jesd204_clk_enable;
	jesd_tx_hw.dev_clk_disable = jesd204_clk_disable;
	jesd_tx_hw.dev_clk_set_rate = jesd204_clk_set_rate;

	clk[0].name = "jesd_rx";
	clk[0].hw = &jesd_rx_hw;

	clk[1].name = "jesd_tx";
	clk[1].hw = &jesd_tx_hw;

	return SUCCESS;
}
