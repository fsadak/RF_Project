#ifndef __NRF24_MENU_H__
#define __NRF24_MENU_H__

#if defined(HAS_TFT) || defined(HAS_SCREEN)
#include <MenuItemInterface.h>

class NRF24Menu : public MenuItemInterface {
public:
    NRF24Menu() : MenuItemInterface("NRF24") {}

    void optionsMenu(void);
    void configMenu(void);
    void drawIcon(float scale);
    void drawIconImg();
    bool getTheme() { return bruceConfig.theme.nrf; }
};

#endif
#endif
