#ifndef __IR_MENU_H__
#define __IR_MENU_H__

#if defined(HAS_TFT) || defined(HAS_SCREEN)
#include <MenuItemInterface.h>

class IRMenu : public MenuItemInterface {
public:
    IRMenu() : MenuItemInterface("IR") {}

    void optionsMenu(void);
    void drawIcon(float scale);
    void drawIconImg();
    bool getTheme() { return bruceConfig.theme.ir; }

private:
    void configMenu(void);
};

#endif
#endif
