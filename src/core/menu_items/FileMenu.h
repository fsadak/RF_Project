#ifndef __FILE_MENU_H__
#define __FILE_MENU_H__
#if defined(HAS_TFT) || defined(HAS_SCREEN)
#include <MenuItemInterface.h>

class FileMenu : public MenuItemInterface {
public:
    FileMenu() : MenuItemInterface("Files") {}

    void optionsMenu(void);
    void drawIcon(float scale);
    void drawIconImg();
    bool getTheme() { return bruceConfig.theme.files; }
};

#endif
#endif
