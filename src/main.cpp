#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/CCKeyboardDispatcher.hpp>

using namespace geode::prelude;

static int  g_saved   = 0;      // 0 = off, else ticks to buffer
static bool g_overlay = true;

struct Pending {
    bool active = false;
    int  age = 0;
    int  button = 1;
    bool p1 = true;
};
static Pending g_pending[2];     // one per player
static bool    g_replaying = false;

static void refreshLabel(PlayLayer* pl) {
    if (!pl) return;
    auto lbl = static_cast<CCLabelBMFont*>(pl->getChildByID("tickbuffer-label"));
    if (!lbl) {
        lbl = CCLabelBMFont::create("", "bigFont.fnt");
        lbl->setID("tickbuffer-label");
        lbl->setScale(0.4f);
        lbl->setAnchorPoint({0, 1});
        lbl->setPosition({6, CCDirector::get()->getWinSize().height - 6});
        lbl->setZOrder(1000);
        pl->addChild(lbl);
    }
    lbl->setVisible(g_overlay);
    lbl->setString(g_saved == 0
        ? "Tick buffer: OFF"
        : fmt::format("Tick buffer: {} tick{}", g_saved, g_saved > 1 ? "s" : "").c_str());
}

class $modify(TBLayer, GJBaseGameLayer) {
    void handleButton(bool down, int button, bool isPlayer1) {
        if (g_replaying || g_saved == 0 || !down) {
            if (!down) g_pending[isPlayer1 ? 0 : 1].active = false;
            return GJBaseGameLayer::handleButton(down, button, isPlayer1);
        }
        auto* p = isPlayer1 ? m_player1 : m_player2;
        if (p && p->m_isOnGround) {
            return GJBaseGameLayer::handleButton(down, button, isPlayer1);
        }
        auto& slot = g_pending[isPlayer1 ? 0 : 1];
        slot = { true, 0, button, isPlayer1 };
    }

    void processCommands(float dt) {
        GJBaseGameLayer::processCommands(dt);
        for (auto& s : g_pending) {
            if (!s.active) continue;
            auto* p = s.p1 ? m_player1 : m_player2;
            if (p && p->m_isOnGround) {
                s.active = false;
                g_replaying = true;
                GJBaseGameLayer::handleButton(true, s.button, s.p1);
                g_replaying = false;
            } else if (++s.age > g_saved) {
                s.active = false;
            }
        }
    }
};

class $modify(TBPlay, PlayLayer) {
    void setupHasCompleted() {
        PlayLayer::setupHasCompleted();
        refreshLabel(this);
    }
    void resetLevel() {
        for (auto& s : g_pending) s.active = false;
        PlayLayer::resetLevel();
    }
};

class $modify(TBKeys, CCKeyboardDispatcher) {
    bool dispatchKeyboardMSG(enumKeyCodes key, bool down, bool repeat) {
        if (down && !repeat) {
            int max = Mod::get()->getSettingValue<int64_t>("max-ticks");
            if (key == KEY_F6) {
                g_saved = (g_saved + 1) > max ? 0 : g_saved + 1;
                refreshLabel(PlayLayer::get());
            } else if (key == KEY_F7) {
                g_overlay = !g_overlay;
                refreshLabel(PlayLayer::get());
            }
        }
        return CCKeyboardDispatcher::dispatchKeyboardMSG(key, down, repeat);
    }
};
