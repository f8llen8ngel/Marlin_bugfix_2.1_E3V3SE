/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2020 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * Based on Sprinter and grbl.
 * Copyright (c) 2011 Camiel Gubbels / Erik van der Zalm
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, ANY version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */
/**
 * DWIN by Creality3D
 */

#include "../../../inc/MarlinConfigPre.h"
#include "../../../inc/MarlinConfig.h"
#if ENABLED(DWIN_CREALITY_LCD)
#include "dwin.h"
#include "ui_position.h" //Ui position
#if ANY(AUTO_BED_LEVELING_BILINEAR, AUTO_BED_LEVELING_LINEAR, AUTO_BED_LEVELING_3POINT) && DISABLED(PROBE_MANUALLY)
#define HAS_ONESTEP_LEVELING 1
#endif
#if ANY(BABYSTEPPING, HAS_BED_PROBE, HAS_WORKSPACE_OFFSET)
#define HAS_ZOFFSET_ITEM 1
#endif
#if !HAS_BED_PROBE && ENABLED(BABYSTEPPING)
#define JUST_BABYSTEP 1
#endif
#include <stdio.h>
#include <string.h>
#include "fontutils.h"
#include "../../marlinui.h"
#include "../../../sd/cardreader.h"
#include "../../../MarlinCore.h"
#include "../../../core/serial.h"
#include "../../../core/macros.h"
#include "../../../gcode/queue.h"
#include "../../../module/temperature.h"
#include "../../../module/printcounter.h"
#include "../../../module/motion.h"
#include "../../../module/planner.h"
#if ENABLED(EEPROM_SETTINGS)
#include "../../../module/settings.h"
#endif
#if ENABLED(HOST_ACTION_COMMANDS)
#include "../../../feature/host_actions.h"
#endif
#if HAS_ONESTEP_LEVELING
#include "../../../feature/bedlevel/bedlevel.h"
#endif
#if HAS_BED_PROBE
#include "../../../module/probe.h"
#endif
#include "../../../feature/babystep.h"

#if ENABLED(POWER_LOSS_RECOVERY)
#include "../../../feature/powerloss.h"
#endif

#include "../../../module/AutoOffset.h"

// #include <QRCodeGenerator.h>

#ifndef MACHINE_SIZE
#define MACHINE_SIZE STRINGIFY(X_BED_SIZE) "x" STRINGIFY(Y_BED_SIZE) "x" STRINGIFY(Z_MAX_POS)
#endif
#ifndef CORP_WEBSITE
#define CORP_WEBSITE WEBSITE_URL
#endif

#define CORP_WEBSITE_C "github.com/navaismo"
#define CORP_WEBSITE_E "github.com/navaismo"
#define PAUSE_HEAT
#define CHECKFILAMENT true

#define USE_STRING_HEADINGS
// #define USE_STRING_TITLES

#define DWIN_FONT_MENU font8x16
#define DWIN_FONT_STAT font10x20
#define DWIN_FONT_HEAD font10x20
#define DWIN_MIDDLE_FONT_STAT font8x16
#define MENU_CHAR_LIMIT 24

// Print speed limit
#define MIN_PRINT_SPEED 10
#define MAX_PRINT_SPEED 999

// Feedspeed limit (max feedspeed = DEFAULT_MAX_FEEDRATE *2)
#define MIN_MAXFEEDSPEED 1
#define MIN_MAXACCELERATION 1
#define MIN_MAXJERK 0.1
#define MIN_STEP 1

#define FEEDRATE_E (60)

// Minimum unit (0.1) : multiple (10)
#define UNITFDIGITS 1
#define MINUNITMULT pow(10, UNITFDIGITS)

#define ENCODER_WAIT_MS 20
#define DWIN_VAR_UPDATE_INTERVAL 1024
#define DACAI_VAR_UPDATE_INTERVAL 4048
#define HEAT_ANIMATION_FLASH 150                   // Heating animation refresh
#define DWIN_SCROLL_UPDATE_INTERVAL SEC_TO_MS(0.5) // Rock 20210819
#define DWIN_REMAIN_TIME_UPDATE_INTERVAL SEC_TO_MS(20)

#define PRINT_SET_OFFSET 4 // Rock 20211115
#define TEMP_SET_OFFSET 2  // Rock 20211115
#define PID_VALUE_OFFSET 10
#if ENABLED(DWIN_CREALITY_480_LCD)
#define JPN_OFFSET -13                           // Rock 20211115
constexpr uint16_t TROWS = 6, MROWS = TROWS - 1, // Total rows, and other-than-Back
    TITLE_HEIGHT = 30,                           // Title bar height
    MLINE = 53,                                  // Menu line height
    LBLX = 58,                                   // Menu item label X
    MENU_CHR_W = 8, STAT_CHR_W = 10;
#define MBASE(L) (49 + MLINE * (L))
#elif ENABLED(DWIN_CREALITY_320_LCD)
#define JPN_OFFSET -5 // Rock 20211115
constexpr uint16_t TROWS = 6, MROWS = TROWS - 1, // Total rows, and other-than-Back
    TITLE_HEIGHT = 24,                           // Title bar height
    MLINE = 36,                                  // Menu line height
    LBLX = 42,                                   // Menu item label X
    MENU_CHR_W = 8, STAT_CHR_W = 10,
                   MROWS2 = 6;
#define MBASE(L) (34 + MLINE * (L))
#endif

#define font_offset 19
#define BABY_Z_VAR TERN(HAS_BED_PROBE, probe.offset.z, dwin_zoffset)

char shift_name[101];
char current_file_name[30];
static char *print_name = card.longest_filename();
static uint8_t print_len_name = strlen(print_name);
int8_t shift_amt;  // = 0
millis_t shift_ms; // = 0
static uint8_t left_move_index = 0;

bool isPaused = false;

// bool qrShown = false;
#if ENABLED(PREHEAT_ALERT)
  bool preheat_flag = false;
  uint8_t material_index = 0;
#endif

/* Value Init */
HMI_value_t HMI_ValueStruct;
HMI_Flag_t HMI_flag{0};
millis_t dwin_heat_time = 0;
uint8_t G29_level_num = 0; // Record how many points g29 has been leveled to determine whether g29 is leveled normally.
bool end_flag = false;     // Prevent repeated refresh of curve completion instructions
enum DC_language current_language;
volatile uint8_t checkkey = 0;
// 0 Without interruption, 1 runout filament paused 2 remove card pause
static bool temp_remove_card_flag = false, temp_cutting_line_flag = false /*,temp wifi print flag=false*/;

bool hasThumbnail = false;
bool OctoRefresh = false;
    
typedef struct
{
  uint8_t now, last;
  void set(uint8_t v) { now = last = v; }
  void reset() { set(0); }
  bool changed()
  {
    bool c = (now != last);
    if (c)
      last = now;
    return c;
  }
  bool dec()
  {
    if (now)
      now--;
    return changed();
  }
  bool inc(uint8_t v)
  {
    if (now < (v - 1))
      now++;
    else
      now = (v - 1);
    return changed();
  }
} select_t;

typedef struct
{
  char filename[FILENAME_LENGTH];
  char longfilename[LONG_FILENAME_LENGTH];
} PrintFile_InfoTypeDef;

select_t select_page{0}, select_file{0}, select_print{0}, select_prepare{0}, select_control{0}, select_axis{0}, select_temp{0}, select_motion{0}, select_tune{0}, select_advset{0}, select_PLA{0}, select_ABS{0}, select_TPU{0},  select_PETG{0},  
        select_speed{0}, select_acc{0}, select_jerk{0}, select_step{0}, 
        select_input_shaping{0}, 
        select_skew{0}, 
        select_cextr{0}, 
        select_display{0}, 
        select_item{0}, select_language{0}, select_hm_set_pid{0}, select_set_pid{0}, select_level{0}, select_show_pic{0};

uint8_t index_file = MROWS,
        index_prepare = MROWS,
        index_control = MROWS,
        index_leveling = MROWS,
        index_tune = MROWS,
        index_advset = MROWS,
        index_language = MROWS + 2,
        index_temp = MROWS,
        index_pid = MROWS,
        index_select = 0;
bool dwin_abort_flag = false; // Flag to reset feedrate, return to Home

constexpr float default_max_feedrate[] = DEFAULT_MAX_FEEDRATE;
constexpr float default_max_acceleration[] = DEFAULT_MAX_ACCELERATION;

#if HAS_CLASSIC_JERK
constexpr float default_max_jerk[] = {DEFAULT_XJERK, DEFAULT_YJERK, DEFAULT_ZJERK, DEFAULT_EJERK};
#endif

uint8_t Cloud_Progress_Bar = 0; // The cloud prints the transmitted progress bar data

float default_nozzle_ptemp = DEFAULT_KP;
float default_nozzle_itemp = DEFAULT_KI;
float default_nozzle_dtemp = DEFAULT_KD;

float default_hotbed_ptemp = DEFAULT_BED_KP;
float default_hotbed_itemp = DEFAULT_BED_KI;
float default_hotbed_dtemp = DEFAULT_BED_KD;
uint16_t auto_bed_pid = 100, auto_nozzle_pid = 260;

#if ENABLED(PAUSE_HEAT)
#if ENABLED(HAS_HOTEND)
uint16_t resume_hotend_temp = 0;
#endif
#if ENABLED(HAS_HEATED_BED)
uint16_t resume_bed_temp = 0;
#endif
#endif

#if HAS_ZOFFSET_ITEM
float dwin_zoffset = 0, last_zoffset = 0;
float dwin_zoffset_edit = 0, last_zoffset_edit = 0, temp_zoffset_single = 0; // The leveling value before adjustment of the current point;
#endif

int16_t temphot = 0;
uint8_t afterprobe_fan0_speed = 0;
bool home_flag = false;
bool G29_flag = false;

#if ENABLED(DWIN_ZHOME_MENU)
  uint8_t CZ_AFTER_HOMING = Z_AFTER_HOMING;
#endif

#define DWIN_BOOT_STEP_EEPROM_ADDRESS 0x01 // Set up boot steps
#define DWIN_LANGUAGE_EEPROM_ADDRESS 0x02  // Between 0x01 and 0x63 (EEPROM_OFFSET-1)
#define DWIN_AUTO_BED_EEPROM_ADDRESS 0x03  // Hot bed automatic pid target value
#define DWIN_AUTO_NOZZ_EEPROM_ADDRESS 0x05 // Nozzle automatic pid target value

void Draw_Leveling_Highlight(const bool sel)
{
  HMI_flag.select_flag = sel;
  const uint16_t c1 = sel ? Button_Select_Color : Color_Bg_Black,
                 c2 = sel ? Color_Bg_Black : Button_Select_Color;
  // DWIN_Draw_Rectangle(0, c1, 25, 306, 126, 345);
  // DWIN_Draw_Rectangle(0, c1, 24, 305, 127, 346);
  DWIN_Draw_Rectangle(0, c1, BUTTON_EDIT_X, BUTTON_EDIT_Y, BUTTON_EDIT_X + BUTTON_W - 1, BUTTON_OK_Y + BUTTON_H - 1);
  DWIN_Draw_Rectangle(0, c1, BUTTON_EDIT_X - 1, BUTTON_EDIT_Y - 1, BUTTON_EDIT_X + BUTTON_W, BUTTON_EDIT_Y + BUTTON_H);
  // DWIN_Draw_Rectangle(0, c2, 146, 306, 246, 345);
  // DWIN_Draw_Rectangle(0, c2, 145, 305, 247, 346);
  DWIN_Draw_Rectangle(0, c2, BUTTON_OK_X, BUTTON_OK_Y, BUTTON_OK_X + BUTTON_W - 1, BUTTON_OK_Y + BUTTON_H - 1);
  DWIN_Draw_Rectangle(0, c2, BUTTON_OK_X - 1, BUTTON_OK_Y - 1, BUTTON_OK_X + BUTTON_W, BUTTON_OK_Y + BUTTON_H);
}
// RUN_AND_WAIT_GCODE_CMD("M24", true);
// pause_resume_feedstock(FEEDING_DEF_DISTANCE,FEEDING_DEF_SPEED);
static void pause_resume_feedstock(uint16_t _distance, uint16_t _feedRate)
{
  char cmd[20], str_1[16];
  motion.position[E_AXIS] += _distance;
  motion.goto_current_position(feedRate_t(_feedRate));
  motion.position[E_AXIS] -= _distance;
  memset(cmd, 0, sizeof(cmd));
  sprintf_P(cmd, PSTR("G92.9E%s"), dtostrf(motion.position[E_AXIS], 1, 3, str_1));
  gcode.process_subcommands_now(cmd);
  memset(cmd, 0, sizeof(cmd));
  // Resume the feedrate
  sprintf_P(cmd, PSTR("G1 F%d"), int(MMS_TO_MMM(motion.feedrate_mm_s) + 0.5f));
  gcode.process_subcommands_now(cmd);
}

void In_out_feedtock_level(uint16_t _distance, uint16_t _feedRate, bool dir)
{
  char cmd[20]; //str_1[16];
  const float olde = motion.position.e;
  if (dir)
  {
    motion.position.e += _distance;
    motion.goto_current_position(feedRate_t(_feedRate));
  }
  else // Withdraw
  {
    motion.position.e -= _distance;
    motion.goto_current_position(feedRate_t(_feedRate));
  }
  motion.position.e = olde;
  planner.set_e_position_mm(olde);
  planner.synchronize();
  sprintf_P(cmd, PSTR("G1 F%s"), getStr(motion.feedrate_mm_s)); // Set original speed
  gcode.process_subcommands_now(cmd);
}

void In_out_feedtock(uint16_t _distance, uint16_t _feedRate, bool dir)
{
  char cmd[20]; //str_1[16];
  float olde = motion.position.e, differ_value = 0;
  if (motion.position.e < _distance)
    differ_value = (_distance - motion.position.e);
  else
    differ_value = 0;
  if (dir)
  {
    motion.position.e += _distance;
    motion.goto_current_position(feedRate_t(_feedRate));
  }
  else // Withdraw
  {
    if (differ_value)
    {
      motion.position.e += differ_value;
      motion.goto_current_position(feedRate_t(FEEDING_DEF_SPEED)); // The speed is too fast and there is noise
      planner.synchronize();
    }
    motion.position.e -= _distance;
    motion.goto_current_position(feedRate_t(_feedRate));
  }
  motion.position.e = olde;
  planner.set_e_position_mm(olde);
  planner.synchronize();
  sprintf_P(cmd, PSTR("G1 F%s"), getStr(motion.feedrate_mm_s)); // Set original speed
  gcode.process_subcommands_now(cmd);
  // RUN_AND_WAIT_GCODE_CMD(cmd, true);                  //Rock_20230821
}
/*
0 Automatic return: Preparation->Automatic return --->Heat to 260℃--"A pop-up box prompts that the material is being returned --->
            E-axis feeds 15mm---> E-axis withdraws 90mm---》A pop-up window prompts to manually remove the material and cool it to 140℃----》
           The window returns to the preparation page
1 Automatic feeding: Preparation -> Automatic feeding ---> First heat to 240℃--》The pop-up box prompts to manually insert the material (oblique mouth 45°) and press
           ----> Click OK ---> E-axis feeds 90mm ----》 The window returns to the preparation page and cools down to 140℃
*/
static void Auto_in_out_feedstock(bool dir) // 0 returns material, 1 feeds
{
  if (dir) // Feed
  {
    Popup_Window_Feedstork_Tip(1); // Feeding tips
    // show
    SET_HOTEND_TEMP(FEED_TEMPERATURE, 0); // First heat to 240℃
    WAIT_HOTEND_TEMP(60 * 5 * 1000, 5);   // Wait for the nozzle temperature to reach the set value
    Popup_Window_Feedstork_Finish(1);     // Feed confirmation
    HMI_flag.Auto_inout_end_flag = true;
    checkkey = AUTO_IN_FEEDSTOCK;
    DWIN_ICON_Not_Filter_Show(HMI_flag.language, LANGUAGE_Confirm, 79, 264); // OK button
    // Return to preparation page
  }
  else // Return material
  {
    Popup_Window_Feedstork_Tip(0);                                    // Return tips
    SET_HOTEND_TEMP(EXIT_TEMPERATURE, 0);                             // First heat to 260℃
    WAIT_HOTEND_TEMP(60 * 5 * 1000, 5);                               // Wait for the nozzle temperature to reach the set value
    In_out_feedtock(FEEDING_DEF_DISTANCE_1, FEEDING_DEF_SPEED, true); // Feed 15mm
    In_out_feedtock(IN_DEF_DISTANCE, FEEDING_DEF_SPEED / 2, false);   // Withdraw 90mm
    Popup_Window_Feedstork_Finish(0);                                 // Return confirmation
    HMI_flag.Auto_inout_end_flag = true;
    checkkey = AUTO_OUT_FEEDSTOCK;
    DWIN_ICON_Not_Filter_Show(HMI_flag.language, LANGUAGE_Confirm, 79, 264); // OK button
    SET_HOTEND_TEMP(STOP_TEMPERATURE, 0);                                    // Cool down to 140℃
    // WAIT_HOTEND_TEMP(60 *5 *1000, 5); //Wait for the nozzle temperature to reach the set value
  }
}

#if ENABLED(PREHEAT_ALERT)
  // Preheat finished alert
  void Preheat_alert(uint8_t material){
    if( preheat_flag && thermalManager.degHotend(0) >= ui.material_preset[material].hotend_temp && thermalManager.degBed() >= ui.material_preset[material].bed_temp){
      // beep to alert process finished
      Generic_BeepAlert();
      delay(200);
      Generic_BeepAlert();
      preheat_flag = false;
    }
  }
#endif

#if ENABLED(DWIN_CUSTOM_EXTRUDE)
    // Custom Extrude Process
    static void Custom_Extrude_Process(uint16_t temp, uint16_t length) // Extrude material based on user temp & length
    {
      HMI_flag.Refresh_bottom_flag = false;
      char str[25]; // Sufficient buffer for string and number
      snprintf(str, sizeof(str), "Extruding %u mm", length);
      Popup_Window_Feedstork_Tip(1); // Feeding tips
      // SERIAL_ECHOLNPGM("Extruding: ", str);
      // SERIAL_ECHOLNPGM("CurrTEMP: ", thermalManager.degHotend(0));
      // SERIAL_ECHOLNPGM("TargetTEMP: ", temp);
      // SERIAL_ECHOLNPGM("Length: ", length);
      // SERIAL_ECHOLNPGM("Termal Temp", thermalManager.degTargetHotend(0));
      SET_HOTEND_TEMP(temp, 0);           // First heat to Target Temp
      WAIT_HOTEND_TEMP(60 * 5 * 1000, 3); // Wait until the hotend temperature reaches the target temperature

      delay(1000);                                                                          // Wait for 1s
      Clear_Title_Bar();                                                                    // Clear title bar
      DWIN_Draw_String(false, false, DWIN_FONT_HEAD, Color_Red, Color_Bg_Blue, 10, 4, str); // Draw title

      In_out_feedtock(length, FEEDING_DEF_SPEED, true);                        // Feed material
      delay(1000);                                                             // Wait for 1s
      Clear_Title_Bar();                                                       // Clear title bar
      Popup_Window_Feedstork_Finish(1);                                        // Feed confirmation
      DWIN_ICON_Not_Filter_Show(HMI_flag.language, LANGUAGE_Confirm, 79, 264); // OK button

      SET_HOTEND_TEMP(STOP_TEMPERATURE, 0); // Cool down to 140℃
      checkkey = M117Info;
    }
#endif    

#if ENABLED(OCTOPRINT_PLUGIN)
  uint16_t OctoImageLine[OctoIMAGE_WIDTH];
  //vars to scroll title when octoprinting
  uint8_t scrollOffset = 0;
  millis_t lastScrollTime = 0;
  const int scrollDelay2 = 250; // this to move chars
  char visibleText[31] = {0};

  //clear the image map to black
  void initializeImageMap() {
    memset(OctoImageLine, 0, sizeof(OctoImageLine));
    //SERIAL_ECHOLN("OctoImageLine initialized to 0:");
  }

  // Function to create a shortened filename without extension
  void octo_make_name_without_ext(char *dst, char *src, size_t maxlen = MENU_CHAR_LIMIT)
  {    
    size_t pos = strlen(src); // index of ending nul
    // For files, remove the extension
    // which may be .gcode, .gco, or .g
      while (pos && src[pos] != '.')
        pos--; // find last '.' (stop at 0)

    size_t len = pos; // nul or '.'
    if (len > maxlen)
    {                     // Keep the name short
      pos = len = maxlen; // move nul down
      dst[--pos] = '.';   // insert dots
      dst[--pos] = '.';
      dst[--pos] = '.';
    }

    dst[len] = '\0'; // end it

    // Copy down to 0
    while (pos--)
      dst[pos] = src[pos];
  }

  // Draw the octoprint title
  void Draw_OctoTitle(const char *const title)
  {
    char* nTitle = const_cast<char*>(title);
    octo_make_name_without_ext(shift_name, nTitle, sizeof(shift_name) - 1); // Copy to bounded buffer
    DWIN_Draw_String(false, false, DWIN_FONT_HEAD, Color_Yellow, Color_Bg_Black, 4, 4, shift_name);
  }

  //scroll title name
  void octoUpdateScroll() {
      if (strlen(shift_name) <= 30) return; // No need to update if filename is less than 30chars

      const uint8_t maxOffset = strlen(shift_name) - 30;
      const millis_t currentTime = millis(); // check interval
      if (currentTime - lastScrollTime >= scrollDelay2) {
          lastScrollTime = currentTime;
          
          Clear_Title_Bar(); //clear title bar to avoid ghosting text
          strncpy(visibleText, shift_name + scrollOffset, 30); // copy the text to shift left
          visibleText[30] = '\0';
          // Draw the string
          DWIN_Draw_String(false, false, DWIN_FONT_HEAD, Color_Yellow, Color_Bg_Black, 4, 4, visibleText);
          
          // Inc and reset
          scrollOffset++;
          if (scrollOffset > maxOffset) {
              scrollOffset = 0;  // Restart
          }
      }
  }

#endif

/*Get the specified g file information *short_file_name: short file name *file: file information pointer Return value*/
void get_file_info(char *short_file_name, PrintFile_InfoTypeDef *file)
{
  if (!card.isMounted())
  {
    return;
  }
  // Get root directory
  card.getWorkDirName();
  if (card.filename[0] != '/')
  {
    card.cdup();
  }
  // Select file
  card.selectFileByName(short_file_name);
  // Get gcode file information
  strcpy(file->filename, card.filename);
  strcpy(file->longfilename, card.longFilename);
}

inline bool HMI_IsJapanese() { return HMI_flag.language == DACAI_JAPANESE; }

void HMI_SetLanguageCache()
{
  // DWIN_JPG_CacheTo1(HMI_IsJapanese() ? Language_Chinese : Language_English);
}

void HMI_ResetLanguage()
{
  HMI_flag.language = Language_Max;
  HMI_flag.boot_step = Set_language;
  Save_Boot_Step_Value(); // Save boot steps
  BL24CXX::write(DWIN_LANGUAGE_EEPROM_ADDRESS, (uint8_t *)&HMI_flag.language, sizeof(HMI_flag.language));
  HMI_SetLanguageCache();
}
static void __attribute__((unused)) HMI_ResetDevice()
{
  // uint8_t current_device = DEVICE_UNKNOWN; //Add this way temporarily
  // BL24CXX::write(LASER_FDM_ADDR, (uint8_t *)&current_device, 1);
}
static void Read_Boot_Step_Value()
{
  // SERIAL_ECHOLNPGM(" >>> Read_Boot_Step_Value");
#if ENABLED(EEPROM_SETTINGS) && ENABLED(IIC_BL24CXX_EEPROM)
  // First read the value of hmi flag.boot step to determine whether booting is required.
  BL24CXX::read(DWIN_BOOT_STEP_EEPROM_ADDRESS, (uint8_t *)&HMI_flag.boot_step, sizeof(HMI_flag.boot_step));
  // Read language configuration items
  BL24CXX::read(DWIN_LANGUAGE_EEPROM_ADDRESS, (uint8_t *)&HMI_flag.language, sizeof(HMI_flag.language));
#endif
  // SERIAL_ECHOLNPGM(" HMI_flag.boot_step: ", HMI_flag.boot_step);
  // SERIAL_ECHOLNPGM(" HMI_flag.language: ", HMI_flag.language);
  if (HMI_flag.boot_step != Boot_Step_Max)
  {
    HMI_flag.Need_boot_flag = true; // Requires booting
    HMI_flag.boot_step = Set_language;
    HMI_flag.language = English;
  }
  else
  {
    HMI_flag.Need_boot_flag = false; // No boot required
    HMI_StartFrame(true);            // Jump to the main interface
  }
}

void Save_Boot_Step_Value()
{
  // SERIAL_ECHOLNPGM(" >>> Save_Boot_Step_Value");
  // SERIAL_ECHOLNPGM(" HMI_flag.boot_step: ", HMI_flag.boot_step);
#if ENABLED(EEPROM_SETTINGS) && ENABLED(IIC_BL24CXX_EEPROM)

  BL24CXX::write(DWIN_BOOT_STEP_EEPROM_ADDRESS, (uint8_t *)&HMI_flag.boot_step, sizeof(HMI_flag.boot_step));
  // SERIAL_ECHOLNPGM(" Saved HMI_flag.boot_step to EEPROM ", HMI_flag.boot_step);
#endif
}
static void Read_Auto_PID_Value()
{
  BL24CXX::read(DWIN_AUTO_BED_EEPROM_ADDRESS, (uint8_t *)&HMI_ValueStruct.Auto_PID_Value + 2, sizeof(HMI_ValueStruct.Auto_PID_Value[1]));
  BL24CXX::read(DWIN_AUTO_NOZZ_EEPROM_ADDRESS, (uint8_t *)&HMI_ValueStruct.Auto_PID_Value + 4, sizeof(HMI_ValueStruct.Auto_PID_Value[2]));
  LIMIT(HMI_ValueStruct.Auto_PID_Value[1], 60, BED_MAX_TARGET);
  LIMIT(HMI_ValueStruct.Auto_PID_Value[2], 100, thermalManager.hotend_max_target(0));
}
static void Save_Auto_PID_Value()
{
  BL24CXX::write(DWIN_AUTO_BED_EEPROM_ADDRESS, (uint8_t *)&HMI_ValueStruct.Auto_PID_Value + 2, sizeof(HMI_ValueStruct.Auto_PID_Value[1]));
  BL24CXX::write(DWIN_AUTO_NOZZ_EEPROM_ADDRESS, (uint8_t *)&HMI_ValueStruct.Auto_PID_Value + 4, sizeof(HMI_ValueStruct.Auto_PID_Value[2]));
}

void HMI_SetLanguage()
{
  // rock_901122 Solve the problem of language confusion when powering on for the first time
  // if(HMI_flag.language > Portuguese)
  reset_flag = true;
  if (HMI_flag.Need_boot_flag || (HMI_flag.language < Chinese) || (HMI_flag.language >= Language_Max))
  {
    // Draw_Mid_Status_Area(true); //rock_20230529
    HMI_flag.language = English;
    select_language.reset();
    index_language = MROWS + 1;
    Draw_Poweron_Select_language();
    checkkey = Poweron_select_language;
  }
  else
  {
    // Hmi start frame(true);
  }
  // Hmi set language cache();
}

static uint8_t Move_Language(uint8_t curr_language)
{
  switch (curr_language)
  {
  case Chinese:
    curr_language = English;
    break;

  case English:
    curr_language = German;
    break;

  case German:
    curr_language = Russian;
    break;

  case Russian:
    curr_language = French;
    break;

  case French:
    curr_language = Turkish;
    break;

  case Turkish:
    curr_language = Spanish;
    break;

  case Spanish:
    curr_language = Italian;
    break;

  case Italian:
    curr_language = Portuguese;
    break;

  case Portuguese:
    curr_language = Japanese;
    break;

  case Japanese:
    curr_language = Korean;
    break;
  case Korean:
    curr_language = Chinese;
    break;

  default:
    curr_language = English;
    break;
  }
  return curr_language;
}

void HMI_ToggleLanguage()
{
  HMI_flag.language = Move_Language(HMI_flag.language);
  // SERIAL_ECHOLNPGM("HMI_flag.language=: ", HMI_flag.language);
// HMI_flag.language = HMI_IsJapanese() ? DWIN_ENGLISH : DACAI_JAPANESE;

// Hmi set language cache();
#if ENABLED(EEPROM_SETTINGS) && ENABLED(IIC_BL24CXX_EEPROM)
  BL24CXX::write(DWIN_LANGUAGE_EEPROM_ADDRESS, (uint8_t *)&HMI_flag.language, sizeof(HMI_flag.language));
#endif
}

// Show title in print
static void Show_JPN_print_title(void)
{
  if (HMI_flag.language < Language_Max)
  {
    Clear_Title_Bar();
    DWIN_ICON_Show(HMI_flag.language, LANGUAGE_Printing, TITLE_X, TITLE_Y);
  }
}

static void Show_JPN_pause_title(void)
{
  if (HMI_flag.language < Language_Max)
  {
    Clear_Title_Bar();
    DWIN_ICON_Show(HMI_flag.language, LANGUAGE_Pausing, TITLE_X, TITLE_Y);
  }
}

#if ENABLED(SHOW_GRID_VALUES) //

void DWIN_Draw_Z_Offset_Float(uint8_t size, uint16_t color, uint16_t bcolor, uint8_t iNum, uint8_t fNum, uint16_t x, uint16_t y, long value)
{
#if ENABLED(COMPACT_GRID_VALUES)
  char valueStr[48] = "\0";
  if (value < 0)
  {
    DWIN_Draw_String(false, true, font6x12, bcolor == Color_Bg_Black ? Color_Blue : color, bcolor, x + 5, y, F("-")); // draw minus sign above
    value *= -1;
  }
  else
  {
    DWIN_Draw_String(false, true, font6x12, bcolor == Color_Bg_Black ? Color_Red : color, bcolor, x + 5, y, F("+")); // draw plus sign above
  }
  if (value < 100)
  {
    sprintf_P(valueStr, ".%02d", static_cast<int>(value & 0xFF));
  }
  else
  {
    sprintf_P(valueStr, "%d.%d", static_cast<int>((value & 0xFF) / 100), static_cast<int>(((value & 0xFF) % 100) / 10));
  }
  DWIN_Draw_String(false, true, size, color, bcolor, x, y + 8, F(valueStr));
#else  // COMPACT_GRID_VALUES
  if (value < 0)
  {
    DWIN_Draw_FloatValue(true, true, 0, size, color, bcolor, iNum, fNum, x + 1, y, -value);
    DWIN_Draw_String(false, true, font6x12, color, bcolor, x, y, F("-"));
  }
  else
  {
    DWIN_Draw_FloatValue(true, true, 0, size, color, bcolor, iNum, fNum, x, y, value);
  }
#endif // COMPACT_GRID_VALUES
}
#endif
void DWIN_Draw_Signed_Float(uint8_t size, uint16_t bColor, uint8_t iNum, uint8_t fNum, uint16_t x, uint16_t y, long value)
{
  if (value < 0)
  {
    DWIN_Draw_FloatValue(true, true, 0, size, Color_White, bColor, iNum, fNum, x, y + 2, -value);
    DWIN_Draw_String(false, true, font6x12, Color_White, bColor, x + 2, y + 2, F("-"));
  }
  else
  {
    DWIN_Draw_FloatValue(true, true, 0, size, Color_White, bColor, iNum, fNum, x, y + 2, value);
    DWIN_Draw_String(false, true, font6x12, Color_White, bColor, x + 2, y + 2, F(""));
  }
}
void DWIN_Draw_Signed_Float_Temp(uint8_t size, uint16_t bColor, uint8_t iNum, uint8_t fNum, uint16_t x, uint16_t y, long value)
{
  if (value < 0)
  {
    DWIN_Draw_FloatValue(true, true, 0, size, Color_Yellow, bColor, iNum, fNum, x, y + 2, -value);
    DWIN_Draw_String(false, true, font6x12, Color_Yellow, bColor, x + 2, y + 2, F("-"));
  }
  else
  {
    DWIN_Draw_FloatValue(true, true, 0, size, Color_Yellow, bColor, iNum, fNum, x, y + 2, value);
    DWIN_Draw_String(false, true, font6x12, Color_Yellow, bColor, x + 2, y + 2, F(""));
  }
}
void ICON_Print()
{
#if ENABLED(DWIN_CREALITY_480_LCD)

#elif ENABLED(DWIN_CREALITY_320_LCD)
  // DWIN_ICON_Show(HMI_flag.language, LANGUAGE_Pausing, TITLE_X, TITLE_Y);
  if (select_page.now == 0)
  {
    DWIN_ICON_Not_Filter_Show(ICON, ICON_Print_1, ICON_PRINT_X, ICON_PRINT_Y);
    DWIN_Draw_Rectangle(0, Color_White, ICON_PRINT_X, ICON_PRINT_Y, ICON_PRINT_X + ICON_W, ICON_PRINT_Y + ICON_H);
    if (HMI_flag.language < Language_Max)
    {
      // rock_j print 1
      DWIN_ICON_Show(HMI_flag.language, LANGUAGE_Print_1, WORD_PRINT_X, WORD_PRINT_Y);
    }
  }
  else
  {
    DWIN_ICON_Not_Filter_Show(ICON, ICON_Print_0, ICON_PRINT_X, ICON_PRINT_Y);
    if (HMI_flag.language < Language_Max)
    {
      // rock_j print 0
      DWIN_ICON_Show(HMI_flag.language, LANGUAGE_Print_0, WORD_PRINT_X, WORD_PRINT_Y);
    }
  }
#endif
}

void ICON_Prepare()
{
#if ENABLED(DWIN_CREALITY_480_LCD)

#elif ENABLED(DWIN_CREALITY_320_LCD)
  if (select_page.now == 1)
  {
    DWIN_ICON_Not_Filter_Show(ICON, ICON_Prepare_1, ICON_PREPARE_X, ICON_PREPARE_Y);
    DWIN_Draw_Rectangle(0, Color_White, ICON_PREPARE_X, ICON_PREPARE_Y, ICON_PREPARE_X + ICON_W, ICON_PREPARE_Y + ICON_H);
    if (HMI_flag.language < Language_Max)
    {
      // DWIN_Frame_AreaCopy(1, 31, 447, 58, 460, 186, 201);
      // rock_j prepare 1
      DWIN_ICON_Show(HMI_flag.language, LANGUAGE_Prepare_1, WORD_PREPARE_X, WORD_PREPARE_Y);
    }
    else
    {
      DWIN_Frame_AreaCopy(1, 13, 239, 55, 241, 117, 109);
    }
  }
  else
  {
    DWIN_ICON_Not_Filter_Show(ICON, ICON_Prepare_0, ICON_PREPARE_X, ICON_PREPARE_Y);
    if (HMI_flag.language < Language_Max)
    {
      // DWIN_Frame_AreaCopy(1, 31, 405, 58, 420, 186, 201);
      // rock_j prepare 0
      DWIN_ICON_Show(HMI_flag.language, LANGUAGE_Prepare_0, WORD_PREPARE_X, WORD_PREPARE_Y);
    }
    else
    {
      DWIN_Frame_AreaCopy(1, 13, 203, 55, 205, 117, 109);
    }
  }
#endif
}

void ICON_Control()
{
#if ENABLED(DWIN_CREALITY_480_LCD)

#elif ENABLED(DWIN_CREALITY_320_LCD)
  if (select_page.now == 2)
  {
    DWIN_ICON_Not_Filter_Show(ICON, ICON_Control_1, ICON_CONTROL_X, ICON_CONTROL_Y);
    DWIN_Draw_Rectangle(0, Color_White, ICON_CONTROL_X, ICON_CONTROL_Y, ICON_CONTROL_X + ICON_W, ICON_CONTROL_Y + ICON_H);
    if (HMI_flag.language < Language_Max)
    {
      DWIN_ICON_Show(HMI_flag.language, LANGUAGE_Control_1, WORD_CONTROL_X, WORD_CONTROL_Y);
    }
  }
  else
  {
    DWIN_ICON_Not_Filter_Show(ICON, ICON_Control_0, ICON_CONTROL_X, ICON_CONTROL_Y);
    if (HMI_flag.language < Language_Max)
    {
      // rock_j control 0
      DWIN_ICON_Show(HMI_flag.language, LANGUAGE_Control_0, WORD_CONTROL_X, WORD_CONTROL_Y);
    }
    else
    {
      DWIN_Frame_AreaCopy(1, 56, 203, 88, 205, 32, 195);
    }
  }
#endif
}

void ICON_StartInfo(bool show)
{
  if (show)
  {
    // DWIN_ICON_Not_Filter_Show(ICON, ICON_Info_1, 145, 246);
    DWIN_Draw_Rectangle(0, Color_White, 145, 246, 254, 345);
    if (HMI_flag.language < Language_Max)
    {
      // Rock j
      DWIN_ICON_Show(HMI_flag.language, LANGUAGE_Info_1, 150, 318);
    }
    else
    {
      DWIN_Frame_AreaCopy(1, 132, 451, 159, 466, 186, 318);
    }
  }
  else
  {
    // DWIN_ICON_Not_Filter_Show(ICON, ICON_Info_0, 145, 246);
    if (HMI_flag.language < Language_Max)
    {
      // DWIN_Frame_AreaCopy(1, 91, 405, 118, 420, 186, 318);
      DWIN_ICON_Show(HMI_flag.language, LANGUAGE_Info_0, 150, 318);
    }
    else
    {
      DWIN_Frame_AreaCopy(1, 132, 423, 159, 435, 186, 318);
    }
  }
}
void Draw_Menu_Line_UP(uint8_t line, uint8_t icon, uint8_t picID, bool more)
{
  // Draw_Menu_Item(line, icon, picID, more);
  if (picID)
    DWIN_ICON_Show(HMI_flag.language, picID, 60, MBASE(line) + JPN_OFFSET);
  if (icon)
    Draw_Menu_Icon(line, icon);
  if (more)
    Draw_More_Icon(line);
  DWIN_Draw_Line(Line_Color, 16, MBASE(line) + 34, 256, MBASE(line) + 34);
}
// Rock 20210726
void ICON_Leveling(bool show)
{
#if ENABLED(DWIN_CREALITY_480_LCD)

#elif ENABLED(DWIN_CREALITY_320_LCD)
  if (show)
  {
    DWIN_ICON_Not_Filter_Show(ICON, ICON_Leveling_1, ICON_LEVEL_X, ICON_LEVEL_Y);
    DWIN_Draw_Rectangle(0, Color_White, ICON_LEVEL_X, ICON_LEVEL_Y, ICON_LEVEL_X + ICON_W, ICON_LEVEL_Y + ICON_H);
    if (HMI_flag.language < Language_Max)
    {
      if (HMI_flag.language == Russian)
      {
        DWIN_ICON_Show(HMI_flag.language, LANGUAGE_Level_1, WORD_LEVEL_X, WORD_LEVEL_Y);
      }
      else
      {
        DWIN_ICON_Show(HMI_flag.language, LANGUAGE_Level_1, WORD_LEVEL_X, WORD_LEVEL_Y);
      }
    }
    else
    {
      // Rock
      DWIN_Frame_AreaCopy(1, 56, 242, 80, 238, 121, 195);
    }
  }
  else
  {
    DWIN_ICON_Not_Filter_Show(ICON, ICON_Leveling_0, ICON_LEVEL_X, ICON_LEVEL_Y);
    if (HMI_flag.language < Language_Max)
    {
      // The Russian entry is too long and moved forward by 30 pixels.
      if (HMI_flag.language == Russian)
      {
        DWIN_ICON_Show(HMI_flag.language, LANGUAGE_Level_0, WORD_LEVEL_X, WORD_LEVEL_Y);
      }
      else
      {
        DWIN_ICON_Show(HMI_flag.language, LANGUAGE_Level_0, WORD_LEVEL_X, WORD_LEVEL_Y);
      }
    }
    else
    {
      DWIN_Frame_AreaCopy(1, 56, 169, 80, 170, 121, 195);
      // DWIN_Frame_AreaCopy(1, 84, 465, 120, 478, 182, 318);
    }
  }
#endif
}

void ICON_Tune()
{
#if ENABLED(DWIN_CREALITY_480_LCD)

#elif ENABLED(DWIN_CREALITY_320_LCD)
  if (select_print.now == 0)
  {
    DWIN_ICON_Not_Filter_Show(ICON, ICON_Setup_1, ICON_SET_X, ICON_SET_Y);
    DWIN_Draw_Rectangle(0, Color_White, ICON_SET_X, ICON_SET_Y, ICON_SET_X + ICON_W, ICON_SET_Y + ICON_H);
    if (HMI_flag.language < Language_Max)
    {
      DWIN_ICON_Show(HMI_flag.language, LANGUAGE_Tune_1, WORD_SET_X, WORD_SET_Y);
    }
  }
  else
  {
    DWIN_ICON_Not_Filter_Show(ICON, ICON_Setup_0, ICON_SET_X, ICON_SET_Y);
    if (HMI_flag.language < Language_Max)
    {
      DWIN_ICON_Show(HMI_flag.language, LANGUAGE_Tune_0, WORD_SET_X, WORD_SET_Y);
    }
    else
    {
      DWIN_Frame_AreaCopy(1, 56, 203, 88, 205, 32, 195);
    }
  }
#endif
}
// Dit is nu de allerlaatste regel van dwin.cpp geworden:
#endif // ENABLED(DWIN_CREALITY_LCD)
