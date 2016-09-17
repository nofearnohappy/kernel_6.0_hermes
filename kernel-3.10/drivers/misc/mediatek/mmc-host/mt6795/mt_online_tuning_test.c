static int mt_ot_stress_test(struct msdc_host *host, u32 rawcmd, u32 rawarg, u8 *error, u8 rw)
{
    u32 base = host->base;
    struct mmc_command *cmd = host->cmd;
    u32 gear_window_size = OT_START_TUNEWND;
    u32 i;
    
    u32 CRCStatus_CMD=0;

    u32 intsts;
    u32 goldenCRC = 0xFFFFFFFF >> (32 - gear_window_size * 2 - 1);

	pr_debug("[SDIO_DVT]Enter %s\n", __func__);

    //sdr_write32(SDIO_TUNE_WIND, gear_window_size);	// Set gear window size
    sdr_write32(MSDC_DAT_RDDLY0, 0x00000000);			// Set dat delay
    
    for (i=0; i<2000; i++) {
        if (ot_do_command(host, cmd, rawcmd, rawarg, &intsts)) {
			*error = 1;
			return -1;
		}
		
		/* CMD CRC status */
		CRCStatus_CMD = sdr_read32(CMD_TUNE_CRC);
		if (CRCStatus_CMD != goldenCRC) {
			pr_debug("[SDIO_DVT]CMD CRC error, CRC status: 0x%x\n", CRCStatus_CMD);
		    *error = 1;
		}
		
		/* DAT CRC status */
		if (cmd->resp[0] != goldenCRC | 
		    cmd->resp[1] != goldenCRC |
		    cmd->resp[2] != goldenCRC |
		    cmd->resp[3] != goldenCRC ) {
			pr_debug("[SDIO_DVT]DAT CRC error\n");
			pr_debug("[SDIO_DVT]DAT0 CRC status: 0x%x\n", cmd->resp[0]);
			pr_debug("[SDIO_DVT]DAT1 CRC status: 0x%x\n", cmd->resp[1]);
			pr_debug("[SDIO_DVT]DAT2 CRC status: 0x%x\n", cmd->resp[2]);
			pr_debug("[SDIO_DVT]DAT3 CRC status: 0x%x\n", cmd->resp[3]);
		    *error = 1;
		}
    }
    
    return 0;
}

static int mt_ot_tmo_test(struct msdc_host *host, u32 rawcmd, u32 rawarg, u8 *error, u8 rw)
{
	u32 base = host->base;
	struct mmc_command *cmd = host->cmd;
	
	u8 ACMDTMO = 0;
	u8 DATTMO = 0;
	u8 ACMD53Done = 0;

	u32 intsts;

	pr_debug("[SDIO_DVT][%d] Enter mt_ot_tmo_test\n", rw);
	if(ot_do_command(host, cmd, rawcmd, rawarg, &intsts))
	{
		*error = 1;
		//return -1;
	}

	sdr_get_field(MSDC_INT, MSDC_INT_ACMDTMO, ACMDTMO);	// Get ACMD TMO flag
	if(ACMDTMO)
	{
		pr_debug("[SDIO_DVT][%d] ACMD TMO\n", rw);
		*error = 1;
	}
	sdr_get_field(MSDC_INT, MSDC_INT_DATTMO, DATTMO);	// Get DAT TMO flag
	if(DATTMO)
	{
		pr_debug("[SDIO_DVT][%d] DAT TMO\n", rw);
		*error = 1;
	}

	/* AUTOCMD53_DONE test */
	if(ACMDTMO | DATTMO)
	{
		sdr_get_field(MSDC_INT, MSDC_INT_ACMD53_DONE, ACMD53Done);	// Get ACMD53 done flag
		if(ACMD53Done == 1)
		{
			pr_debug("[SDIO_DVT][%d] AUTOCMD53_DONE status error\n", rw);
			*error = 1;
		}
	}

	pr_debug("[SDIO_DVT][%d] Leave mt_ot_tmo_test\n", rw);

	return 0;
}

static int mt_ot_gearRegister_test(struct msdc_host *host, u8 rw, u32 eco_ver, u8 *error)
{
	u32 base = host->base;
	
	u32 cmdrdly;
	u32 rxdly0;
	u32 tmpdly;
	u32 dat0rddly, dat1rddly, dat2rddly, dat3rddly;
	u32 gear_window_size;

	u32 CRCStatus_CMD=0;
	u8 CRCErr = 0;
	u8 ACMD53Err = 0;
	u8 ACMD53Done = 0;
	u8 GearOutBound = 0;

	pr_debug("[SDIO_DVT]Enter mt_ot_otRegister_test\n");

	//char *rw = i_rw == 0 ? "Data Read" : "Data Write";

	/* test all delay settings */
    for(cmdrdly = 0; cmdrdly < 32; cmdrdly++)
    {
    	/* Register PAD_CMD_RXDLY test */
		sdr_set_field(MSDC_PAD_TUNE, MSDC_PAD_TUNE_CMDRDLY, cmdrdly);	// Set cmd delay

		if(cmdrdly < 32)
		{
			sdr_get_field(MSDC_PAD_TUNE, MSDC_PAD_TUNE_CMDRDLY, tmpdly);	// Get cmd delay

			if(tmpdly != cmdrdly)
			{
				pr_debug("[SDIO_DVT][%d] PAD_CMD_RXDLY, write = %d, read = %d\n", rw, cmdrdly, tmpdly);
				*error = 1;
			}
		}
		else
		{
			sdr_get_field(MSDC_PAD_TUNE, MSDC_PAD_TUNE_CMDRDLY, tmpdly);	// Get cmd delay
			pr_debug("[SDIO_DVT][%d] cmdrdly = 32, PAD_CMD_RXDLY value = %d\n", rw, tmpdly);
		}
    }

	dat0rddly = 0;
	dat1rddly = 0;
	dat2rddly = 0;
	dat3rddly = 0;

	for(; dat0rddly < 32; dat0rddly++)
	{
		if (eco_ver >= 4) {
			rxdly0 = (dat0rddly << 24) | (dat1rddly << 16) | (dat2rddly << 8) | (dat3rddly << 0);
		} else {   
			rxdly0 = (dat0rddly << 0) | (dat1rddly << 8) | (dat2rddly << 16) | (dat3rddly << 24);
		}

		/* Register DAT_RD_DLY0 test */
		sdr_write32(MSDC_DAT_RDDLY0, rxdly0);			// Set dat delay
		
		if(dat0rddly < 32)
		{
			tmpdly = sdr_read32(MSDC_DAT_RDDLY0);			// Read dat delay
			if(tmpdly != rxdly0)
			{
				pr_debug("[SDIO_DVT][%d] write=%d,read=%d,cmddly=%d\n", rw, rxdly0, tmpdly, cmdrdly);
				*error = 1;
			}
		}
		else
		{
			tmpdly = sdr_read32(MSDC_DAT_RDDLY0);			// Read dat delay
			pr_debug("[SDIO_DVT][%d] dat0rddly = 32, DAT_RD_DLY0 value = %d\n", rw, tmpdly);
		}
	}

	dat0rddly = 0;

	for(; dat1rddly < 32; dat1rddly++)
	{
		if (eco_ver >= 4) {
			rxdly0 = (dat0rddly << 24) | (dat1rddly << 16) | (dat2rddly << 8) | (dat3rddly << 0);
		} else {   
			rxdly0 = (dat0rddly << 0) | (dat1rddly << 8) | (dat2rddly << 16) | (dat3rddly << 24);
		}

		/* Register DAT_RD_DLY0 test */
		sdr_write32(MSDC_DAT_RDDLY0, rxdly0);			// Set dat delay
		
		if(dat1rddly < 32)
		{
			tmpdly = sdr_read32(MSDC_DAT_RDDLY0);			// Read dat delay
			if(tmpdly != rxdly0)
			{
				pr_debug("[SDIO_DVT][%d] write=%d,read=%d,cmddly=%d\n", rw, rxdly0, tmpdly, cmdrdly);
				*error = 1;
			}
		}
		else
		{
			tmpdly = sdr_read32(MSDC_DAT_RDDLY0);			// Read dat delay
			pr_debug("[SDIO_DVT][%d] dat1rddly = 32, DAT_RD_DLY0 value = %d\n", rw, tmpdly);
		}
	}

	dat1rddly = 0;

	for(; dat2rddly < 32; dat2rddly++)
	{
		if (eco_ver >= 4) {
			rxdly0 = (dat0rddly << 24) | (dat1rddly << 16) | (dat2rddly << 8) | (dat3rddly << 0);
		} else {   
			rxdly0 = (dat0rddly << 0) | (dat1rddly << 8) | (dat2rddly << 16) | (dat3rddly << 24);
		}

		/* Register DAT_RD_DLY0 test */
		sdr_write32(MSDC_DAT_RDDLY0, rxdly0);			// Set dat delay
		
		if(dat2rddly < 32)
		{
			tmpdly = sdr_read32(MSDC_DAT_RDDLY0);			// Read dat delay
			if(tmpdly != rxdly0)
			{
				pr_debug("[SDIO_DVT][%d] write=%d,read=%d,cmddly=%d\n", rw, rxdly0, tmpdly, cmdrdly);
				*error = 1;
			}
		}
		else
		{
			tmpdly = sdr_read32(MSDC_DAT_RDDLY0);			// Read dat delay
			pr_debug("[SDIO_DVT][%d] dat2rddly = 32, DAT_RD_DLY0 value = %d\n", rw, tmpdly);
		}
	}

	dat2rddly = 0;

	for(; dat3rddly < 32; dat3rddly++)
	{
		if (eco_ver >= 4) {
			rxdly0 = (dat0rddly << 24) | (dat1rddly << 16) | (dat2rddly << 8) | (dat3rddly << 0);
		} else {   
			rxdly0 = (dat0rddly << 0) | (dat1rddly << 8) | (dat2rddly << 16) | (dat3rddly << 24);
		}

		/* Register DAT_RD_DLY0 test */
		sdr_write32(MSDC_DAT_RDDLY0, rxdly0);			// Set dat delay
		
		if(dat3rddly < 32)
		{
			tmpdly = sdr_read32(MSDC_DAT_RDDLY0);			// Read dat delay
			if(tmpdly != rxdly0)
			{
				pr_debug("[SDIO_DVT][%d]write=%d,read=%d,cmddly=%d\n", rw, rxdly0, tmpdly, cmdrdly);
				*error = 1;
			}
		}
		else
		{
			tmpdly = sdr_read32(MSDC_DAT_RDDLY0);			// Read dat delay
			pr_debug("[SDIO_DVT][%d] dat3rddly = 32, DAT_RD_DLY0 value = %d\n", rw, tmpdly);
		}
	}

	dat3rddly = 0;

	/* test all gear window size */
	for(gear_window_size = 0; gear_window_size < 16; gear_window_size++)
	{
		/* Register SDIO_TUNE_WIND test */
		sdr_write32(SDIO_TUNE_WIND, gear_window_size);	// Set gear window size
		tmpdly = sdr_read32(SDIO_TUNE_WIND);			// Read gear window size
		if(tmpdly != gear_window_size)
		{
			pr_debug("[SDIO_DVT][%d]w=%d,r=%d,c=%d,d=%d\n", rw, gear_window_size, tmpdly, cmdrdly, rxdly0);
			*error = 1;
		}
	}

	pr_debug("[SDIO_DVT]Leave mt_ot_otRegister_test\n");
	
	return 0;
}

static int mt_ot_outbound_test(struct msdc_host *host, u32 rawcmd, u32 rawarg, u8 i_rw, u32 eco_ver, u8 *error)
{
	
	u32 base = host->base;
	struct mmc_command *cmd = host->cmd;
		
	u32 cmdrdly[20] = {31,30,15,0 ,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15};
	u32 rxdly0;
	u32 tmpdly;
	u32 dat0rddly[20] = {15,15,15,15,31,30,15,0 ,15,15,15,15,15,15,15,15,15,15,15,15};
	u32 dat1rddly[20] = {15,15,15,15,15,15,15,15,31,30,15,0 ,15,15,15,15,15,15,15,15};
	u32 dat2rddly[20] = {15,15,15,15,15,15,15,15,15,15,15,15,31,30,15,0 ,15,15,15,15};
	u32 dat3rddly[20] = {15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,31,30,15,0 };
	u32 gear_window_size = 7;
	u32 i;

	u32 CRCStatus_CMD=0;
	u32 CRCErr = 0;
	u32 ACMD53Err = 0;
	u32 ACMD53Done = 0;
	u32 GearOutBound = 0;

	u32 intsts;

	//char *rw = i_rw == 0 ? "Data Read" : "Data Write";

	pr_debug("[SDIO_DVT]Enter mt_ot_outbound_test\n");
	if(cmd == NULL)
	{
		pr_debug("[SDIO_DVT]host->cmd is NULL\n");
		*error = 1;
		return -1;
	}

	sdr_write32(SDIO_TUNE_WIND, gear_window_size);	// Set gear window size
	
	for(i=0; i<20; i++)
	{
		sdr_set_field(MSDC_PAD_TUNE, MSDC_PAD_TUNE_CMDRDLY, cmdrdly[i]);	// Set cmd delay

		if (eco_ver >= 4) {
			rxdly0 = (dat0rddly[i] << 24) | (dat1rddly[i] << 16) | (dat2rddly[i] << 8) | (dat3rddly[i] << 0);
		} else {   
			rxdly0 = (dat0rddly[i] << 0) | (dat1rddly[i] << 8) | (dat2rddly[i] << 16) | (dat3rddly[i] << 24);
		}
		sdr_write32(MSDC_DAT_RDDLY0, rxdly0);			// Set dat delay

		if(ot_do_command(host, cmd, rawcmd, rawarg, &intsts))
		{
			pr_debug("[SDIO_DVT][%d] crdly=%d,dly=%d,dly=%d,dly=%d, dly=%d,gear_window_size = %d\n\n"
				, i_rw, cmdrdly[i], dat0rddly[i], dat1rddly[i]
				, dat2rddly[i], dat3rddly[i], gear_window_size);
			*error = 1;
			return -1;
		}

		//sdr_get_field(MSDC_INT, MSDC_INT_GRAE_OUT_BOUND, GearOutBound);	// Get grae out of bound flag
		GearOutBound = (intsts & MSDC_INT_GEAR_OUT_BOUND) >> 20;
		if(cmdrdly[i] > 31 || dat0rddly[i] > 31 || dat1rddly[i] > 31 || dat2rddly[i] > 31 || dat3rddly[i] > 31)
		{
			if(GearOutBound == 0)
			{
				pr_debug("[SDIO_DVT]intsts = 0x%x, GearOutBound = %d\n", intsts, GearOutBound);
				pr_debug("[SDIO_DVT][%d] GEAR_OUT_BOUND status error\n", i_rw);
				pr_debug("[SDIO_DVT][%d] cdly = %d,dly=%d,dly=%d,dly=%d,dly=%d,win_size = %d\n\n"
				, i_rw, cmdrdly[i], dat0rddly[i], dat1rddly[i]
				, dat2rddly[i], dat3rddly[i], gear_window_size);
				*error = 1;
			}
		}
		else if(cmdrdly[i] + gear_window_size / 2 > 31 || cmdrdly[i] < gear_window_size / 2 ||
				dat0rddly[i] + gear_window_size / 2 > 31 || dat0rddly[i] < gear_window_size / 2 ||
				dat1rddly[i] + gear_window_size / 2 > 31 || dat1rddly[i] < gear_window_size / 2 ||
				dat2rddly[i] + gear_window_size / 2 > 31 || dat2rddly[i] < gear_window_size / 2 ||
				dat3rddly[i] + gear_window_size / 2 > 31 || dat3rddly[i] < gear_window_size / 2)
		{
			if(GearOutBound == 0)
			{
				pr_debug("[SDIO_DVT]intsts = 0x%x, GearOutBound = %d\n", intsts, GearOutBound);
				pr_debug("[SDIO_DVT][%d] GEAR_OUT_BOUND status error\n", i_rw);
				pr_debug("[SDIO_DVT][%d] cdly=%d,dly=%d,dly=%d,dly=%d,dly=%d,win_size=%d\n\n"
					, i_rw, cmdrdly[i], dat0rddly[i], dat1rddly[i]
					, dat2rddly[i], dat3rddly[i], gear_window_size);
				*error = 1;
			}
		}
		else
		{
			if(GearOutBound == 1)
			{
				pr_debug("[SDIO_DVT]intsts = 0x%x, GearOutBound = %d\n", intsts, GearOutBound);
				pr_debug("[SDIO_DVT][%d] GEAR_OUT_BOUND status error\n", i_rw);
				pr_debug("[SDIO_DVT][%d] cdly=%d,dly=%d,dly=%d,dly=%d,dly=%d,winw_size=%d\n\n"
					, i_rw, cmdrdly[i], dat0rddly[i], dat1rddly[i]
					, dat2rddly[i], dat3rddly[i], gear_window_size);
				*error = 1;
			}
		}

	}

	pr_debug("[SDIO_DVT]Leave mt_ot_outbound_test\n");

	return 0;
}

static int mt_ot_crcerr_acmd53_test(struct msdc_host *host, u32 rawcmd, u32 rawarg, u8 rw, u32 eco_ver, u8 *error)
{
	
	u32 base = host->base;
	struct mmc_command *cmd = host->cmd;
		
	u32 cmdrdly[20] = {31,31,15,0 ,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	u32 rxdly0;
	u32 tmpdly;
	u32 dat0rddly[20] = {0,0,0,0,31,31,15,0 ,0,0,0,0,0,0,0,0,0,0,0,0};
	u32 dat1rddly[20] = {0,0,0,0,0,0,0,0,31,31,15,0 ,0,0,0,0,0,0,0,0};
	u32 dat2rddly[20] = {0,0,0,0,0,0,0,0,0,0,0,0,31,31,15,0 ,0,0,0,0};
	u32 dat3rddly[20] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,31,31,15,0 };
	u32 gear_window_size = 15;
	u32 i;

	u32 CRCStatus_CMD=0;
	u32 CRCErr = 0;
	u32 ACMD53Err = 0;
	u32 ACMD53Done = 0;
	u32 GearOutBound = 0;

	u32 intsts;
	u32 goldenCRC = 0xFFFFFFFF >> (32 - gear_window_size * 2 - 1);

	//u32 temp0, temp1, temp2;

	//char *rw = i_rw == 0 ? "Data Read" : "Data Write";

	pr_debug("[SDIO_DVT]Enter %s\n", __func__);
	/* pr_debug("[SDIO_DVT]goldenCRC = 0x%x\n", goldenCRC); */

	sdr_write32(SDIO_TUNE_WIND, gear_window_size);	// Set gear window size
	
	for (i=0; i<20; i++) {
		sdr_set_field(MSDC_PAD_TUNE, MSDC_PAD_TUNE_CMDRDLY, cmdrdly[i]);	// Set cmd delay

		if (eco_ver >= 4) {
			rxdly0 = (dat0rddly[i] << 24) | (dat1rddly[i] << 16) | (dat2rddly[i] << 8) | (dat3rddly[i] << 0);
		} else {   
			rxdly0 = (dat0rddly[i] << 0) | (dat1rddly[i] << 8) | (dat2rddly[i] << 16) | (dat3rddly[i] << 24);
		}
		sdr_write32(MSDC_DAT_RDDLY0, rxdly0);			// Set dat delay

		if (ot_do_command(host, cmd, rawcmd, rawarg, &intsts)) {
			*error = 1;
			return -1;
		}

	
		/* CMD CRC status */
		CRCStatus_CMD = sdr_read32(CMD_TUNE_CRC);
		if(CRCStatus_CMD != goldenCRC)
		{
			//sdr_get_field(MSDC_INT, MSDC_INT_ACMDCRCERR, CRCErr);	// Get cmd crc err
			//sdr_get_field(MSDC_INT, MSDC_INT_ACMD53_FAIL, ACMD53Err);	// Get ACMD53 error flag
			CRCErr = (intsts & MSDC_INT_RSPCRCERR) >> 10;	// Get cmd crc err
			ACMD53Err = (intsts & MSDC_INT_ACMD53_FAIL) >> 22;	// Get ACMD53 error flag
			#if 1
			if(CRCErr == 0)
			{
				pr_debug("[SDIO_DVT]intsts = 0x%x\n", intsts);
				*error = 1;
			}
			#endif
			if(ACMD53Err == 0)
			{
				pr_debug("[SDIO_DVT]intsts = 0x%x\n", intsts);
				*error = 1;
			}
		}
		else
		{

			CRCErr = (intsts & MSDC_INT_RSPCRCERR) >> 10;	// Get cmd crc err
			ACMD53Err = (intsts & MSDC_INT_ACMD53_FAIL) >> 22;	// Get ACMD53 error flag
			if(CRCErr)
			{
				pr_debug("[SDIO_DVT]intsts = 0x%x\n", intsts);
				*error = 1;
			}
			if(ACMD53Err)
			{
				pr_debug("[SDIO_DVT]intsts = 0x%x\n", intsts);
				*error = 1;
			}
		}

		/* DAT CRC status */
		if(cmd->resp[0] != goldenCRC | 
		   cmd->resp[1] != goldenCRC |
		   cmd->resp[2] != goldenCRC |
		   cmd->resp[3] != goldenCRC )
		{
			//sdr_get_field(MSDC_INT, MSDC_INT_DATCRCERR, CRCErr);	// Get dat crc err
			//sdr_get_field(MSDC_INT, MSDC_INT_ACMD53_FAIL, ACMD53Err);	// Get ACMD53 error flag
			CRCErr = (intsts & MSDC_INT_DATCRCERR) >> 15;	// Get dat crc err
			ACMD53Err = (intsts & MSDC_INT_ACMD53_FAIL) >> 22;	// Get ACMD53 error flag
			if(CRCErr == 0)
			{
				pr_debug("[SDIO_DVT]intsts = 0x%x\n", intsts);
				*error = 1;
			}
			if(ACMD53Err == 0)
			{
				pr_debug("[SDIO_DVT]intsts = 0x%x\n", intsts);
				*error = 1;
			}
		}
		else
		{
			CRCErr = (intsts & MSDC_INT_DATCRCERR) >> 15;	// Get dat crc err
			ACMD53Err = (intsts & MSDC_INT_ACMD53_FAIL) >> 22;	// Get ACMD53 error flag
			if(CRCErr)
			{
				pr_debug("[SDIO_DVT]intsts = 0x%x\n", intsts);
				*error = 1;
			}
			if(ACMD53Err && (CRCStatus_CMD == goldenCRC))
			{
				pr_debug("[SDIO_DVT]intsts = 0x%x\n", intsts);
				*error = 1;
			}
		}

		/* AUTOCMD53_DONE test */
		//sdr_get_field(MSDC_INT, MSDC_INT_ACMD53_DONE, ACMD53Done);	// Get ACMD53 done flag
		ACMD53Done = (intsts & MSDC_INT_ACMD53_DONE) >> 21;	// Get ACMD53 done flag
		if(ACMD53Done == 0)
		{
			pr_debug("[SDIO_DVT]intsts = 0x%x\n", intsts);
			*error = 1;
		}
	}

	pr_debug("[SDIO_DVT]Leave %s\n", __func__);

	return 0;
}

static int mt_ot_composite_test(struct msdc_host *host, u32 rawcmd, u32 rawarg, u8 *error,  char *xferMode, u8 rw, u32 eco_ver)
{
	u8 nRet = 0;
#if 1
	/*======TMO test======*/
	nRet = mt_ot_tmo_test(host, rawcmd, rawarg, error, rw);
	
	if(nRet || *error)
	{
		pr_debug("[SDIO_DVT][%s] TMO test(%d) fail\n", xferMode, rw);
		goto result;
	}

	pr_debug("[SDIO_DVT][%s] TMO test(%d) pass\n", xferMode, rw);
	
	/*======Gear register test======*/
	nRet = mt_ot_gearRegister_test(host, rw, eco_ver, error);

	if(nRet || *error)
	{
		pr_debug("[SDIO_DVT][%s] gear register test fail\n", xferMode);
		goto result;
	}

	pr_debug("[SDIO_DVT][%s] gear register test pass\n", xferMode);
#endif
#if 1
	/*======Gear out bound test======*/
	nRet = mt_ot_outbound_test(host, rawcmd, rawarg, rw, eco_ver, error);

	if(nRet || *error)
	{
		pr_debug("[SDIO_DVT][%s] gear out bound test(%d) fail\n", xferMode, rw);
		goto result;
	}

	pr_debug("[SDIO_DVT][%s] gear out bound test(%d) pass\n", xferMode, rw);
#endif
#if 1
	/*======CRC Err and ACMD53 test======*/
	nRet = mt_ot_crcerr_acmd53_test(host, rawcmd, rawarg, rw, eco_ver, error);

	if(nRet || *error)
	{
		pr_debug("[SDIO_DVT][%s] CRC Err and ACMD53 test(%d) fail\n", xferMode, rw);
		goto result;
	}

	pr_debug("[SDIO_DVT][%s] CRC Err and ACMD53 test(%d) pass\n", xferMode, rw);
#endif
#if 1
    /*======AUTOCMD53 stress test======*/
    nRet = mt_ot_stress_test(host, rawcmd, rawarg, error, rw);
	
	if(nRet || *error)
	{
		pr_debug("[SDIO_DVT][%s] stress test(%d) fail\n", xferMode, rw);
		goto result;
	}

	pr_debug("[SDIO_DVT][%s] stress test(%d) pass\n", xferMode, rw);
#endif
result:
	if(*error == 0)
		pr_debug("[SDIO_DVT][%s] Online tuning composite test(%d) Pass!!!\n", xferMode, rw);
	else
		pr_debug("[SDIO_DVT][%s] Online tuning composite test(%d) Fail!!!\n", xferMode, rw);

	return 0;
}

u8 mt_ot_test_buf[2048];

int mt_msdc_online_tuning_test(struct msdc_host *host, u32 rawcmd, u32 rawarg, u8 rw)
{
	u32 base = host->base;

	u32 default_cmdrdly;
	u32 default_rxdly0;
	u32 default_fifo_data;

	u8 error = 0;
	u8 nRet = 0;
	
	u32 eco_ver;
	int dir = 0;
	
	struct mmc_host *mmc = host->mmc;
    struct ot_data otData;
    struct mmc_command *orig_cmd = host->cmd;
    struct mmc_command cmd;
    struct mmc_data *orig_data = host->data;
    struct mmc_data data = {0};
	struct scatterlist sg;
	
	pr_debug("[SDIO_DVT]Enter online tuning test\n");
	
	/* ungate clock */
    msdc_ungate_clock(host);  // set sw flag 
	
	/* Claim host */
    mmc_claim_host(mmc);
	
	host->cmd = &cmd;
	host->data = &data;
    host->blksz = OT_BLK_SIZE;
	
	otData.fn = 1;
	otData.addr = 0x00B0;
	otData.tune_wind_size = OT_START_TUNEWND;
	ot_init(host, &otData);
	rawcmd = otData.rawcmd;
	rawarg = otData.rawarg;
    eco_ver = otData.eco_ver;
    
    data.blksz = OT_BLK_SIZE;
    data.blocks = 1;
    data.flags = rw ? MMC_DATA_WRITE : MMC_DATA_READ;
    data.sg = &sg;
	data.sg_len = 1;
	
	sg_init_one(&sg, mt_ot_test_buf, data.blksz * data.blocks);
	
	/* Get current delay settings */
    sdr_get_field(MSDC_PAD_TUNE, MSDC_PAD_TUNE_CMDRDLY, default_cmdrdly);	// Get cmd delay
	pr_debug("[SDIO_DVT]default cmd delay = %d\n", default_cmdrdly);
    
    default_rxdly0 = sdr_read32(MSDC_DAT_RDDLY0);
	pr_debug("[SDIO_DVT]default DAT read delay line 0 = %x\n", default_rxdly0);

#if 1
	/*======PIO test======*/
	nRet = mt_ot_composite_test(host, rawcmd, rawarg, &error, "PIO", rw, eco_ver);
	if(nRet || error)
	{
		pr_debug("[SDIO_DVT]PIO test(%d) fail\n", rw);
		goto result;
	}

	pr_debug("[SDIO_DVT]PIO test pass\n");

#endif
#if 1
	/*======Basic DMA test======*/
	msdc_dma_on();  /* enable DMA mode first!! */
	init_completion(&host->xfer_done);
	
	if(rw == 0)
		dir = DMA_FROM_DEVICE;
	else
		dir = DMA_TO_DEVICE;
	
	(void)dma_map_sg(mmc_dev(host->mmc), host->data->sg, host->data->sg_len, dir);
	msdc_dma_setup(host, &host->dma, host->data->sg, host->data->sg_len);  
	
	nRet = mt_ot_composite_test(host, rawcmd, rawarg, &error, "Basic DMA", rw, eco_ver);

	//msdc_dma_off();     
    //host->dma.used_bd  = 0;
    //host->dma.used_gpd = 0;
    //dma_unmap_sg(mmc_dev(host->mmc), host->data->sg, host->data->sg_len, dir);
				
	if(nRet || error)
	{
		pr_debug("[SDIO_DVT]Basic DMA test(%d) fail\n", rw);
		goto result;
	}

	pr_debug("[SDIO_DVT]Basic DMA test pass\n");
#endif
#if 1
	/*======Descriptor DMA test======*/
	msdc_dma_on();  /* enable DMA mode first!! */
	init_completion(&host->xfer_done);
	host->data->sg_len = 1;
	
	if(rw == 0)
		dir = DMA_FROM_DEVICE;
	else
		dir = DMA_TO_DEVICE;
	
	(void)dma_map_sg(mmc_dev(host->mmc), host->data->sg, host->data->sg_len, dir);
	msdc_dma_setup(host, &host->dma, host->data->sg, host->data->sg_len);  
	
	nRet = mt_ot_composite_test(host, rawcmd, rawarg, &error, "Descriptor DMA", rw, eco_ver);

	//msdc_dma_off();     
    //host->dma.used_bd  = 0;
    //host->dma.used_gpd = 0;
    //dma_unmap_sg(mmc_dev(host->mmc), host->data->sg, host->data->sg_len, dir);
	
	if(nRet || error)
	{
		pr_debug("[SDIO_DVT]Descriptor DMA test(%d) fail\n", rw);
		goto result;
	}
	
	pr_debug("[SDIO_DVT]Descriptor DMA test pass\n");
#endif
#if 0
	/*======Enhanced DMA test======*/
	msdc_dma_on();  /* enable DMA mode first!! */
	init_completion(&host->xfer_done);
	host->data->sg_len = 2;	// data->sg_len = 1 is basic dma
	host->dma_xfer = 2;	// host->dma_xfer = 2 for enhanced dma
	
	if(rw == 0)
		dir = DMA_FROM_DEVICE;
	else
		dir = DMA_TO_DEVICE;
	
	(void)dma_map_sg(mmc_dev(host->mmc), host->data->sg, host->data->sg_len, dir);
	msdc_dma_setup(host, &host->dma, host->data->sg, host->data->sg_len);  
	host->dma.gpd->extlen = 0xC;
	host->dma.gpd->arg = rawarg;
	host->dma.gpd->cmd = rawcmd;
	
	nRet = mt_ot_composite_test(host, rawcmd, rawarg, &error, "Enhanced DMA", rw, eco_ver);

	//msdc_dma_off();     
    //host->dma.used_bd  = 0;
    //host->dma.used_gpd = 0;
    //dma_unmap_sg(mmc_dev(host->mmc), host->data->sg, host->data->sg_len, dir);
	
	if(nRet || error)
	{
		pr_debug("[SDIO_DVT]Enhanced DMA test(%s) fail\n", rw == 0 ? "Read" : "Write");
		goto result;
	}
	
	pr_debug("[SDIO_DVT]Enhanced DMA test pass\n");
#endif	


	
result:
	if(error == 0)
		pr_debug("[SDIO_DVT]Online tuning test Pass!!!\n");
	else
		pr_debug("[SDIO_DVT]Online tuning test Fail!!!\n");

	sdr_set_field(MSDC_PAD_TUNE, MSDC_PAD_TUNE_CMDRDLY, default_cmdrdly);	// Set default cmd delay
	sdr_write32(MSDC_DAT_RDDLY0, default_rxdly0);							// Set default dat delay
	
	ot_deinit(host, otData);
    
    host->cmd = orig_cmd;
    host->data = orig_data;
    
    /* release host */
    mmc_release_host(mmc);
    
    /* gate clock */
    msdc_gate_clock(host, 1); // clear flag. 

	pr_debug("[SDIO_DVT]Leave online tuning test\n");

	return 0;
}

static int msdc_ottest_proc_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    return count;
}

static char mt_ot_cmd_buf[256];

static int msdc_ottest_proc_write(struct file *file, const char *buf, unsigned long count, void *data)
{
    struct msdc_host *host = NULL;
    int ret;
    int id;
    
    ret = copy_from_user(mt_ot_cmd_buf, buf, count);
	if (ret < 0)
        return -1;

	mt_ot_cmd_buf[count] = '\0';
	pr_debug("[SDIO_DVT]ottest Write %s\n", mt_ot_cmd_buf);
	
	sscanf(mt_ot_cmd_buf, "%x", &id);

    host = mtk_msdc_host[id];
    
	pr_debug("[SDIO_DVT][%s] Start Online Tuning DVT test\n", __func__);
    mt_msdc_online_tuning_test(host, 0, 0, 0);
	pr_debug("[SDIO_DVT][%s] Finish Online Tuning DVT test\n", __func__);

    return count;
}

int msdc_ottest_proc_init(void)
{
    struct proc_dir_entry *prEntry;
    
    prEntry = create_proc_entry("msdc_ottest", 0660, 0);
    
    if(prEntry)
    {
       prEntry->read_proc  = msdc_ottest_proc_read;
       prEntry->write_proc = msdc_ottest_proc_write;
	pr_debug("[%s]: successfully create /proc/msdc_ottest\n", __func__);
    }else{
	pr_debug("[%s]: failed to create /proc/msdc_ottest\n", __func__);
    }
    
    return 0;
}
