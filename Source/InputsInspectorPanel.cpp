//#ifdef _WIN32
//#include "PanelManager.h"
//#include "system/JoystickManager.h"
//#include <windows.h>
//#include <string>
//#include "InputsManager.h"
//
//// USELESS // Helper to build list text of connected joysticks
////static std::string BuildJoystickListText()
////{
////    auto ids = JoystickManager::Get().GetConnectedJoysticks();
////    std::string s;
////    s += "Connected Joysticks:\r\n";
////    for (auto id : ids)
////    {
////        bool connected = JoystickManager::Get().IsJoystickConnected(id);
////        s += "ID:" + std::to_string(id) + (connected ? " (connected)" : " (disconnected)") + "\r\n";
////    }
////    return s;
////}
//
////// Called by PanelManager to refresh the content of inputs inspector
////void UpdateInputsInspectorUI(HWND hwndEdit)
////{
////    if (!hwndEdit) return;
////    //std::string text = BuildJoystickListText();
////	std::string text = InputsManager::Get().GetDevicesStatusUpdate();
////    SetWindowTextA(hwndEdit, text.c_str());
////}
//#endif
