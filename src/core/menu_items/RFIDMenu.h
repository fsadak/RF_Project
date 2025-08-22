#ifndef __RFID_MENU_H__
#define __RFID_MENU_H__

#if defined(HAS_TFT) || defined(HAS_SCREEN)
#include <MenuItemInterface.h>

class RFIDMenu : public MenuItemInterface {
public:
    RFIDMenu() : MenuItemInterface("RFID") {}

    void optionsMenu(void);
    void drawIcon(float scale);
    void drawIconImg();
    bool getTheme() { return bruceConfig.theme.rfid; }

private:
    void configMenu(void);
};

#endif
#endif
