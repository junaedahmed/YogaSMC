//  SPDX-License-Identifier: GPL-2.0-only
//
//  IdeaVPC.hpp
//  YogaSMC
//
//  Created by Zhen on 2020/7/11.
//  Copyright © 2020 Zhen. All rights reserved.
//

#ifndef IdeaVPC_hpp
#define IdeaVPC_hpp

#include "YogaVPC.hpp"
#ifndef ALTER
#include "IdeaSMC.hpp"
#endif

// from linux/drivers/platform/x86/ideapad-laptop.c

#define BM_RAPIDCHARGE_BIT   (2)
#define BM_BATTERY0BAD_BIT   (3)
#define BM_BATTERY1BAD_BIT   (4)
#define BM_CONSERVATION_BIT  (5)
#define HA_BACKLIGHT_CAP_BIT (4)
#define HA_BACKLIGHT_BIT     (5)
#define HA_FNLOCK_CAP_BIT    (9)
#define HA_FNLOCK_BIT        (10)
#define HA_PRIMEKEY_BIT      (11)

#define CFG_GRAPHICS_BIT     (8)
#define CFG_BT_BIT           (16)
#define CFG_3G_BIT           (17)
#define CFG_WIFI_BIT         (18)
#define CFG_CAMERA_BIT       (19)

enum {
    BMCMD_CONSERVATION_ON = 3,
    BMCMD_CONSERVATION_OFF = 5,
    BMCMD_RAPIDCHARGE_ON = 7,
    BMCMD_RAPIDCHARGE_OFF = 8,
    HACMD_BACKLIGHT_ON = 0x8,
    HACMD_BACKLIGHT_OFF = 0x9,
    HACMD_FNLOCK_ON = 0xe,
    HACMD_FNLOCK_OFF = 0xf,
};

enum {
    VPCCMD_R_VPC1 = 0x10,
    VPCCMD_R_BL_MAX, /* 0x11 */
    VPCCMD_R_BL, /* 0x12 */
    VPCCMD_W_BL, /* 0x13 */
    VPCCMD_R_WIFI, /* 0x14 */
    VPCCMD_W_WIFI, /* 0x15 */
    VPCCMD_R_BT, /* 0x16 */
    VPCCMD_W_BT, /* 0x17 */
    VPCCMD_R_BL_POWER, /* 0x18 */
    VPCCMD_R_NOVO, /* 0x19 */
    VPCCMD_R_VPC2, /* 0x1A */
    VPCCMD_R_TOUCHPAD, /* 0x1B */
    VPCCMD_W_TOUCHPAD, /* 0x1C */
    VPCCMD_R_CAMERA, /* 0x1D */
    VPCCMD_W_CAMERA, /* 0x1E */
    VPCCMD_R_3G, /* 0x1F */
    VPCCMD_W_3G, /* 0x20 */
    VPCCMD_R_ODD, /* 0x21 */
    VPCCMD_W_FAN, /* 0x22 */
    VPCCMD_R_RF, /* 0x23 */
    VPCCMD_W_RF, /* 0x24 */
    VPCCMD_R_FAN = 0x2B,
    VPCCMD_R_SPECIAL_BUTTONS = 0x31,
    VPCCMD_W_BL_POWER = 0x33,
} VPCCMD;

#define IDEAPAD_EC_TIMEOUT (200) /* in ms */

class IdeaVPC : public YogaVPC
{
    typedef YogaVPC super;
    OSDeclareDefaultStructors(IdeaVPC)

private:
    /**
     *  Related ACPI methods
     */
    static constexpr const char *getVPCConfig         = "_CFG";
    static constexpr const char *getBatteryID         = "GBID";
    static constexpr const char *getBatteryInfo       = "GSBI";
    static constexpr const char *getBatteryMode       = "GBMD";
    static constexpr const char *setBatteryMode       = "SBMC";
    static constexpr const char *getKeyboardMode      = "HALS";
    static constexpr const char *setKeyboardMode      = "SALS";
    static constexpr const char *readVPCStatus        = "VPCR";
    static constexpr const char *writeVPCStatus       = "VPCW";

    /**
     * VPC0 config
     */
    UInt32 config;
    UInt8 cap_graphics;
    bool cap_bt;
    bool cap_3g;
    bool cap_wifi;
    bool cap_camera;

    bool initVPC() APPLE_KEXT_OVERRIDE;
    void setPropertiesGated(OSObject* props) APPLE_KEXT_OVERRIDE;
    void updateAll() APPLE_KEXT_OVERRIDE;
    void updateVPC() APPLE_KEXT_OVERRIDE;

    /**
     *  Battery conservation mode status
     */
    bool conservationMode;
    
    /**
     *  Prohibit early conservation mode manipulation
     */
    bool conservationModeLock {true};

    /**
     *  Prohibit diirect EC manipulate by default
     */
    bool ECLock {true};

    /**
     *  Fn lock mode capability, will be update on init
     */
    bool FnlockCap {false};

    /**
     *  Fn lock mode status
     */
    bool FnlockMode {false};

    /**
     *  Rapid charge mode status
     */
    bool rapidChargeMode {false};

    /**
     *  Initialize VPC EC status
     *
     *  @return true if success
     */
    bool initEC();

#ifndef ALTER
    /**
     *  Initialize SMC
     *
     *  @return true if success
     */
    inline void initSMC() APPLE_KEXT_OVERRIDE {smc = IdeaSMC::withDevice(this, ec);};
#endif

    /**
     *  Update battery ID
     *
     *  Method(GBID, 0, Serialized) will return a package in following format, while value
     *  with 0xff may be considered invalid. Four groups of 16 bits in ID field are filled
     *  with following values:
     *
     *  BMIL/BMIH Battery Manufacturer
     *  HIDL/HIDH Hardware ID
     *  FMVL/FMVH Firmware Version
     *  DAVL/DAVH ?
     *
     *  Name (BFIF, Package (0x04)
     *  {
     *      Buffer (0x02)
     *      {
     *           0x00, 0x00                                       // Battery 0 Cycle Count
     *      },
     *
     *      Buffer (0x02)
     *      {
     *           0xFF, 0xFF                                       // Battery 1 Cycle Count
     *      },
     *
     *      Buffer (0x08)
     *      {
     *           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00   // Battery 0 ID
     *      },
     *
     *      Buffer (0x08)
     *      {
     *           0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF   // Battery 1 ID
     *      }
     *  })
     *
     *  @param bat0 Dictionary for Battery 0
     *  @param bat1 Dictionary for Battery 1
     *
     *  @return true if success
     */
    bool updateBatteryID(OSDictionary *bat0, OSDictionary *bat1);

    /**
     *  Update battery information
     *
     *  @param bat0 Dictionary for Battery 0
     *  @param bat1 Dictionary for Battery 1
     */
    bool updateBatteryInfo(OSDictionary *bat0, OSDictionary *bat1);

    /**
     *  Update battery stats
     *
     *  @param batState battery state from GBMD
     */
    void updateBatteryStats(UInt32 batState);

    /**
     *  Update keyboard stats
     *
     *  @param kbdState keyboard state from HALS
     */
    void updateKeyboardStats(UInt32 kbdState);
    /**
     *  Update battery conservation mode status
     *
     *  @param update only update internal status when false
     *
     *  @return true if success
     */
    bool updateBattery(bool update=true);

    /**
     *  Update rapid charge mode status
     *
     *  @param update only update internal status when false
     *
     *  @return true if success
     */
    bool updateRapidCharge(bool update=true);

    /**
     *  Update Keyboard status
     *
     *  @param update only update internal status when false
     *
     *  @return true if success
     */
    bool updateKeyboard(bool update=false);

    inline bool updateBacklight(bool update=false) APPLE_KEXT_OVERRIDE {return updateKeyboard(update);};
    bool setBacklight(UInt32 level) APPLE_KEXT_OVERRIDE;

    /**
     *  Toggle battery conservation mode
     *
     *  @return true if success
     */
    bool toggleConservation();

    /**
     *  Toggle rapid charge mode
     *
     *  @return true if success
     */
    bool toggleRapidCharge();

    /**
     *  Toggle Fn lock mode
     *
     *  @return true if success
     */
    bool toggleFnlock();

    /**
     *  Read EC data
     *
     *  @param cmd see VPCCMD
     *  @param result EC read result
     *  @param retries number of retries when reading, timeout after 5 retires or IDEAPAD_EC_TIMEOUT
     *
     *  @return true if success
     */
    bool read_ec_data(UInt32 cmd, UInt32 *result, UInt8 *retries);

    /**
     *  Write EC data
     *
     *  @param cmd see VPCCMD
     *  @param value EC write value
     *  @param retries number of retries when writing, timeout after 5 retires or IDEAPAD_EC_TIMEOUT
     *
     *  @return true if success
     */
    bool write_ec_data(UInt32 cmd, UInt32 value, UInt8 *retries);

    /**
     *  Write EC data
     *
     *  @param cmd 0 for value, 1 for VMCCMD
     *  @param data EC value or VMCCMD
     *
     *  @return true if success
     */
    bool method_vpcw(UInt32 cmd, UInt32 data);

    /**
     *  Read EC data
     *
     *  @param cmd 1 for status, 0 for value
     *  @param result write status (0) or EC value
     *
     *  @return true if success
     */
    bool method_vpcr(UInt32 cmd, UInt32 *result);

public:
    IOReturn message(UInt32 type, IOService *provider, void *argument) APPLE_KEXT_OVERRIDE;
};

#endif /* IdeaVPC_hpp */
