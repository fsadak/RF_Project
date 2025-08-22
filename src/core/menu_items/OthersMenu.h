#ifndef __OTHERS_MENU_H__
#define __OTHERS_MENU_H__

#if defined(HAS_TFT) || defined(HAS_SCREEN)
#include <MenuItemInterface.h>

class OthersMenu : public MenuItemInterface {
public:
    OthersMenu() : MenuItemInterface("Others") {}

    void optionsMenu(void);
    void drawIcon(float scale);
    void drawIconImg();
    bool getTheme() { return bruceConfig.theme.others; }
};

#endif
#endif

