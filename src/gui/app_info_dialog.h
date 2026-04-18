#pragma once

namespace gui {
    /**
     * @brief Renders the "About" dialog popup.
     *        Handles positioning, visibility, and content rendering.
     */
    class AppInfoDialog {
        public:
            void render(bool& showAboutPopup);

        private:
            void setPopupPosition();
    };
}
