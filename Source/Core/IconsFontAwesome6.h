/**
 * @file IconsFontAwesome6.h
 * @brief Font Awesome 6 Free icons for ImGui
 * @author Olympe Engine
 * @date 2026
 * 
 * @details
 * Font Awesome 6 Free icon definitions for use with ImGui.
 * Download Font Awesome from: https://fontawesome.com/download
 * Use the "Font Awesome 6 Free-Solid-900.otf" file
 * 
 * Usage:
 *   ImGui::MenuItem(ICON_FA_FILE " New File");
 *   ImGui::Button(ICON_FA_SAVE " Save");
 */

#pragma once

// Font Awesome 6 Free Solid icons
// Format: UTF-8 encoded Unicode characters

// File operations
#define ICON_FA_FILE "\xef\x85\x9b"                    // 
#define ICON_FA_FILE_CODE "\xef\x87\x89"               // 
#define ICON_FA_FOLDER "\xef\x81\xbb"                  // 
#define ICON_FA_FOLDER_OPEN "\xef\x81\xbc"             // 
#define ICON_FA_FLOPPY_DISK "\xef\x83\x87"             // 
#define ICON_FA_COPY "\xef\x83\x85"                    // 
#define ICON_FA_SCISSORS "\xef\x83\x84"                // 
#define ICON_FA_CLIPBOARD "\xef\x8c\xa8"               // 
#define ICON_FA_DOWNLOAD "\xef\x80\x99"                // 

// Edit operations
#define ICON_FA_PEN "\xef\x8c\x84"                     // 
#define ICON_FA_PEN_TO_SQUARE "\xef\x81\x84"           // 
#define ICON_FA_TRASH "\xef\x87\xb8"                   // 
#define ICON_FA_TRASH_CAN "\xef\x8a\xad"               // 
#define ICON_FA_PLUS "\x2b"                            // +
#define ICON_FA_MINUS "\x2d"                           // -
#define ICON_FA_XMARK "\xef\x80\x8d"                   // 

// Undo/Redo
#define ICON_FA_ROTATE_LEFT "\xef\x83\xa2"             // 
#define ICON_FA_ROTATE_RIGHT "\xef\x83\xa1"            // 
#define ICON_FA_ARROW_ROTATE_LEFT "\xef\x83\xa2"       // 
#define ICON_FA_ARROW_ROTATE_RIGHT "\xef\x80\x9e"      // 

// View operations
#define ICON_FA_EYE "\xef\x81\xae"                     // 
#define ICON_FA_EYE_SLASH "\xef\x81\xb0"               // 
#define ICON_FA_MAGNIFYING_GLASS "\xef\x80\x82"        // 
#define ICON_FA_MAGNIFYING_GLASS_PLUS "\xef\x80\x8a"   // 
#define ICON_FA_MAGNIFYING_GLASS_MINUS "\xef\x80\x90"  // 
#define ICON_FA_ARROWS_UP_DOWN_LEFT_RIGHT "\xef\x81\x87" // 
#define ICON_FA_EXPAND "\xef\x81\xa5"                  // 

// Navigation
#define ICON_FA_HOUSE "\xef\x80\x95"                   // 
#define ICON_FA_ARROW_LEFT "\xef\x81\x80"              // 
#define ICON_FA_ARROW_RIGHT "\xef\x81\x81"             // 
#define ICON_FA_ARROW_UP "\xef\x81\x82"                // 
#define ICON_FA_ARROW_DOWN "\xef\x81\x83"              // 

// Blueprint/Node specific
#define ICON_FA_DIAGRAM_PROJECT "\xef\x95\xa2"         // 
#define ICON_FA_SITEMAP "\xef\x83\xa8"                 // 
#define ICON_FA_NETWORK_WIRED "\xef\xa0\xbf"           // 
#define ICON_FA_CODE_BRANCH "\xef\x84\xa6"             // 
#define ICON_FA_CODE "\xef\x84\xa1"                    // 
#define ICON_FA_CUBE "\xef\x86\xb2"                    // 
#define ICON_FA_CUBES "\xef\x87\xb1"                   // 

// AI/Behavior Tree specific
#define ICON_FA_BRAIN "\xef\xa2\x99"                   // 
#define ICON_FA_ROBOT "\xef\xa4\xb8"                   // 
#define ICON_FA_MICROCHIP "\xef\xaa\xbb"               // 
#define ICON_FA_CIRCLE_NODES "\xef\x87\x82"            // 

// Debug/Play controls
#define ICON_FA_PLAY "\xef\x81\x8b"                    // 
#define ICON_FA_PAUSE "\xef\x81\x8c"                   // 
#define ICON_FA_STOP "\xef\x81\x8d"                    // 
#define ICON_FA_FORWARD "\xef\x81\x8e"                 // 
#define ICON_FA_BACKWARD "\xef\x81\x8a"                // 
#define ICON_FA_BUG "\xef\x86\x88"                     // 
#define ICON_FA_GAUGE "\xef\x98\xa4"                   // 

// Settings/Tools
#define ICON_FA_GEAR "\xef\x80\x93"                    // 
#define ICON_FA_GEARS "\xef\x82\x85"                   // 
#define ICON_FA_WRENCH "\xef\x82\xad"                  // 
#define ICON_FA_SLIDERS "\xef\x87\xa1"                 // 
#define ICON_FA_SCREWDRIVER_WRENCH "\xef\xa4\xad"      // 
#define ICON_FA_TOOLBOX "\xef\xa4\xb8"                 // 

// Info/Help
#define ICON_FA_CIRCLE_INFO "\xef\x81\x9a"             // 
#define ICON_FA_CIRCLE_QUESTION "\xef\x81\x99"         // 
#define ICON_FA_BOOK "\xef\x80\xb2"                    // 
#define ICON_FA_LIGHTBULB "\xef\x83\xab"               // 

// Status indicators
#define ICON_FA_CHECK "\xef\x80\x8c"                   // 
#define ICON_FA_CIRCLE_CHECK "\xef\x81\x98"            // 
#define ICON_FA_TRIANGLE_EXCLAMATION "\xef\x81\xb1"    // 
#define ICON_FA_CIRCLE_EXCLAMATION "\xef\x81\xaa"      // 
#define ICON_FA_CIRCLE_XMARK "\xef\x81\x97"            // 

// UI elements
#define ICON_FA_BARS "\xef\x83\x89"                    // 
#define ICON_FA_ELLIPSIS "\xef\x85\x81"                // 
#define ICON_FA_GRIP_VERTICAL "\xef\x96\x88"           // 
#define ICON_FA_LIST "\xef\x80\xba"                    // 
#define ICON_FA_TABLE "\xef\x83\x8e"                   // 
#define ICON_FA_WINDOW_MAXIMIZE "\xef\x8b\x90"         // 
#define ICON_FA_WINDOW_MINIMIZE "\xef\x8b\x91"         // 

// Special
#define ICON_FA_STAR "\xef\x80\x85"                    // 
#define ICON_FA_BOOKMARK "\xef\x80\xae"                // 
#define ICON_FA_TAG "\xef\x80\xab"                     // 
#define ICON_FA_TAGS "\xef\x80\xac"                    // 
#define ICON_FA_FILTER "\xef\x83\x80"                  // 
#define ICON_FA_CLOCK "\xef\x80\x97"                   // 
#define ICON_FA_CALENDAR "\xef\x84\xb3"                // 

// Window controls
#define ICON_FA_WINDOW_RESTORE "\xef\x8b\x92"          // 
#define ICON_FA_UP_RIGHT_FROM_SQUARE "\xef\x8d\x85"    // 
#define ICON_FA_SQUARE_XMARK "\xef\x8b\x94"            // 

// Font Awesome 6 Free configuration
#define FONT_ICON_FILE_NAME_FA "fa-solid-900.otf"
#define FONT_ICON_RANGE_FA_MIN 0xf000
#define FONT_ICON_RANGE_FA_MAX 0xf8ff

