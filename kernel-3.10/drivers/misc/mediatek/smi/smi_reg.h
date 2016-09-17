#ifndef _SMI_REG_H
#define _SMI_REG_H

#if defined(SMI_RO) || defined(SMI_K2)
#include "smi_reg_v2.h"
#elif defined(SMI_82) || defined(SMI_92)
#include "smi_reg_v1.h"
#endif	/*  */

#endif
