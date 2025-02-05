#include "cpu-custom.h"

ARM64SysReg arm64_id_regs[NUM_ID_IDX];

void initialize_cpu_sysreg_properties(void)
{
    memset(arm64_id_regs, 0, sizeof(ARM64SysReg) * NUM_ID_IDX);

    /* ID_PFR0_EL1 */
    ARM64SysReg *ID_PFR0_EL1 = arm64_sysreg_get(ID_PFR0_EL1_IDX);
    ID_PFR0_EL1->name = "ID_PFR0_EL1";
    arm64_sysreg_add_field(ID_PFR0_EL1, "RAS", 28, 31);
    arm64_sysreg_add_field(ID_PFR0_EL1, "DIT", 24, 27);
    arm64_sysreg_add_field(ID_PFR0_EL1, "AMU", 20, 23);
    arm64_sysreg_add_field(ID_PFR0_EL1, "CSV2", 16, 19);
    arm64_sysreg_add_field(ID_PFR0_EL1, "State3", 12, 15);
    arm64_sysreg_add_field(ID_PFR0_EL1, "State2", 8, 11);
    arm64_sysreg_add_field(ID_PFR0_EL1, "State1", 4, 7);
    arm64_sysreg_add_field(ID_PFR0_EL1, "State0", 0, 3);

    /* ID_PFR1_EL1 */
    ARM64SysReg *ID_PFR1_EL1 = arm64_sysreg_get(ID_PFR1_EL1_IDX);
    ID_PFR1_EL1->name = "ID_PFR1_EL1";
    arm64_sysreg_add_field(ID_PFR1_EL1, "GIC", 28, 31);
    arm64_sysreg_add_field(ID_PFR1_EL1, "Virt_frac", 24, 27);
    arm64_sysreg_add_field(ID_PFR1_EL1, "Sec_frac", 20, 23);
    arm64_sysreg_add_field(ID_PFR1_EL1, "GenTimer", 16, 19);
    arm64_sysreg_add_field(ID_PFR1_EL1, "Virtualization", 12, 15);
    arm64_sysreg_add_field(ID_PFR1_EL1, "MProgMod", 8, 11);
    arm64_sysreg_add_field(ID_PFR1_EL1, "Security", 4, 7);
    arm64_sysreg_add_field(ID_PFR1_EL1, "ProgMod", 0, 3);

    /* ID_DFR0_EL1 */
    ARM64SysReg *ID_DFR0_EL1 = arm64_sysreg_get(ID_DFR0_EL1_IDX);
    ID_DFR0_EL1->name = "ID_DFR0_EL1";
    arm64_sysreg_add_field(ID_DFR0_EL1, "TraceFilt", 28, 31);
    arm64_sysreg_add_field(ID_DFR0_EL1, "PerfMon", 24, 27);
    arm64_sysreg_add_field(ID_DFR0_EL1, "MProfDbg", 20, 23);
    arm64_sysreg_add_field(ID_DFR0_EL1, "MMapTrc", 16, 19);
    arm64_sysreg_add_field(ID_DFR0_EL1, "CopTrc", 12, 15);
    arm64_sysreg_add_field(ID_DFR0_EL1, "MMapDbg", 8, 11);
    arm64_sysreg_add_field(ID_DFR0_EL1, "CopSDbg", 4, 7);
    arm64_sysreg_add_field(ID_DFR0_EL1, "CopDbg", 0, 3);

    /* ID_AFR0_EL1 */
    ARM64SysReg *ID_AFR0_EL1 = arm64_sysreg_get(ID_AFR0_EL1_IDX);
    ID_AFR0_EL1->name = "ID_AFR0_EL1";
    arm64_sysreg_add_field(ID_AFR0_EL1, "IMPDEF3", 12, 15);
    arm64_sysreg_add_field(ID_AFR0_EL1, "IMPDEF2", 8, 11);
    arm64_sysreg_add_field(ID_AFR0_EL1, "IMPDEF1", 4, 7);
    arm64_sysreg_add_field(ID_AFR0_EL1, "IMPDEF0", 0, 3);

    /* ID_MMFR0_EL1 */
    ARM64SysReg *ID_MMFR0_EL1 = arm64_sysreg_get(ID_MMFR0_EL1_IDX);
    ID_MMFR0_EL1->name = "ID_MMFR0_EL1";
    arm64_sysreg_add_field(ID_MMFR0_EL1, "InnerShr", 28, 31);
    arm64_sysreg_add_field(ID_MMFR0_EL1, "FCSE", 24, 27);
    arm64_sysreg_add_field(ID_MMFR0_EL1, "AuxReg", 20, 23);
    arm64_sysreg_add_field(ID_MMFR0_EL1, "TCM", 16, 19);
    arm64_sysreg_add_field(ID_MMFR0_EL1, "ShareLvl", 12, 15);
    arm64_sysreg_add_field(ID_MMFR0_EL1, "OuterShr", 8, 11);
    arm64_sysreg_add_field(ID_MMFR0_EL1, "PMSA", 4, 7);
    arm64_sysreg_add_field(ID_MMFR0_EL1, "VMSA", 0, 3);

    /* ID_MMFR1_EL1 */
    ARM64SysReg *ID_MMFR1_EL1 = arm64_sysreg_get(ID_MMFR1_EL1_IDX);
    ID_MMFR1_EL1->name = "ID_MMFR1_EL1";
    arm64_sysreg_add_field(ID_MMFR1_EL1, "BPred", 28, 31);
    arm64_sysreg_add_field(ID_MMFR1_EL1, "L1TstCln", 24, 27);
    arm64_sysreg_add_field(ID_MMFR1_EL1, "L1Uni", 20, 23);
    arm64_sysreg_add_field(ID_MMFR1_EL1, "L1Hvd", 16, 19);
    arm64_sysreg_add_field(ID_MMFR1_EL1, "L1UniSW", 12, 15);
    arm64_sysreg_add_field(ID_MMFR1_EL1, "L1HvdSW", 8, 11);
    arm64_sysreg_add_field(ID_MMFR1_EL1, "L1UniVA", 4, 7);
    arm64_sysreg_add_field(ID_MMFR1_EL1, "L1HvdVA", 0, 3);

    /* ID_MMFR2_EL1 */
    ARM64SysReg *ID_MMFR2_EL1 = arm64_sysreg_get(ID_MMFR2_EL1_IDX);
    ID_MMFR2_EL1->name = "ID_MMFR2_EL1";
    arm64_sysreg_add_field(ID_MMFR2_EL1, "HWAccFlg", 28, 31);
    arm64_sysreg_add_field(ID_MMFR2_EL1, "WFIStall", 24, 27);
    arm64_sysreg_add_field(ID_MMFR2_EL1, "MemBarr", 20, 23);
    arm64_sysreg_add_field(ID_MMFR2_EL1, "UniTLB", 16, 19);
    arm64_sysreg_add_field(ID_MMFR2_EL1, "HvdTLB", 12, 15);
    arm64_sysreg_add_field(ID_MMFR2_EL1, "L1HvdRng", 8, 11);
    arm64_sysreg_add_field(ID_MMFR2_EL1, "L1HvdBG", 4, 7);
    arm64_sysreg_add_field(ID_MMFR2_EL1, "L1HvdFG", 0, 3);

    /* ID_MMFR3_EL1 */
    ARM64SysReg *ID_MMFR3_EL1 = arm64_sysreg_get(ID_MMFR3_EL1_IDX);
    ID_MMFR3_EL1->name = "ID_MMFR3_EL1";
    arm64_sysreg_add_field(ID_MMFR3_EL1, "Supersec", 28, 31);
    arm64_sysreg_add_field(ID_MMFR3_EL1, "CMemSz", 24, 27);
    arm64_sysreg_add_field(ID_MMFR3_EL1, "CohWalk", 20, 23);
    arm64_sysreg_add_field(ID_MMFR3_EL1, "PAN", 16, 19);
    arm64_sysreg_add_field(ID_MMFR3_EL1, "MaintBcst", 12, 15);
    arm64_sysreg_add_field(ID_MMFR3_EL1, "BPMaint", 8, 11);
    arm64_sysreg_add_field(ID_MMFR3_EL1, "CMaintSW", 4, 7);
    arm64_sysreg_add_field(ID_MMFR3_EL1, "CMaintVA", 0, 3);

    /* ID_ISAR0_EL1 */
    ARM64SysReg *ID_ISAR0_EL1 = arm64_sysreg_get(ID_ISAR0_EL1_IDX);
    ID_ISAR0_EL1->name = "ID_ISAR0_EL1";
    arm64_sysreg_add_field(ID_ISAR0_EL1, "Divide", 24, 27);
    arm64_sysreg_add_field(ID_ISAR0_EL1, "Debug", 20, 23);
    arm64_sysreg_add_field(ID_ISAR0_EL1, "Coproc", 16, 19);
    arm64_sysreg_add_field(ID_ISAR0_EL1, "CmpBranch", 12, 15);
    arm64_sysreg_add_field(ID_ISAR0_EL1, "BitField", 8, 11);
    arm64_sysreg_add_field(ID_ISAR0_EL1, "BitCount", 4, 7);
    arm64_sysreg_add_field(ID_ISAR0_EL1, "Swap", 0, 3);

    /* ID_ISAR1_EL1 */
    ARM64SysReg *ID_ISAR1_EL1 = arm64_sysreg_get(ID_ISAR1_EL1_IDX);
    ID_ISAR1_EL1->name = "ID_ISAR1_EL1";
    arm64_sysreg_add_field(ID_ISAR1_EL1, "Jazelle", 28, 31);
    arm64_sysreg_add_field(ID_ISAR1_EL1, "Interwork", 24, 27);
    arm64_sysreg_add_field(ID_ISAR1_EL1, "Immediate", 20, 23);
    arm64_sysreg_add_field(ID_ISAR1_EL1, "IfThen", 16, 19);
    arm64_sysreg_add_field(ID_ISAR1_EL1, "Extend", 12, 15);
    arm64_sysreg_add_field(ID_ISAR1_EL1, "Except_AR", 8, 11);
    arm64_sysreg_add_field(ID_ISAR1_EL1, "Except", 4, 7);
    arm64_sysreg_add_field(ID_ISAR1_EL1, "Endian", 0, 3);

    /* ID_ISAR2_EL1 */
    ARM64SysReg *ID_ISAR2_EL1 = arm64_sysreg_get(ID_ISAR2_EL1_IDX);
    ID_ISAR2_EL1->name = "ID_ISAR2_EL1";
    arm64_sysreg_add_field(ID_ISAR2_EL1, "Reversal", 28, 31);
    arm64_sysreg_add_field(ID_ISAR2_EL1, "PSR_AR", 24, 27);
    arm64_sysreg_add_field(ID_ISAR2_EL1, "MultU", 20, 23);
    arm64_sysreg_add_field(ID_ISAR2_EL1, "MultS", 16, 19);
    arm64_sysreg_add_field(ID_ISAR2_EL1, "Mult", 12, 15);
    arm64_sysreg_add_field(ID_ISAR2_EL1, "MultiAccessInt", 8, 11);
    arm64_sysreg_add_field(ID_ISAR2_EL1, "MemHint", 4, 7);
    arm64_sysreg_add_field(ID_ISAR2_EL1, "LoadStore", 0, 3);

    /* ID_ISAR3_EL1 */
    ARM64SysReg *ID_ISAR3_EL1 = arm64_sysreg_get(ID_ISAR3_EL1_IDX);
    ID_ISAR3_EL1->name = "ID_ISAR3_EL1";
    arm64_sysreg_add_field(ID_ISAR3_EL1, "T32EE", 28, 31);
    arm64_sysreg_add_field(ID_ISAR3_EL1, "TrueNOP", 24, 27);
    arm64_sysreg_add_field(ID_ISAR3_EL1, "T32Copy", 20, 23);
    arm64_sysreg_add_field(ID_ISAR3_EL1, "TabBranch", 16, 19);
    arm64_sysreg_add_field(ID_ISAR3_EL1, "SynchPrim", 12, 15);
    arm64_sysreg_add_field(ID_ISAR3_EL1, "SVC", 8, 11);
    arm64_sysreg_add_field(ID_ISAR3_EL1, "SIMD", 4, 7);
    arm64_sysreg_add_field(ID_ISAR3_EL1, "Saturate", 0, 3);

    /* ID_ISAR4_EL1 */
    ARM64SysReg *ID_ISAR4_EL1 = arm64_sysreg_get(ID_ISAR4_EL1_IDX);
    ID_ISAR4_EL1->name = "ID_ISAR4_EL1";
    arm64_sysreg_add_field(ID_ISAR4_EL1, "SWP_frac", 28, 31);
    arm64_sysreg_add_field(ID_ISAR4_EL1, "PSR_M", 24, 27);
    arm64_sysreg_add_field(ID_ISAR4_EL1, "SynchPrim_frac", 20, 23);
    arm64_sysreg_add_field(ID_ISAR4_EL1, "Barrier", 16, 19);
    arm64_sysreg_add_field(ID_ISAR4_EL1, "SMC", 12, 15);
    arm64_sysreg_add_field(ID_ISAR4_EL1, "Writeback", 8, 11);
    arm64_sysreg_add_field(ID_ISAR4_EL1, "WithShifts", 4, 7);
    arm64_sysreg_add_field(ID_ISAR4_EL1, "Unpriv", 0, 3);

    /* ID_ISAR5_EL1 */
    ARM64SysReg *ID_ISAR5_EL1 = arm64_sysreg_get(ID_ISAR5_EL1_IDX);
    ID_ISAR5_EL1->name = "ID_ISAR5_EL1";
    arm64_sysreg_add_field(ID_ISAR5_EL1, "VCMA", 28, 31);
    arm64_sysreg_add_field(ID_ISAR5_EL1, "RDM", 24, 27);
    arm64_sysreg_add_field(ID_ISAR5_EL1, "CRC32", 16, 19);
    arm64_sysreg_add_field(ID_ISAR5_EL1, "SHA2", 12, 15);
    arm64_sysreg_add_field(ID_ISAR5_EL1, "SHA1", 8, 11);
    arm64_sysreg_add_field(ID_ISAR5_EL1, "AES", 4, 7);
    arm64_sysreg_add_field(ID_ISAR5_EL1, "SEVL", 0, 3);

    /* ID_ISAR6_EL1 */
    ARM64SysReg *ID_ISAR6_EL1 = arm64_sysreg_get(ID_ISAR6_EL1_IDX);
    ID_ISAR6_EL1->name = "ID_ISAR6_EL1";
    arm64_sysreg_add_field(ID_ISAR6_EL1, "I8MM", 24, 27);
    arm64_sysreg_add_field(ID_ISAR6_EL1, "BF16", 20, 23);
    arm64_sysreg_add_field(ID_ISAR6_EL1, "SPECRES", 16, 19);
    arm64_sysreg_add_field(ID_ISAR6_EL1, "SB", 12, 15);
    arm64_sysreg_add_field(ID_ISAR6_EL1, "FHM", 8, 11);
    arm64_sysreg_add_field(ID_ISAR6_EL1, "DP", 4, 7);
    arm64_sysreg_add_field(ID_ISAR6_EL1, "JSCVT", 0, 3);

    /* ID_MMFR4_EL1 */
    ARM64SysReg *ID_MMFR4_EL1 = arm64_sysreg_get(ID_MMFR4_EL1_IDX);
    ID_MMFR4_EL1->name = "ID_MMFR4_EL1";
    arm64_sysreg_add_field(ID_MMFR4_EL1, "EVT", 28, 31);
    arm64_sysreg_add_field(ID_MMFR4_EL1, "CCIDX", 24, 27);
    arm64_sysreg_add_field(ID_MMFR4_EL1, "LSM", 20, 23);
    arm64_sysreg_add_field(ID_MMFR4_EL1, "HPDS", 16, 19);
    arm64_sysreg_add_field(ID_MMFR4_EL1, "CnP", 12, 15);
    arm64_sysreg_add_field(ID_MMFR4_EL1, "XNX", 8, 11);
    arm64_sysreg_add_field(ID_MMFR4_EL1, "AC2", 4, 7);
    arm64_sysreg_add_field(ID_MMFR4_EL1, "SpecSEI", 0, 3);

    /* MVFR0_EL1 */
    ARM64SysReg *MVFR0_EL1 = arm64_sysreg_get(MVFR0_EL1_IDX);
    MVFR0_EL1->name = "MVFR0_EL1";
    arm64_sysreg_add_field(MVFR0_EL1, "FPRound", 28, 31);
    arm64_sysreg_add_field(MVFR0_EL1, "FPShVec", 24, 27);
    arm64_sysreg_add_field(MVFR0_EL1, "FPSqrt", 20, 23);
    arm64_sysreg_add_field(MVFR0_EL1, "FPDivide", 16, 19);
    arm64_sysreg_add_field(MVFR0_EL1, "FPTrap", 12, 15);
    arm64_sysreg_add_field(MVFR0_EL1, "FPDP", 8, 11);
    arm64_sysreg_add_field(MVFR0_EL1, "FPSP", 4, 7);
    arm64_sysreg_add_field(MVFR0_EL1, "SIMDReg", 0, 3);

    /* MVFR1_EL1 */
    ARM64SysReg *MVFR1_EL1 = arm64_sysreg_get(MVFR1_EL1_IDX);
    MVFR1_EL1->name = "MVFR1_EL1";
    arm64_sysreg_add_field(MVFR1_EL1, "SIMDFMAC", 28, 31);
    arm64_sysreg_add_field(MVFR1_EL1, "FPHP", 24, 27);
    arm64_sysreg_add_field(MVFR1_EL1, "SIMDHP", 20, 23);
    arm64_sysreg_add_field(MVFR1_EL1, "SIMDSP", 16, 19);
    arm64_sysreg_add_field(MVFR1_EL1, "SIMDInt", 12, 15);
    arm64_sysreg_add_field(MVFR1_EL1, "SIMDLS", 8, 11);
    arm64_sysreg_add_field(MVFR1_EL1, "FPDNaN", 4, 7);
    arm64_sysreg_add_field(MVFR1_EL1, "FPFtZ", 0, 3);

    /* MVFR2_EL1 */
    ARM64SysReg *MVFR2_EL1 = arm64_sysreg_get(MVFR2_EL1_IDX);
    MVFR2_EL1->name = "MVFR2_EL1";
    arm64_sysreg_add_field(MVFR2_EL1, "FPMisc", 4, 7);
    arm64_sysreg_add_field(MVFR2_EL1, "SIMDMisc", 0, 3);

    /* ID_PFR2_EL1 */
    ARM64SysReg *ID_PFR2_EL1 = arm64_sysreg_get(ID_PFR2_EL1_IDX);
    ID_PFR2_EL1->name = "ID_PFR2_EL1";
    arm64_sysreg_add_field(ID_PFR2_EL1, "RAS_frac", 8, 11);
    arm64_sysreg_add_field(ID_PFR2_EL1, "SSBS", 4, 7);
    arm64_sysreg_add_field(ID_PFR2_EL1, "CSV3", 0, 3);

    /* ID_DFR1_EL1 */
    ARM64SysReg *ID_DFR1_EL1 = arm64_sysreg_get(ID_DFR1_EL1_IDX);
    ID_DFR1_EL1->name = "ID_DFR1_EL1";
    arm64_sysreg_add_field(ID_DFR1_EL1, "HPMN0", 4, 7);
    arm64_sysreg_add_field(ID_DFR1_EL1, "MTPMU", 0, 3);

    /* ID_MMFR5_EL1 */
    ARM64SysReg *ID_MMFR5_EL1 = arm64_sysreg_get(ID_MMFR5_EL1_IDX);
    ID_MMFR5_EL1->name = "ID_MMFR5_EL1";
    arm64_sysreg_add_field(ID_MMFR5_EL1, "nTLBPA", 4, 7);
    arm64_sysreg_add_field(ID_MMFR5_EL1, "ETS", 0, 3);

    /* ID_AA64PFR0_EL1 */
    ARM64SysReg *ID_AA64PFR0_EL1 = arm64_sysreg_get(ID_AA64PFR0_EL1_IDX);
    ID_AA64PFR0_EL1->name = "ID_AA64PFR0_EL1";
    arm64_sysreg_add_field(ID_AA64PFR0_EL1, "CSV3", 60, 63);
    arm64_sysreg_add_field(ID_AA64PFR0_EL1, "CSV2", 56, 59);
    arm64_sysreg_add_field(ID_AA64PFR0_EL1, "RME", 52, 55);
    arm64_sysreg_add_field(ID_AA64PFR0_EL1, "DIT", 48, 51);
    arm64_sysreg_add_field(ID_AA64PFR0_EL1, "AMU", 44, 47);
    arm64_sysreg_add_field(ID_AA64PFR0_EL1, "MPAM", 40, 43);
    arm64_sysreg_add_field(ID_AA64PFR0_EL1, "SEL2", 36, 39);
    arm64_sysreg_add_field(ID_AA64PFR0_EL1, "SVE", 32, 35);
    arm64_sysreg_add_field(ID_AA64PFR0_EL1, "RAS", 28, 31);
    arm64_sysreg_add_field(ID_AA64PFR0_EL1, "GIC", 24, 27);
    arm64_sysreg_add_field(ID_AA64PFR0_EL1, "AdvSIMD", 20, 23);
    arm64_sysreg_add_field(ID_AA64PFR0_EL1, "AdvSIMD", 20, 23);
    arm64_sysreg_add_field(ID_AA64PFR0_EL1, "FP", 16, 19);
    arm64_sysreg_add_field(ID_AA64PFR0_EL1, "FP", 16, 19);
    arm64_sysreg_add_field(ID_AA64PFR0_EL1, "EL3", 12, 15);
    arm64_sysreg_add_field(ID_AA64PFR0_EL1, "EL2", 8, 11);
    arm64_sysreg_add_field(ID_AA64PFR0_EL1, "EL1", 4, 7);
    arm64_sysreg_add_field(ID_AA64PFR0_EL1, "EL0", 0, 3);

    /* ID_AA64PFR1_EL1 */
    ARM64SysReg *ID_AA64PFR1_EL1 = arm64_sysreg_get(ID_AA64PFR1_EL1_IDX);
    ID_AA64PFR1_EL1->name = "ID_AA64PFR1_EL1";
    arm64_sysreg_add_field(ID_AA64PFR1_EL1, "PFAR", 60, 63);
    arm64_sysreg_add_field(ID_AA64PFR1_EL1, "DF2", 56, 59);
    arm64_sysreg_add_field(ID_AA64PFR1_EL1, "MTEX", 52, 55);
    arm64_sysreg_add_field(ID_AA64PFR1_EL1, "THE", 48, 51);
    arm64_sysreg_add_field(ID_AA64PFR1_EL1, "GCS", 44, 47);
    arm64_sysreg_add_field(ID_AA64PFR1_EL1, "MTE_frac", 40, 43);
    arm64_sysreg_add_field(ID_AA64PFR1_EL1, "NMI", 36, 39);
    arm64_sysreg_add_field(ID_AA64PFR1_EL1, "CSV2_frac", 32, 35);
    arm64_sysreg_add_field(ID_AA64PFR1_EL1, "RNDR_trap", 28, 31);
    arm64_sysreg_add_field(ID_AA64PFR1_EL1, "SME", 24, 27);
    arm64_sysreg_add_field(ID_AA64PFR1_EL1, "MPAM_frac", 16, 19);
    arm64_sysreg_add_field(ID_AA64PFR1_EL1, "RAS_frac", 12, 15);
    arm64_sysreg_add_field(ID_AA64PFR1_EL1, "MTE", 8, 11);
    arm64_sysreg_add_field(ID_AA64PFR1_EL1, "SSBS", 4, 7);
    arm64_sysreg_add_field(ID_AA64PFR1_EL1, "BT", 0, 3);

    /* ID_AA64PFR2_EL1 */
    ARM64SysReg *ID_AA64PFR2_EL1 = arm64_sysreg_get(ID_AA64PFR2_EL1_IDX);
    ID_AA64PFR2_EL1->name = "ID_AA64PFR2_EL1";
    arm64_sysreg_add_field(ID_AA64PFR2_EL1, "FPMR", 32, 35);
    arm64_sysreg_add_field(ID_AA64PFR2_EL1, "UINJ", 16, 19);
    arm64_sysreg_add_field(ID_AA64PFR2_EL1, "MTEFAR", 8, 11);
    arm64_sysreg_add_field(ID_AA64PFR2_EL1, "MTESTOREONLY", 4, 7);
    arm64_sysreg_add_field(ID_AA64PFR2_EL1, "MTEPERM", 0, 3);

    /* ID_AA64ZFR0_EL1 */
    ARM64SysReg *ID_AA64ZFR0_EL1 = arm64_sysreg_get(ID_AA64ZFR0_EL1_IDX);
    ID_AA64ZFR0_EL1->name = "ID_AA64ZFR0_EL1";
    arm64_sysreg_add_field(ID_AA64ZFR0_EL1, "F64MM", 56, 59);
    arm64_sysreg_add_field(ID_AA64ZFR0_EL1, "F32MM", 52, 55);
    arm64_sysreg_add_field(ID_AA64ZFR0_EL1, "F16MM", 48, 51);
    arm64_sysreg_add_field(ID_AA64ZFR0_EL1, "I8MM", 44, 47);
    arm64_sysreg_add_field(ID_AA64ZFR0_EL1, "SM4", 40, 43);
    arm64_sysreg_add_field(ID_AA64ZFR0_EL1, "SHA3", 32, 35);
    arm64_sysreg_add_field(ID_AA64ZFR0_EL1, "B16B16", 24, 27);
    arm64_sysreg_add_field(ID_AA64ZFR0_EL1, "BF16", 20, 23);
    arm64_sysreg_add_field(ID_AA64ZFR0_EL1, "BitPerm", 16, 19);
    arm64_sysreg_add_field(ID_AA64ZFR0_EL1, "EltPerm", 12, 15);
    arm64_sysreg_add_field(ID_AA64ZFR0_EL1, "AES", 4, 7);
    arm64_sysreg_add_field(ID_AA64ZFR0_EL1, "SVEver", 0, 3);

    /* ID_AA64SMFR0_EL1 */
    ARM64SysReg *ID_AA64SMFR0_EL1 = arm64_sysreg_get(ID_AA64SMFR0_EL1_IDX);
    ID_AA64SMFR0_EL1->name = "ID_AA64SMFR0_EL1";
    arm64_sysreg_add_field(ID_AA64SMFR0_EL1, "FA64", 63, 63);
    arm64_sysreg_add_field(ID_AA64SMFR0_EL1, "LUTv2", 60, 60);
    arm64_sysreg_add_field(ID_AA64SMFR0_EL1, "SMEver", 56, 59);
    arm64_sysreg_add_field(ID_AA64SMFR0_EL1, "I16I64", 52, 55);
    arm64_sysreg_add_field(ID_AA64SMFR0_EL1, "F64F64", 48, 48);
    arm64_sysreg_add_field(ID_AA64SMFR0_EL1, "I16I32", 44, 47);
    arm64_sysreg_add_field(ID_AA64SMFR0_EL1, "B16B16", 43, 43);
    arm64_sysreg_add_field(ID_AA64SMFR0_EL1, "F16F16", 42, 42);
    arm64_sysreg_add_field(ID_AA64SMFR0_EL1, "F8F16", 41, 41);
    arm64_sysreg_add_field(ID_AA64SMFR0_EL1, "F8F32", 40, 40);
    arm64_sysreg_add_field(ID_AA64SMFR0_EL1, "I8I32", 36, 39);
    arm64_sysreg_add_field(ID_AA64SMFR0_EL1, "F16F32", 35, 35);
    arm64_sysreg_add_field(ID_AA64SMFR0_EL1, "B16F32", 34, 34);
    arm64_sysreg_add_field(ID_AA64SMFR0_EL1, "BI32I32", 33, 33);
    arm64_sysreg_add_field(ID_AA64SMFR0_EL1, "F32F32", 32, 32);
    arm64_sysreg_add_field(ID_AA64SMFR0_EL1, "SF8FMA", 30, 30);
    arm64_sysreg_add_field(ID_AA64SMFR0_EL1, "SF8DP4", 29, 29);
    arm64_sysreg_add_field(ID_AA64SMFR0_EL1, "SF8DP2", 28, 28);
    arm64_sysreg_add_field(ID_AA64SMFR0_EL1, "SBitPerm", 25, 25);
    arm64_sysreg_add_field(ID_AA64SMFR0_EL1, "AES", 24, 24);
    arm64_sysreg_add_field(ID_AA64SMFR0_EL1, "SFEXPA", 23, 23);
    arm64_sysreg_add_field(ID_AA64SMFR0_EL1, "STMOP", 16, 16);
    arm64_sysreg_add_field(ID_AA64SMFR0_EL1, "SMOP4", 0, 0);

    /* ID_AA64FPFR0_EL1 */
    ARM64SysReg *ID_AA64FPFR0_EL1 = arm64_sysreg_get(ID_AA64FPFR0_EL1_IDX);
    ID_AA64FPFR0_EL1->name = "ID_AA64FPFR0_EL1";
    arm64_sysreg_add_field(ID_AA64FPFR0_EL1, "F8CVT", 31, 31);
    arm64_sysreg_add_field(ID_AA64FPFR0_EL1, "F8FMA", 30, 30);
    arm64_sysreg_add_field(ID_AA64FPFR0_EL1, "F8DP4", 29, 29);
    arm64_sysreg_add_field(ID_AA64FPFR0_EL1, "F8DP2", 28, 28);
    arm64_sysreg_add_field(ID_AA64FPFR0_EL1, "F8MM8", 27, 27);
    arm64_sysreg_add_field(ID_AA64FPFR0_EL1, "F8MM4", 26, 26);
    arm64_sysreg_add_field(ID_AA64FPFR0_EL1, "F8E4M3", 1, 1);
    arm64_sysreg_add_field(ID_AA64FPFR0_EL1, "F8E5M2", 0, 0);

    /* ID_AA64DFR0_EL1 */
    ARM64SysReg *ID_AA64DFR0_EL1 = arm64_sysreg_get(ID_AA64DFR0_EL1_IDX);
    ID_AA64DFR0_EL1->name = "ID_AA64DFR0_EL1";
    arm64_sysreg_add_field(ID_AA64DFR0_EL1, "HPMN0", 60, 63);
    arm64_sysreg_add_field(ID_AA64DFR0_EL1, "ExtTrcBuff", 56, 59);
    arm64_sysreg_add_field(ID_AA64DFR0_EL1, "BRBE", 52, 55);
    arm64_sysreg_add_field(ID_AA64DFR0_EL1, "MTPMU", 48, 51);
    arm64_sysreg_add_field(ID_AA64DFR0_EL1, "MTPMU", 48, 51);
    arm64_sysreg_add_field(ID_AA64DFR0_EL1, "TraceBuffer", 44, 47);
    arm64_sysreg_add_field(ID_AA64DFR0_EL1, "TraceFilt", 40, 43);
    arm64_sysreg_add_field(ID_AA64DFR0_EL1, "DoubleLock", 36, 39);
    arm64_sysreg_add_field(ID_AA64DFR0_EL1, "PMSVer", 32, 35);
    arm64_sysreg_add_field(ID_AA64DFR0_EL1, "CTX_CMPs", 28, 31);
    arm64_sysreg_add_field(ID_AA64DFR0_EL1, "SEBEP", 24, 27);
    arm64_sysreg_add_field(ID_AA64DFR0_EL1, "WRPs", 20, 23);
    arm64_sysreg_add_field(ID_AA64DFR0_EL1, "PMSS", 16, 19);
    arm64_sysreg_add_field(ID_AA64DFR0_EL1, "BRPs", 12, 15);
    arm64_sysreg_add_field(ID_AA64DFR0_EL1, "PMUVer", 8, 11);
    arm64_sysreg_add_field(ID_AA64DFR0_EL1, "TraceVer", 4, 7);
    arm64_sysreg_add_field(ID_AA64DFR0_EL1, "DebugVer", 0, 3);

    /* ID_AA64DFR1_EL1 */
    ARM64SysReg *ID_AA64DFR1_EL1 = arm64_sysreg_get(ID_AA64DFR1_EL1_IDX);
    ID_AA64DFR1_EL1->name = "ID_AA64DFR1_EL1";
    arm64_sysreg_add_field(ID_AA64DFR1_EL1, "ABL_CMPs", 56, 63);
    arm64_sysreg_add_field(ID_AA64DFR1_EL1, "DPFZS", 52, 55);
    arm64_sysreg_add_field(ID_AA64DFR1_EL1, "EBEP", 48, 51);
    arm64_sysreg_add_field(ID_AA64DFR1_EL1, "ITE", 44, 47);
    arm64_sysreg_add_field(ID_AA64DFR1_EL1, "ABLE", 40, 43);
    arm64_sysreg_add_field(ID_AA64DFR1_EL1, "PMICNTR", 36, 39);
    arm64_sysreg_add_field(ID_AA64DFR1_EL1, "SPMU", 32, 35);
    arm64_sysreg_add_field(ID_AA64DFR1_EL1, "CTX_CMPs", 24, 31);
    arm64_sysreg_add_field(ID_AA64DFR1_EL1, "WRPs", 16, 23);
    arm64_sysreg_add_field(ID_AA64DFR1_EL1, "BRPs", 8, 15);
    arm64_sysreg_add_field(ID_AA64DFR1_EL1, "SYSPMUID", 0, 7);

    /* ID_AA64DFR2_EL1 */
    ARM64SysReg *ID_AA64DFR2_EL1 = arm64_sysreg_get(ID_AA64DFR2_EL1_IDX);
    ID_AA64DFR2_EL1->name = "ID_AA64DFR2_EL1";
    arm64_sysreg_add_field(ID_AA64DFR2_EL1, "TRBE_EXC", 24, 27);
    arm64_sysreg_add_field(ID_AA64DFR2_EL1, "SPE_nVM", 20, 23);
    arm64_sysreg_add_field(ID_AA64DFR2_EL1, "SPE_EXC", 16, 19);
    arm64_sysreg_add_field(ID_AA64DFR2_EL1, "BWE", 4, 7);
    arm64_sysreg_add_field(ID_AA64DFR2_EL1, "STEP", 0, 3);

    /* ID_AA64AFR0_EL1 */
    ARM64SysReg *ID_AA64AFR0_EL1 = arm64_sysreg_get(ID_AA64AFR0_EL1_IDX);
    ID_AA64AFR0_EL1->name = "ID_AA64AFR0_EL1";
    arm64_sysreg_add_field(ID_AA64AFR0_EL1, "IMPDEF7", 28, 31);
    arm64_sysreg_add_field(ID_AA64AFR0_EL1, "IMPDEF6", 24, 27);
    arm64_sysreg_add_field(ID_AA64AFR0_EL1, "IMPDEF5", 20, 23);
    arm64_sysreg_add_field(ID_AA64AFR0_EL1, "IMPDEF4", 16, 19);
    arm64_sysreg_add_field(ID_AA64AFR0_EL1, "IMPDEF3", 12, 15);
    arm64_sysreg_add_field(ID_AA64AFR0_EL1, "IMPDEF2", 8, 11);
    arm64_sysreg_add_field(ID_AA64AFR0_EL1, "IMPDEF1", 4, 7);
    arm64_sysreg_add_field(ID_AA64AFR0_EL1, "IMPDEF0", 0, 3);

    /* ID_AA64AFR1_EL1 */
    ARM64SysReg *ID_AA64AFR1_EL1 = arm64_sysreg_get(ID_AA64AFR1_EL1_IDX);
    ID_AA64AFR1_EL1->name = "ID_AA64AFR1_EL1";

    /* ID_AA64ISAR0_EL1 */
    ARM64SysReg *ID_AA64ISAR0_EL1 = arm64_sysreg_get(ID_AA64ISAR0_EL1_IDX);
    ID_AA64ISAR0_EL1->name = "ID_AA64ISAR0_EL1";
    arm64_sysreg_add_field(ID_AA64ISAR0_EL1, "RNDR", 60, 63);
    arm64_sysreg_add_field(ID_AA64ISAR0_EL1, "TLB", 56, 59);
    arm64_sysreg_add_field(ID_AA64ISAR0_EL1, "TS", 52, 55);
    arm64_sysreg_add_field(ID_AA64ISAR0_EL1, "FHM", 48, 51);
    arm64_sysreg_add_field(ID_AA64ISAR0_EL1, "DP", 44, 47);
    arm64_sysreg_add_field(ID_AA64ISAR0_EL1, "SM4", 40, 43);
    arm64_sysreg_add_field(ID_AA64ISAR0_EL1, "SM3", 36, 39);
    arm64_sysreg_add_field(ID_AA64ISAR0_EL1, "SHA3", 32, 35);
    arm64_sysreg_add_field(ID_AA64ISAR0_EL1, "RDM", 28, 31);
    arm64_sysreg_add_field(ID_AA64ISAR0_EL1, "TME", 24, 27);
    arm64_sysreg_add_field(ID_AA64ISAR0_EL1, "ATOMIC", 20, 23);
    arm64_sysreg_add_field(ID_AA64ISAR0_EL1, "CRC32", 16, 19);
    arm64_sysreg_add_field(ID_AA64ISAR0_EL1, "SHA2", 12, 15);
    arm64_sysreg_add_field(ID_AA64ISAR0_EL1, "SHA1", 8, 11);
    arm64_sysreg_add_field(ID_AA64ISAR0_EL1, "AES", 4, 7);

    /* ID_AA64ISAR1_EL1 */
    ARM64SysReg *ID_AA64ISAR1_EL1 = arm64_sysreg_get(ID_AA64ISAR1_EL1_IDX);
    ID_AA64ISAR1_EL1->name = "ID_AA64ISAR1_EL1";
    arm64_sysreg_add_field(ID_AA64ISAR1_EL1, "LS64", 60, 63);
    arm64_sysreg_add_field(ID_AA64ISAR1_EL1, "XS", 56, 59);
    arm64_sysreg_add_field(ID_AA64ISAR1_EL1, "I8MM", 52, 55);
    arm64_sysreg_add_field(ID_AA64ISAR1_EL1, "DGH", 48, 51);
    arm64_sysreg_add_field(ID_AA64ISAR1_EL1, "BF16", 44, 47);
    arm64_sysreg_add_field(ID_AA64ISAR1_EL1, "SPECRES", 40, 43);
    arm64_sysreg_add_field(ID_AA64ISAR1_EL1, "SB", 36, 39);
    arm64_sysreg_add_field(ID_AA64ISAR1_EL1, "FRINTTS", 32, 35);
    arm64_sysreg_add_field(ID_AA64ISAR1_EL1, "GPI", 28, 31);
    arm64_sysreg_add_field(ID_AA64ISAR1_EL1, "GPA", 24, 27);
    arm64_sysreg_add_field(ID_AA64ISAR1_EL1, "LRCPC", 20, 23);
    arm64_sysreg_add_field(ID_AA64ISAR1_EL1, "FCMA", 16, 19);
    arm64_sysreg_add_field(ID_AA64ISAR1_EL1, "JSCVT", 12, 15);
    arm64_sysreg_add_field(ID_AA64ISAR1_EL1, "API", 8, 11);
    arm64_sysreg_add_field(ID_AA64ISAR1_EL1, "APA", 4, 7);
    arm64_sysreg_add_field(ID_AA64ISAR1_EL1, "DPB", 0, 3);

    /* ID_AA64ISAR2_EL1 */
    ARM64SysReg *ID_AA64ISAR2_EL1 = arm64_sysreg_get(ID_AA64ISAR2_EL1_IDX);
    ID_AA64ISAR2_EL1->name = "ID_AA64ISAR2_EL1";
    arm64_sysreg_add_field(ID_AA64ISAR2_EL1, "ATS1A", 60, 63);
    arm64_sysreg_add_field(ID_AA64ISAR2_EL1, "LUT", 56, 59);
    arm64_sysreg_add_field(ID_AA64ISAR2_EL1, "CSSC", 52, 55);
    arm64_sysreg_add_field(ID_AA64ISAR2_EL1, "RPRFM", 48, 51);
    arm64_sysreg_add_field(ID_AA64ISAR2_EL1, "PCDPHINT", 44, 47);
    arm64_sysreg_add_field(ID_AA64ISAR2_EL1, "PRFMSLC", 40, 43);
    arm64_sysreg_add_field(ID_AA64ISAR2_EL1, "SYSINSTR_128", 36, 39);
    arm64_sysreg_add_field(ID_AA64ISAR2_EL1, "SYSREG_128", 32, 35);
    arm64_sysreg_add_field(ID_AA64ISAR2_EL1, "CLRBHB", 28, 31);
    arm64_sysreg_add_field(ID_AA64ISAR2_EL1, "PAC_frac", 24, 27);
    arm64_sysreg_add_field(ID_AA64ISAR2_EL1, "BC", 20, 23);
    arm64_sysreg_add_field(ID_AA64ISAR2_EL1, "MOPS", 16, 19);
    arm64_sysreg_add_field(ID_AA64ISAR2_EL1, "APA3", 12, 15);
    arm64_sysreg_add_field(ID_AA64ISAR2_EL1, "GPA3", 8, 11);
    arm64_sysreg_add_field(ID_AA64ISAR2_EL1, "RPRES", 4, 7);
    arm64_sysreg_add_field(ID_AA64ISAR2_EL1, "WFxT", 0, 3);

    /* ID_AA64ISAR3_EL1 */
    ARM64SysReg *ID_AA64ISAR3_EL1 = arm64_sysreg_get(ID_AA64ISAR3_EL1_IDX);
    ID_AA64ISAR3_EL1->name = "ID_AA64ISAR3_EL1";
    arm64_sysreg_add_field(ID_AA64ISAR3_EL1, "FPRCVT", 28, 31);
    arm64_sysreg_add_field(ID_AA64ISAR3_EL1, "LSUI", 24, 27);
    arm64_sysreg_add_field(ID_AA64ISAR3_EL1, "OCCMO", 20, 23);
    arm64_sysreg_add_field(ID_AA64ISAR3_EL1, "LSFE", 16, 19);
    arm64_sysreg_add_field(ID_AA64ISAR3_EL1, "PACM", 12, 15);
    arm64_sysreg_add_field(ID_AA64ISAR3_EL1, "TLBIW", 8, 11);
    arm64_sysreg_add_field(ID_AA64ISAR3_EL1, "FAMINMAX", 4, 7);
    arm64_sysreg_add_field(ID_AA64ISAR3_EL1, "CPA", 0, 3);

    /* ID_AA64MMFR0_EL1 */
    ARM64SysReg *ID_AA64MMFR0_EL1 = arm64_sysreg_get(ID_AA64MMFR0_EL1_IDX);
    ID_AA64MMFR0_EL1->name = "ID_AA64MMFR0_EL1";
    arm64_sysreg_add_field(ID_AA64MMFR0_EL1, "ECV", 60, 63);
    arm64_sysreg_add_field(ID_AA64MMFR0_EL1, "FGT", 56, 59);
    arm64_sysreg_add_field(ID_AA64MMFR0_EL1, "EXS", 44, 47);
    arm64_sysreg_add_field(ID_AA64MMFR0_EL1, "TGRAN4_2", 40, 43);
    arm64_sysreg_add_field(ID_AA64MMFR0_EL1, "TGRAN64_2", 36, 39);
    arm64_sysreg_add_field(ID_AA64MMFR0_EL1, "TGRAN16_2", 32, 35);
    arm64_sysreg_add_field(ID_AA64MMFR0_EL1, "TGRAN4", 28, 31);
    arm64_sysreg_add_field(ID_AA64MMFR0_EL1, "TGRAN4", 28, 31);
    arm64_sysreg_add_field(ID_AA64MMFR0_EL1, "TGRAN64", 24, 27);
    arm64_sysreg_add_field(ID_AA64MMFR0_EL1, "TGRAN64", 24, 27);
    arm64_sysreg_add_field(ID_AA64MMFR0_EL1, "TGRAN16", 20, 23);
    arm64_sysreg_add_field(ID_AA64MMFR0_EL1, "BIGENDEL0", 16, 19);
    arm64_sysreg_add_field(ID_AA64MMFR0_EL1, "SNSMEM", 12, 15);
    arm64_sysreg_add_field(ID_AA64MMFR0_EL1, "BIGEND", 8, 11);
    arm64_sysreg_add_field(ID_AA64MMFR0_EL1, "ASIDBITS", 4, 7);
    arm64_sysreg_add_field(ID_AA64MMFR0_EL1, "PARANGE", 0, 3);

    /* ID_AA64MMFR1_EL1 */
    ARM64SysReg *ID_AA64MMFR1_EL1 = arm64_sysreg_get(ID_AA64MMFR1_EL1_IDX);
    ID_AA64MMFR1_EL1->name = "ID_AA64MMFR1_EL1";
    arm64_sysreg_add_field(ID_AA64MMFR1_EL1, "ECBHB", 60, 63);
    arm64_sysreg_add_field(ID_AA64MMFR1_EL1, "CMOW", 56, 59);
    arm64_sysreg_add_field(ID_AA64MMFR1_EL1, "TIDCP1", 52, 55);
    arm64_sysreg_add_field(ID_AA64MMFR1_EL1, "nTLBPA", 48, 51);
    arm64_sysreg_add_field(ID_AA64MMFR1_EL1, "AFP", 44, 47);
    arm64_sysreg_add_field(ID_AA64MMFR1_EL1, "HCX", 40, 43);
    arm64_sysreg_add_field(ID_AA64MMFR1_EL1, "ETS", 36, 39);
    arm64_sysreg_add_field(ID_AA64MMFR1_EL1, "TWED", 32, 35);
    arm64_sysreg_add_field(ID_AA64MMFR1_EL1, "XNX", 28, 31);
    arm64_sysreg_add_field(ID_AA64MMFR1_EL1, "SpecSEI", 24, 27);
    arm64_sysreg_add_field(ID_AA64MMFR1_EL1, "PAN", 20, 23);
    arm64_sysreg_add_field(ID_AA64MMFR1_EL1, "LO", 16, 19);
    arm64_sysreg_add_field(ID_AA64MMFR1_EL1, "HPDS", 12, 15);
    arm64_sysreg_add_field(ID_AA64MMFR1_EL1, "VH", 8, 11);
    arm64_sysreg_add_field(ID_AA64MMFR1_EL1, "VMIDBits", 4, 7);
    arm64_sysreg_add_field(ID_AA64MMFR1_EL1, "HAFDBS", 0, 3);

    /* ID_AA64MMFR2_EL1 */
    ARM64SysReg *ID_AA64MMFR2_EL1 = arm64_sysreg_get(ID_AA64MMFR2_EL1_IDX);
    ID_AA64MMFR2_EL1->name = "ID_AA64MMFR2_EL1";
    arm64_sysreg_add_field(ID_AA64MMFR2_EL1, "E0PD", 60, 63);
    arm64_sysreg_add_field(ID_AA64MMFR2_EL1, "EVT", 56, 59);
    arm64_sysreg_add_field(ID_AA64MMFR2_EL1, "BBM", 52, 55);
    arm64_sysreg_add_field(ID_AA64MMFR2_EL1, "TTL", 48, 51);
    arm64_sysreg_add_field(ID_AA64MMFR2_EL1, "FWB", 40, 43);
    arm64_sysreg_add_field(ID_AA64MMFR2_EL1, "IDS", 36, 39);
    arm64_sysreg_add_field(ID_AA64MMFR2_EL1, "AT", 32, 35);
    arm64_sysreg_add_field(ID_AA64MMFR2_EL1, "ST", 28, 31);
    arm64_sysreg_add_field(ID_AA64MMFR2_EL1, "NV", 24, 27);
    arm64_sysreg_add_field(ID_AA64MMFR2_EL1, "CCIDX", 20, 23);
    arm64_sysreg_add_field(ID_AA64MMFR2_EL1, "VARange", 16, 19);
    arm64_sysreg_add_field(ID_AA64MMFR2_EL1, "IESB", 12, 15);
    arm64_sysreg_add_field(ID_AA64MMFR2_EL1, "LSM", 8, 11);
    arm64_sysreg_add_field(ID_AA64MMFR2_EL1, "UAO", 4, 7);
    arm64_sysreg_add_field(ID_AA64MMFR2_EL1, "CnP", 0, 3);

    /* ID_AA64MMFR3_EL1 */
    ARM64SysReg *ID_AA64MMFR3_EL1 = arm64_sysreg_get(ID_AA64MMFR3_EL1_IDX);
    ID_AA64MMFR3_EL1->name = "ID_AA64MMFR3_EL1";
    arm64_sysreg_add_field(ID_AA64MMFR3_EL1, "Spec_FPACC", 60, 63);
    arm64_sysreg_add_field(ID_AA64MMFR3_EL1, "ADERR", 56, 59);
    arm64_sysreg_add_field(ID_AA64MMFR3_EL1, "SDERR", 52, 55);
    arm64_sysreg_add_field(ID_AA64MMFR3_EL1, "ANERR", 44, 47);
    arm64_sysreg_add_field(ID_AA64MMFR3_EL1, "SNERR", 40, 43);
    arm64_sysreg_add_field(ID_AA64MMFR3_EL1, "D128_2", 36, 39);
    arm64_sysreg_add_field(ID_AA64MMFR3_EL1, "D128", 32, 35);
    arm64_sysreg_add_field(ID_AA64MMFR3_EL1, "MEC", 28, 31);
    arm64_sysreg_add_field(ID_AA64MMFR3_EL1, "AIE", 24, 27);
    arm64_sysreg_add_field(ID_AA64MMFR3_EL1, "S2POE", 20, 23);
    arm64_sysreg_add_field(ID_AA64MMFR3_EL1, "S1POE", 16, 19);
    arm64_sysreg_add_field(ID_AA64MMFR3_EL1, "S2PIE", 12, 15);
    arm64_sysreg_add_field(ID_AA64MMFR3_EL1, "S1PIE", 8, 11);
    arm64_sysreg_add_field(ID_AA64MMFR3_EL1, "SCTLRX", 4, 7);
    arm64_sysreg_add_field(ID_AA64MMFR3_EL1, "TCRX", 0, 3);

    /* ID_AA64MMFR4_EL1 */
    ARM64SysReg *ID_AA64MMFR4_EL1 = arm64_sysreg_get(ID_AA64MMFR4_EL1_IDX);
    ID_AA64MMFR4_EL1->name = "ID_AA64MMFR4_EL1";
    arm64_sysreg_add_field(ID_AA64MMFR4_EL1, "E3DSE", 36, 39);
    arm64_sysreg_add_field(ID_AA64MMFR4_EL1, "E2H0", 24, 27);
    arm64_sysreg_add_field(ID_AA64MMFR4_EL1, "E2H0", 24, 27);
    arm64_sysreg_add_field(ID_AA64MMFR4_EL1, "NV_frac", 20, 23);
    arm64_sysreg_add_field(ID_AA64MMFR4_EL1, "FGWTE3", 16, 19);
    arm64_sysreg_add_field(ID_AA64MMFR4_EL1, "HACDBS", 12, 15);
    arm64_sysreg_add_field(ID_AA64MMFR4_EL1, "ASID2", 8, 11);
    arm64_sysreg_add_field(ID_AA64MMFR4_EL1, "EIESB", 4, 7);
    arm64_sysreg_add_field(ID_AA64MMFR4_EL1, "EIESB", 4, 7);

/* For ZCR_EL1 fields see ZCR_ELx */

/* For SMCR_EL1 fields see SMCR_ELx */

/* For GCSCR_EL1 fields see GCSCR_ELx */

/* For GCSPR_EL1 fields see GCSPR_ELx */

/* For CONTEXTIDR_EL1 fields see CONTEXTIDR_ELx */

    /* CCSIDR_EL1 */
    ARM64SysReg *CCSIDR_EL1 = arm64_sysreg_get(CCSIDR_EL1_IDX);
    CCSIDR_EL1->name = "CCSIDR_EL1";
    arm64_sysreg_add_field(CCSIDR_EL1, "NumSets", 13, 27);
    arm64_sysreg_add_field(CCSIDR_EL1, "Associativity", 3, 12);
    arm64_sysreg_add_field(CCSIDR_EL1, "LineSize", 0, 2);

    /* CLIDR_EL1 */
    ARM64SysReg *CLIDR_EL1 = arm64_sysreg_get(CLIDR_EL1_IDX);
    CLIDR_EL1->name = "CLIDR_EL1";
    arm64_sysreg_add_field(CLIDR_EL1, "Ttypen", 33, 46);
    arm64_sysreg_add_field(CLIDR_EL1, "ICB", 30, 32);
    arm64_sysreg_add_field(CLIDR_EL1, "LoUU", 27, 29);
    arm64_sysreg_add_field(CLIDR_EL1, "LoC", 24, 26);
    arm64_sysreg_add_field(CLIDR_EL1, "LoUIS", 21, 23);
    arm64_sysreg_add_field(CLIDR_EL1, "Ctype7", 18, 20);
    arm64_sysreg_add_field(CLIDR_EL1, "Ctype6", 15, 17);
    arm64_sysreg_add_field(CLIDR_EL1, "Ctype5", 12, 14);
    arm64_sysreg_add_field(CLIDR_EL1, "Ctype4", 9, 11);
    arm64_sysreg_add_field(CLIDR_EL1, "Ctype3", 6, 8);
    arm64_sysreg_add_field(CLIDR_EL1, "Ctype2", 3, 5);
    arm64_sysreg_add_field(CLIDR_EL1, "Ctype1", 0, 2);

    /* CCSIDR2_EL1 */
    ARM64SysReg *CCSIDR2_EL1 = arm64_sysreg_get(CCSIDR2_EL1_IDX);
    CCSIDR2_EL1->name = "CCSIDR2_EL1";
    arm64_sysreg_add_field(CCSIDR2_EL1, "NumSets", 0, 23);

    /* GMID_EL1 */
    ARM64SysReg *GMID_EL1 = arm64_sysreg_get(GMID_EL1_IDX);
    GMID_EL1->name = "GMID_EL1";
    arm64_sysreg_add_field(GMID_EL1, "BS", 0, 3);

    /* SMIDR_EL1 */
    ARM64SysReg *SMIDR_EL1 = arm64_sysreg_get(SMIDR_EL1_IDX);
    SMIDR_EL1->name = "SMIDR_EL1";
    arm64_sysreg_add_field(SMIDR_EL1, "IMPLEMENTER", 24, 31);
    arm64_sysreg_add_field(SMIDR_EL1, "REVISION", 16, 23);
    arm64_sysreg_add_field(SMIDR_EL1, "SMPS", 15, 15);
    arm64_sysreg_add_field(SMIDR_EL1, "AFFINITY", 0, 11);

    /* CSSELR_EL1 */
    ARM64SysReg *CSSELR_EL1 = arm64_sysreg_get(CSSELR_EL1_IDX);
    CSSELR_EL1->name = "CSSELR_EL1";
    arm64_sysreg_add_field(CSSELR_EL1, "TnD", 4, 4);
    arm64_sysreg_add_field(CSSELR_EL1, "Level", 1, 3);
    arm64_sysreg_add_field(CSSELR_EL1, "InD", 0, 0);

    /* CTR_EL0 */
    ARM64SysReg *CTR_EL0 = arm64_sysreg_get(CTR_EL0_IDX);
    CTR_EL0->name = "CTR_EL0";
    arm64_sysreg_add_field(CTR_EL0, "TminLine", 32, 37);
    arm64_sysreg_add_field(CTR_EL0, "DIC", 29, 29);
    arm64_sysreg_add_field(CTR_EL0, "IDC", 28, 28);
    arm64_sysreg_add_field(CTR_EL0, "CWG", 24, 27);
    arm64_sysreg_add_field(CTR_EL0, "ERG", 20, 23);
    arm64_sysreg_add_field(CTR_EL0, "DminLine", 16, 19);
    arm64_sysreg_add_field(CTR_EL0, "L1Ip", 14, 15);
    arm64_sysreg_add_field(CTR_EL0, "IminLine", 0, 3);

    /* DCZID_EL0 */
    ARM64SysReg *DCZID_EL0 = arm64_sysreg_get(DCZID_EL0_IDX);
    DCZID_EL0->name = "DCZID_EL0";
    arm64_sysreg_add_field(DCZID_EL0, "DZP", 4, 4);
    arm64_sysreg_add_field(DCZID_EL0, "BS", 0, 3);

/* For GCSPR_EL0 fields see GCSPR_ELx */

/* For HFGRTR_EL2 fields see HFGxTR_EL2 */

/* For HFGWTR_EL2 fields see HFGxTR_EL2 */

/* For ZCR_EL2 fields see ZCR_ELx */

/* For SMCR_EL2 fields see SMCR_ELx */

/* For GCSCR_EL2 fields see GCSCR_ELx */

/* For GCSPR_EL2 fields see GCSPR_ELx */

/* For CONTEXTIDR_EL2 fields see CONTEXTIDR_ELx */

/* For CPACR_EL12 fields see CPACR_EL1 */

/* For ZCR_EL12 fields see ZCR_EL1 */

/* For TRFCR_EL12 fields see TRFCR_EL1 */

/* For SMCR_EL12 fields see SMCR_EL1 */

/* For GCSCR_EL12 fields see GCSCR_EL1 */

/* For GCSPR_EL12 fields see GCSPR_EL1 */

/* For MPAM1_EL12 fields see MPAM1_ELx */

/* For CONTEXTIDR_EL12 fields see CONTEXTIDR_EL1 */

/* For TTBR0_EL1 fields see TTBRx_EL1 */

/* For TTBR1_EL1 fields see TTBRx_EL1 */

/* For TCR2_EL12 fields see TCR2_EL1 */

/* For MAIR2_EL1 fields see MAIR2_ELx */

/* For MAIR2_EL2 fields see MAIR2_ELx */

/* For PIRE0_EL1 fields see PIRx_ELx */

/* For PIRE0_EL12 fields see PIRE0_EL1 */

/* For PIRE0_EL2 fields see PIRx_ELx */

/* For PIR_EL1 fields see PIRx_ELx */

/* For PIR_EL12 fields see PIR_EL1 */

/* For PIR_EL2 fields see PIRx_ELx */

/* For POR_EL0 fields see PIRx_ELx */

/* For POR_EL1 fields see PIRx_ELx */

/* For POR_EL2 fields see PIRx_ELx */

/* For POR_EL12 fields see POR_EL1 */

/* For S2POR_EL1 fields see PIRx_ELx */

/* For S2PIR_EL2 fields see PIRx_ELx */

}
