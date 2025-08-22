#ifndef __SCRIPTS_MENU_H__
#define __SCRIPTS_MENU_H__

#if defined(HAS_TFT) || defined(HAS_SCREEN)
#include <MenuItemInterface.h>

class ScriptsMenu : public MenuItemInterface {
public:
    ScriptsMenu() : MenuItemInterface("JS Interpreter") {}

    void optionsMenu(void);
    void drawIcon(float scale);
    void drawIconImg();
    bool getTheme() { return bruceConfig.theme.interpreter; }
};

#endif
#endif
