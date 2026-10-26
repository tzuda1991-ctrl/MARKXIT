#pragma once
#include "include/includes.h"
#include "game.h"
#include "game/Ruleset.h"
#include "imgui/inc/8bp.h"
#include "mod/keylogin.h"
#include "oxorany/oxorany.h"
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <sys/system_properties.h>
#include <ctime>
#include <Vector/Vectors.h>
#include <imgui/imgui.h>
#include "icons/icons.h"
#include "include/java.h"

static void OpenTelegramURL() {
    JNIEnv* env = GetJNIEnv();
    if (!env) return;

    jclass uriClass = env->FindClass("android/net/Uri");
    jmethodID parseMethod = env->GetStaticMethodID(uriClass, "parse", "(Ljava/lang/String;)Landroid/net/Uri;");
    jstring urlString = env->NewStringUTF("https://t.me/+FqiKllIYCr8zOGJl");
    jobject uri = env->CallStaticObjectMethod(uriClass, parseMethod, urlString);
    env->DeleteLocalRef(urlString);

    jclass intentClass = env->FindClass("android/content/Intent");
    jmethodID intentConstructor = env->GetMethodID(intentClass, "<init>", "(Ljava/lang/String;Landroid/net/Uri;)V");
    jstring actionString = env->NewStringUTF("android.intent.action.VIEW");
    jobject intent = env->NewObject(intentClass, intentConstructor, actionString, uri);
    env->DeleteLocalRef(actionString);

    jmethodID addFlagsMethod = env->GetMethodID(intentClass, "addFlags", "(I)Landroid/content/Intent;");
    env->CallObjectMethod(intent, addFlagsMethod, 0x10000000); // FLAG_ACTIVITY_NEW_TASK

    jclass activityThreadClass = env->FindClass("android/app/ActivityThread");
    jmethodID currentActivityThreadMethod = env->GetStaticMethodID(activityThreadClass, "currentActivityThread", "()Landroid/app/ActivityThread;");
    jobject activityThread = env->CallStaticObjectMethod(activityThreadClass, currentActivityThreadMethod);
    if (!activityThread) return;

    jmethodID getApplicationMethod = env->GetMethodID(activityThreadClass, "getApplication", "()Landroid/app/Application;");
    jobject application = env->CallObjectMethod(activityThread, getApplicationMethod);
    if (!application) return;

    jclass contextClass = env->FindClass("android/content/Context");
    jmethodID startActivityMethod = env->GetMethodID(contextClass, "startActivity", "(Landroid/content/Intent;)V");
    env->CallVoidMethod(application, startActivityMethod, intent);

    env->DeleteLocalRef(uri);
    env->DeleteLocalRef(intent);
    env->DeleteLocalRef(application);
    env->DeleteLocalRef(activityThread);
}

using namespace ImGui;
using namespace std;

struct MenuState {
    bool isOpen = false;
    int currentTab = 0;
    float sidebarWidth = 750.0f;
    float animProgress = 0.0f;
    float menuAlpha = 0.0f;
    float menuScale = 0.9f;
    ImVec4 accentColor = ImVec4(0.35f, 0.65f, 0.95f, 1.0f);
};
static MenuState g_menu;

static const int64_t EXPIRY_TS = O(1780099200LL);

static bool DEBUG_BYPASS_LOGIN = false;

static float EaseOutBack(float x) {
    const float c1 = 1.70158f;
    const float c3 = c1 + 1.0f;
    return 1.0f + c3 * powf(x - 1.0f, 3.0f) + c1 * powf(x - 1.0f, 2.0f);
}

static float EaseOutQuart(float x) {
    return 1.0f - powf(1.0f - x, 4.0f);
}

static void DrawGradientRect(ImDrawList* dl, ImVec2 p1, ImVec2 p2, ImU32 col1, ImU32 col2, bool horizontal = true) {
    if (horizontal) {
        dl->AddRectFilledMultiColor(p1, p2, col1, col2, col2, col1);
    } else {
        dl->AddRectFilledMultiColor(p1, p2, col1, col1, col2, col2);
    }
}

static bool SidebarButton(const char* label, GLuint iconTex, bool selected, float width) {
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems) return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);

    float iconSize   = 60.0f;
    float vPad       = 10.0f;
    float btnH       = vPad + iconSize + 4.0f + g.FontSize + vPad;

    ImVec2 pos  = window->DC.CursorPos;
    ImVec2 size = ImVec2(width, btnH);

    const ImRect bb(pos, pos + size);
    ItemSize(size, style.FramePadding.y);
    if (!ItemAdd(bb, id)) return false;

    bool hovered, held;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held);

    ImDrawList* dl = window->DrawList;

    // Icon center position
    float iconBgPad  = 6.0f;
    float iconBgSize = iconSize + iconBgPad * 2.0f;
    ImVec2 iconCenter = ImVec2(
        bb.Min.x + width * 0.5f,
        bb.Min.y + vPad + iconSize * 0.5f
    );

    // Selected: red rounded rect behind icon only
    if (selected) {
        dl->AddRectFilled(
            ImVec2(iconCenter.x - iconBgSize * 0.5f, iconCenter.y - iconBgSize * 0.5f),
            ImVec2(iconCenter.x + iconBgSize * 0.5f, iconCenter.y + iconBgSize * 0.5f),
            IM_COL32(255, 0, 0, 255), 12.0f
        );
    }

    // Draw icon texture centered, with color filter when not selected
    if (iconTex) {
        ImVec2 iconMin = ImVec2(iconCenter.x - iconSize * 0.5f, iconCenter.y - iconSize * 0.5f);
        ImVec2 iconMax = ImVec2(iconCenter.x + iconSize * 0.5f, iconCenter.y + iconSize * 0.5f);
        ImU32 tint = selected ? IM_COL32(255, 255, 255, 255) : IM_COL32(255, 255, 255, 255);
        dl->AddImage((void*)(intptr_t)iconTex, iconMin, iconMax, ImVec2(0,0), ImVec2(1,1));
    }

    // Draw label centered below icon
    ImVec2 labelSize = CalcTextSize(label);
    ImVec2 textPos   = ImVec2(
        bb.Min.x + (width - labelSize.x) * 0.5f,
        bb.Min.y + vPad + iconSize + 4.0f
    );
    ImU32 textCol = selected ? IM_COL32(255, 255, 255, 255) : IM_COL32(140, 140, 150, 255);
    dl->AddText(textPos, textCol, label);

    return pressed;
}

static bool ToggleSwitch(const char* label, bool* v) {
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems) return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);

    float scale = 1.5f; // 1.5f is with 50% bigger than writen values
    float height = 32.0f * scale;
    float width = 56.0f * scale;
    float radius = height * 0.5f;

    ImVec2 textSize = CalcTextSize(label);
    ImVec2 pos = window->DC.CursorPos;
    ImVec2 size = ImVec2(GetContentRegionAvail().x, ImMax(height, textSize.y) + style.FramePadding.y * 2 + 10.0f);

    const ImRect bb(pos, pos + size);
    ItemSize(size, style.FramePadding.y);
    if (!ItemAdd(bb, id)) return false;

    bool hovered, held;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held);
    if (pressed) *v = !*v;

    static std::map<ImGuiID, float> switchAnim;
    float& animT = switchAnim[id];
    float targetT = *v ? 1.0f : 0.0f;
    animT += (targetT - animT) * g.IO.DeltaTime * 14.0f;

    ImDrawList* dl = window->DrawList;
    
    if (hovered) {
        dl->AddRectFilled(bb.Min, bb.Max, IM_COL32(45, 45, 55, 100), 10.0f);
    }
    
    ImVec2 togglePos = ImVec2(bb.Max.x - width - 15.0f, bb.Min.y + (size.y - height) * 0.5f);
    ImVec2 toggleEnd = ImVec2(togglePos.x + width, togglePos.y + height);
    
    ImVec4 offColor = ImVec4(0.27f, 0.27f, 0.31f, 1.0f);
    ImVec4 onColor = ImVec4(1.0f, 0.f, 0.f, 1.0f);
    ImVec4 bgColorV = ImLerp(offColor, onColor, animT);
    dl->AddRectFilled(togglePos, toggleEnd, ImColor(bgColorV), radius);
    
    float knobX = togglePos.x + radius + (width - height) * animT;
    float knobY = togglePos.y + radius;
    float knobR = radius - 4.0f;
    
    dl->AddCircleFilled(ImVec2(knobX, knobY), knobR + 2.0f, IM_COL32(0, 0, 0, 40));
    dl->AddCircleFilled(ImVec2(knobX, knobY), knobR, IM_COL32(255, 255, 255, 255));

    dl->AddText(ImVec2(bb.Min.x + 15.0f, bb.Min.y + (size.y - textSize.y) * 0.5f), IM_COL32(230, 230, 240, 255), label);

    return pressed;
}

// File-scope so DrawToggleButton cancel can also reset countdown
static bool g_aqCounting = false;
static std::chrono::steady_clock::time_point g_aqLastCall;
static std::chrono::steady_clock::time_point g_aqCountdownStart;


static bool IsExpired() {
    return false;
}

INLINE void DrawExpired(ImGuiIO& io) {
    float winW = g_menu.sidebarWidth;

    SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    SetNextWindowSize(ImVec2(winW, 0), ImGuiCond_Always);
    PushStyleColor(ImGuiCol_WindowBg, IM_COL32(21, 21, 21, 255));
    PushStyleVar(ImGuiStyleVar_WindowRounding, 20.0f);
    PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(30.0f, 30.0f));
    PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    if (Begin(O("##ExpiredWin"), nullptr,
              ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
              ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings |
              ImGuiWindowFlags_AlwaysAutoResize)) {

        SetWindowFontScale(1.6f);
        ImVec2 titleSz = CalcTextSize(O("MOD EXPIRED"));
        SetCursorPosX((winW - 60.0f - titleSz.x) * 0.5f);
        TextColored(ImVec4(1.0f, 0.1f, 0.1f, 1.0f), "%s", O("MOD EXPIRED"));
        SetWindowFontScale(1.0f);

        Dummy(ImVec2(0, 16));

        PushTextWrapPos(GetCursorPosX() + winW - 60.0f);
        TextColored(ImVec4(0.85f, 0.85f, 0.90f, 1.0f), "%s",
            O("Beta Version Expired. Update on our Telegram: https://t.me/+FqiKllIYCr8zOGJl"));
        PopTextWrapPos();

        Dummy(ImVec2(0, 10));
    }
    End();
    PopStyleVar(3);
    PopStyleColor();
}

INLINE void DrawAutoQueue() {
    if ((!g_Token.empty() && !g_Auth.empty() && g_Token == g_Auth) || DEBUG_BYPASS_LOGIN) {
        auto now = std::chrono::steady_clock::now();

        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - g_aqLastCall).count() > 500)
            g_aqCounting = false;
        g_aqLastCall = now;

        if (!g_aqCounting) {
            g_aqCounting = true;
            g_aqCountdownStart = now;
        }

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - g_aqCountdownStart).count();
        int remaining_ms = 8000 - (int)elapsed;

      // if (remaining_ms <= 0) {
           // if (sharedMenuManager.getMenuStateId() == 13) PopMenuState(13);
            //StartLastMatch();
           // g_aqCounting = false;
          //  return;
       // }

        std::string count_str = std::to_string((remaining_ms / 1000) + 1);

        // Minimal auto-sized window, transparent bg — we draw our own rounded rect
        SetNextWindowPos(ImVec2(Width * 0.5f, Height * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 1.f));
        PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(32.0f, 20.0f));
        PushStyleVar(ImGuiStyleVar_WindowRounding, 24.0f);

        if (Begin(O("##AutoQueueCD"), nullptr,
                  ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                  ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings |
                  ImGuiWindowFlags_AlwaysAutoResize)) {
            ImDrawList* dl  = GetWindowDrawList();
            ImVec2      wp  = GetWindowPos();
            ImVec2      ws  = GetWindowSize();
            dl->AddRectFilled(wp, ImVec2(wp.x + ws.x, wp.y + ws.y), IM_COL32(20, 20, 28, 0), 24.0f);

            SetWindowFontScale(3.5f);
            TextColored(ImVec4(1.f, 0.f, 0.f, 1.0f), "%s", count_str.c_str());
            SetWindowFontScale(1.0f);
        }
        End();
        PopStyleVar(2);
        PopStyleColor();
    }
}

#include "mod/ButtonClicker.h"

static void DrawToggleButton(bool cancelMode); // forward declaration — defined after DrawFloatingButton

INLINE void DrawESP(ImDrawList* draw) {
    if ((!g_Token.empty() && !g_Auth.empty() && g_Token == g_Auth) || DEBUG_BYPASS_LOGIN) {
        if (!sharedGameManager) return;

        UpdateScreenTable();

        sharedDirector = F(ptr, libmain + O(0x4f06288));
        if (!sharedDirector) return;

        sharedUserInfo = F(ptr, libmain + O(0x4e9feb8));
        if (!sharedUserInfo) return;

        F(bool, sharedUserInfo + 0x340) = true;

        sharedMainManager = F(ptr, libmain + O(0x4dde3e0));
        if (!sharedMainManager) return;

        sharedMenuManager = F(ptr, libmain + O(0x4dfe838));
        if (!sharedMenuManager) return;

        MainStateManager mainStateManager = sharedMainManager.mStateManager;
        if (!mainStateManager) return;
        if (!mainStateManager.isInGame()) {
        if (persistent_bool[O("bAutoQueue")]) {
            if (!sharedMenuManager.isInQueue()) DrawAutoQueue();
            DrawToggleButton(true);  // acts as cancel button for autoqueue
        } return;
        }

        auto visualCue = sharedGameManager.mVisualCue();

        Ball::Classification myclass = sharedGameManager.getPlayerClassification();

        Table table = sharedGameManager.mTable;
        if (!table) return;

        auto tableProperties = table.mTableProperties();
        if (!tableProperties) return;

        auto& pockets = tableProperties.mPockets();

        GameStateManager gameStateManager = sharedGameManager.mStateManager;
        if (!gameStateManager) return;

        if (persistent_bool[O("bAutoPlay")]) {
            DrawToggleButton(false);
        AutoPlay::Update();
        }

        auto stateId = gameStateManager.getCurrentStateId();
        if (stateId == 4) gPrediction->determineShotResult(false);
        if (stateId == 6 || stateId == 7 || stateId == 8) return;

        if (persistent_bool[O("bESP_DrawPocketsShotState")]) {
            for (int i = 0; i < 6; i++) {
                if (Prediction::pocketStatus[i]) {
                    auto screenPos = WorldToScreen(pockets[i]);
                    draw->AddCircle(ImVec2(screenPos.x, screenPos.y), 30, GREEN, 0, 5.f);
                }
            }
        }

        if (persistent_bool[O("bESP_DrawPredictionLine")]) {
            for (int i = 0; i < gPrediction->guiData.ballsCount; i++) {
                auto& ball = gPrediction->guiData.balls[i];

                if (ball.initialPosition != ball.predictedPosition) {
                    ImVec2 lastPos{};
                    float lineThick = (float)persistent_int[O("iLineThickness")];
                    if (lineThick < 1.f) lineThick = 1.f;
                    for (int j = 1; j < ball.positions.size(); j++) {
                        auto point = WorldToScreen(ball.positions[j]);
                        if (lastPos.x || lastPos.y) draw->AddLine(lastPos, point, colors[i], lineThick);
                        lastPos = point;
                    }
                }
            }
        }

        if (persistent_bool[O("bESP_DrawPredictionLine")]) {
            for (int i = 0; i < gPrediction->guiData.ballsCount; i++) {
                auto& ball = gPrediction->guiData.balls[i];

                if (ball.initialPosition != ball.predictedPosition) {
                    float circleR = (float)persistent_int[O("iLineThickness")] + 1.f;
                    if (circleR < 2.f) circleR = 2.f;
                    draw->AddCircleFilled(WorldToScreen(ball.initialPosition), circleR, colors[i]);
                    draw->AddCircleFilled(WorldToScreen(ball.predictedPosition), 16, colors[i]);
                }
            }
        }
    }
}


// ----------------- Premium UI Helpers -----------------
static int currentTheme = 1; // 1 is Red
static ImVec4 accent = ImVec4(0.90f, 0.22f, 0.20f, 1.0f); // Red
static const char* Themes[] = { "Purple", "Red", "Blue", "Green" };

static void ApplyTheme(int themeIndex) {
    switch (themeIndex) {
        case 0: accent = ImVec4(0.56f, 0.11f, 0.95f, 1.0f); break; // Screenshot Purple
        case 1: accent = ImVec4(0.90f, 0.22f, 0.20f, 1.0f); break;
        case 2: accent = ImVec4(0.15f, 0.39f, 0.92f, 1.0f); break;
        case 3: accent = ImVec4(0.13f, 0.77f, 0.37f, 1.0f); break;
        default: accent = ImVec4(0.56f, 0.11f, 0.95f, 1.0f); break;
    }
}

static void PushPremiumStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 12.0f;
    style.ChildRounding = 8.0f;
    style.FrameRounding = 6.0f;
    style.PopupRounding = 6.0f;
    style.ScrollbarRounding = 6.0f;
    style.GrabRounding = 6.0f;
    style.TabRounding = 6.0f;

    style.WindowPadding = ImVec2(16, 16);
    style.FramePadding = ImVec2(12, 10);
    style.ItemSpacing = ImVec2(12, 12);

    ImVec4* colors = style.Colors;
    // Deep dark background for main window
    colors[ImGuiCol_WindowBg]        = ImVec4(0.05f, 0.05f, 0.07f, 0.98f);
    // Sidebar and elements background
    colors[ImGuiCol_ChildBg]         = ImVec4(0.07f, 0.08f, 0.10f, 1.0f);
    colors[ImGuiCol_PopupBg]         = ImVec4(0.07f, 0.08f, 0.10f, 0.98f);
    colors[ImGuiCol_Border]          = ImVec4(1.0f, 1.0f, 1.0f, 0.04f);
    colors[ImGuiCol_Text]            = ImVec4(0.90f, 0.90f, 0.92f, 1.0f);
    colors[ImGuiCol_TextDisabled]    = ImVec4(0.40f, 0.40f, 0.45f, 1.0f);

    colors[ImGuiCol_FrameBg]         = ImVec4(0.12f, 0.12f, 0.15f, 1.0f);
    colors[ImGuiCol_FrameBgHovered]  = ImVec4(accent.x, accent.y, accent.z, 0.30f);
    colors[ImGuiCol_FrameBgActive]   = ImVec4(accent.x, accent.y, accent.z, 0.50f);

    colors[ImGuiCol_Button]          = ImVec4(0.12f, 0.12f, 0.15f, 1.0f); // Default button matches frame
    colors[ImGuiCol_ButtonHovered]   = ImVec4(accent.x, accent.y, accent.z, 0.80f);
    colors[ImGuiCol_ButtonActive]    = ImVec4(accent.x, accent.y, accent.z, 1.00f);

    colors[ImGuiCol_CheckMark]       = accent; // Use accent color for checkmark
    // Overwrite frame active color when checkbox is active, ImGui handles this partly, but we set SliderGrab to accent
    colors[ImGuiCol_SliderGrab]      = accent;
    colors[ImGuiCol_SliderGrabActive]= ImVec4(accent.x + 0.1f, accent.y + 0.1f, accent.z + 0.1f, 1.0f);

    colors[ImGuiCol_Header]          = ImVec4(accent.x, accent.y, accent.z, 0.25f);
    colors[ImGuiCol_HeaderHovered]   = ImVec4(accent.x, accent.y, accent.z, 0.45f);
    colors[ImGuiCol_HeaderActive]    = ImVec4(accent.x, accent.y, accent.z, 0.65f);
}

static void TabHeader(GLuint iconTex, const char* title, const char* desc) {
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    float width = ImGui::GetContentRegionAvail().x;
    float height = 45.0f;
    
    // Rounded dark box background for the header
    dl->AddRectFilled(p, ImVec2(p.x + width, p.y + height), IM_COL32(20, 20, 25, 255), 8.0f);

    ImFont* font = ImGui::GetFont();
    
    // Title centered vertically and aligned left with padding
    dl->AddText(font, ImGui::GetFontSize(), ImVec2(p.x + 15, p.y + 14), ImGui::ColorConvertFloat4ToU32(accent), title);

    ImGui::Dummy(ImVec2(width, height + 5));
}

static bool PremiumSidebarButton(const char* label, GLuint iconTex, bool active) {
    ImVec2 size = ImVec2(150, 80);
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    
    bool clicked = ImGui::InvisibleButton(label, size);
    bool hovered = ImGui::IsItemHovered();

    if (active) {
        // Subtle background for active state
        dl->AddRectFilled(p, ImVec2(p.x + size.x, p.y + size.y), ImGui::ColorConvertFloat4ToU32(ImVec4(0.12f, 0.12f, 0.15f, 1.0f)), 8.0f);
        
        // Active accent line on the right side
        dl->AddRectFilled(ImVec2(p.x + size.x - 4.0f, p.y + 15.0f), ImVec2(p.x + size.x, p.y + size.y - 15.0f), ImGui::ColorConvertFloat4ToU32(accent), 4.0f, ImDrawFlags_RoundCornersLeft);
    } else if (hovered) {
        dl->AddRectFilled(p, ImVec2(p.x + size.x, p.y + size.y), IM_COL32(40, 40, 45, 150), 8.0f);
    }

    // Pure red icons as requested
    ImVec4 iconColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); 
    ImVec4 textColor = active ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f) : ImVec4(0.7f, 0.7f, 0.75f, 1.0f);

    ImFont* font = ImGui::GetFont();
    ImVec2 labelSize = ImGui::CalcTextSize(label);

    if (iconTex) {
        float iconW = 32.0f; // High quality slightly larger size
        dl->AddImage((void*)(intptr_t)iconTex, ImVec2(p.x + (size.x - iconW) * 0.5f, p.y + 10), ImVec2(p.x + (size.x + iconW) * 0.5f, p.y + 10 + iconW), ImVec2(0,0), ImVec2(1,1), ImGui::ColorConvertFloat4ToU32(iconColor));
    }

    // Proper native unscaled text
    dl->AddText(font, ImGui::GetFontSize(), ImVec2(p.x + (size.x - labelSize.x) * 0.5f, p.y + 50), ImGui::ColorConvertFloat4ToU32(textColor), label);

    return clicked;
}
// ------------------------------------------------------
// Reads an IL2CPP/Unity NSString (UTF-16 internal buffer at offset 0x14, length at 0x10)
static std::string ReadNSString(ptr str) {
    if (!str) return "null";
    int32_t len = F(int32_t, str + 0x10);
    if (len <= 0 || len > 512) return "?";
    std::string result;
    result.reserve(len);
    for (int32_t i = 0; i < len; i++) {
        uint16_t ch = F(uint16_t, str + 0x14 + i * 2);
        result += (ch > 0 && ch < 128) ? (char)ch : '?';
    }
    return result;
}

// Shared vertical position for DrawToggleButton and DrawFloatingButton (they move together)
static float g_sideBtnsY      = 0.0f;
// Kept for linker compatibility — no longer used for animation
static float g_toggleRotAngle = 0.0f;
// Set true by AutoPlay when in SLOW scan state — shows CALCULATING overlay
static bool  g_autoPlayCalculating = false;

// ── svConfig ──────────────────────────────────────────────────────────────────
static void svConfig_Save() {
    std::string path = O("/data/user/0/") + PACKAGE_NAME + O("/files/svConfig.txt");
    FILE* f = fopen(path.c_str(), O("w"));
    if (!f) return;
    fprintf(f, O("iLineThickness=%d\n"),  persistent_int[O("iLineThickness")]);
    fprintf(f, O("iMenuSizeOffset=%d\n"), persistent_int[O("iMenuSizeOffset")]);
    fclose(f);
}
static void svConfig_Load() {
    std::string path = O("/data/user/0/") + PACKAGE_NAME + O("/files/svConfig.txt");
    FILE* f = fopen(path.c_str(), O("r"));
    if (!f) return;
    char line[64];
    while (fgets(line, sizeof(line), f)) {
        int v = 0;
        if (sscanf(line, O("iLineThickness=%d"),  &v) == 1) { persistent_int[O("iLineThickness")]  = v; continue; }
        if (sscanf(line, O("iMenuSizeOffset=%d"), &v) == 1) { persistent_int[O("iMenuSizeOffset")] = v; }
    }
    fclose(f);
}

// ── CALCULATING overlay (shown during AutoPlay SLOW scan) ─────────────────────
static void DrawCalculating(ImGuiIO& io) {
    // Setăm poziția pe centrul ecranului (Width*0.5, Height*0.5)
    // Pivotul (0.5f, 0.5f) înseamnă că mijlocul ferestrei va fi fix pe coordonatele date
    SetNextWindowPos(ImVec2(Width * 0.5f, Height * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    
    // Auto-resize face ca fereastra să aibă dimensiunea textului automat
    PushStyleColor(ImGuiCol_WindowBg, IM_COL32(21, 21, 21, 255));
    PushStyleColor(ImGuiCol_Border, IM_COL32(220, 30, 30, 255));
    PushStyleVar(ImGuiStyleVar_WindowRounding, 18.0f);
    PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.0f);

    if (Begin(O("##CalcOverlay"), nullptr,
              ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
              ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | 
              ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs)) {
        
        SetWindowFontScale(1.4f);
        TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), O("CALCULATING..."));
        SetWindowFontScale(1.0f);
    }
    End();
    PopStyleVar(2);
    PopStyleColor(2);
}



INLINE void DrawMenu(ImGuiIO& io) {
    if ((!g_Token.empty() && !g_Auth.empty() && g_Token == g_Auth) || DEBUG_BYPASS_LOGIN) {
        if (is_segv_handler_active()) {
            jump_buffer_active = 1;
            if (!sigsetjmp(jump_buffer, 1)) DrawESP(GetBackgroundDrawList());
            jump_buffer_active = 0;
        }

        if (!g_menu.isOpen) return;

        PushPremiumStyle();

        static float uiWidth = 950.0f;
        static float uiHeight = 600.0f;
        static float uiRoundness = 16.0f;
        static float uiGlass = 0.98f;

        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding = uiRoundness;
        style.ChildRounding = uiRoundness - 2.0f;
        style.FrameRounding = uiRoundness - 4.0f;

        ImVec4 bgCol = style.Colors[ImGuiCol_WindowBg];
        style.Colors[ImGuiCol_WindowBg] = ImVec4(bgCol.x, bgCol.y, bgCol.z, uiGlass);

        ImGui::SetNextWindowSize(ImVec2(uiWidth, uiHeight), ImGuiCond_Always);
        ImGui::SetNextWindowPos(ImVec2(Width / 2.0f, Height / 2.0f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

        ImGui::Begin(O("Gtf 8ball - Premium Control Panel"), &g_menu.isOpen, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);

        ImVec2 cursorPos = ImGui::GetCursorPos();
        ImVec2 screenPos = ImGui::GetCursorScreenPos();
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        static GLuint logo_tex = LoadTextureFromMemory(logo_jpg, logo_jpg_len);
        static GLuint draw_tex = LoadTextureFromMemory(draw_icon_png, draw_icon_png_len);
        static GLuint play_tex = LoadTextureFromMemory(play_icon_png, play_icon_png_len);
        static GLuint q_tex    = LoadTextureFromMemory(q_icon_png, q_icon_png_len);
        static GLuint user_tex = LoadTextureFromMemory(user_icon_png, user_icon_png_len);

        float logoBoxSize = 75.0f;
        draw_list->AddImage((void*)(intptr_t)logo_tex, screenPos, ImVec2(screenPos.x + logoBoxSize, screenPos.y + logoBoxSize), ImVec2(0, 0), ImVec2(1, 1), IM_COL32_WHITE);

        ImGui::SetCursorPos(ImVec2(cursorPos.x + logoBoxSize + 15.0f, cursorPos.y + 25.0f));
        ImGui::SetWindowFontScale(1.1f);
        
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 50, 50, 255));
        ImGui::Text(O("GTF 8BALL | "));
        ImGui::PopStyleColor();
        
        ImGui::SameLine(0, 0);
        
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 255));
        ImGui::Text(O("PREMIUM CONTROL PANEL"));
        ImGui::PopStyleColor();
        
        ImGui::SetWindowFontScale(1.0f);

        // Close button top right
        ImGui::SetCursorPos(ImVec2(uiWidth - 75, cursorPos.y + 5.0f));
        ImGui::SetWindowFontScale(1.5f);
        if (ImGui::Button("X", ImVec2(60, 60))) g_menu.isOpen = false;
        ImGui::SetWindowFontScale(1.0f);

        ImGui::SetCursorPos(ImVec2(cursorPos.x, cursorPos.y + logoBoxSize + 15.0f));
        ImGui::Separator();

        ImGui::BeginChild(O("Sidebar"), ImVec2(170, 0), true);
        if (PremiumSidebarButton(O("Draw"), draw_tex, g_menu.currentTab == 0)) g_menu.currentTab = 0;
        ImGui::Dummy(ImVec2(0, 8.0f));
        if (PremiumSidebarButton(O("Auto Play"), play_tex, g_menu.currentTab == 1)) g_menu.currentTab = 1;
        ImGui::Dummy(ImVec2(0, 8.0f));
        if (PremiumSidebarButton(O("Queue"), q_tex, g_menu.currentTab == 2)) g_menu.currentTab = 2;
        ImGui::Dummy(ImVec2(0, 8.0f));
        if (PremiumSidebarButton(O("System"), user_tex, g_menu.currentTab == 3)) g_menu.currentTab = 3;
        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild(O("Content"), ImVec2(ImGui::GetContentRegionAvail().x - 10, 0), true);
        
        bool need_save = false;

        switch (g_menu.currentTab) {
            case 0: {
                TabHeader(draw_tex, O("VISUALS"), O("Visual improvements."));
                ImGui::Checkbox(O("Draw Lines"), &persistent_bool[O("bESP_DrawPredictionLine")]);
                if (ImGui::IsItemDeactivatedAfterEdit()) need_save = true;
                ImGui::Checkbox(O("Draw Prediction Pos"), &persistent_bool[O("bESP_DrawPredictionPos")]);
                if (ImGui::IsItemDeactivatedAfterEdit()) need_save = true;
                ImGui::Checkbox(O("Draw Initial Pos"), &persistent_bool[O("bESP_DrawInitialPos")]);
                if (ImGui::IsItemDeactivatedAfterEdit()) need_save = true;
                ImGui::Checkbox(O("Draw Pockets"), &persistent_bool[O("bESP_DrawPockets")]);
                if (ImGui::IsItemDeactivatedAfterEdit()) need_save = true;
                ImGui::Checkbox(O("Draw Pocket State"), &persistent_bool[O("bESP_DrawPocketsShotState")]);
                if (ImGui::IsItemDeactivatedAfterEdit()) need_save = true;
                
                ImGui::Separator();
                ImGui::TextDisabled(O("LINE THICKNESS"));
                if (persistent_int[O("iLineThickness")] < 1) persistent_int[O("iLineThickness")] = 4;
                ImGui::SliderInt(O("##lineThick"), &persistent_int[O("iLineThickness")], 1, 10, "%d");
                if (ImGui::IsItemDeactivatedAfterEdit()) need_save = true;
                break;
            }
            case 1: {
                TabHeader(play_tex, O("AUTO PLAY"), O("Automatically play the game."));
                ImGui::Checkbox(O("Enable AutoPlay"), &persistent_bool[O("bAutoPlay")]);
                if (ImGui::IsItemDeactivatedAfterEdit()) need_save = true;
                ImGui::Separator();
                ImGui::TextDisabled(O("Auto Play Will AutoMatically"));
                ImGui::TextDisabled(O("Aim And Shoot For You"));
                break;
            }
            case 2: {
                TabHeader(q_tex, O("QUEUE"), O("Queue enhancements."));
                ImGui::Checkbox(O("Enable AutoQueue"), &persistent_bool[O("bAutoQueue")]);
                if (ImGui::IsItemDeactivatedAfterEdit()) need_save = true;
                ImGui::Separator();
                
                ImGui::TextDisabled(O("Mode"));
                ImGui::Combo(O("##mode"), &persistent_int["iAutoQueue_Mode"], "Last Selected\0Smart\0Fix Table\0\0");
                if (ImGui::IsItemDeactivatedAfterEdit()) need_save = true;

                if (persistent_int["iAutoQueue_Mode"] == 1) {
                    ImGui::Separator();
                    ImGui::TextDisabled(O("Bet Percent"));
                    ImGui::SliderInt(O("##betpercent"), &persistent_int["iAutoQueue_BetPercent"], 1, 100, "%d%%");
                    if (ImGui::IsItemDeactivatedAfterEdit()) need_save = true;
                }

                if (persistent_int["iAutoQueue_Mode"] == 2) {
                    ImGui::Separator();
                    ImGui::TextDisabled(O("Select Table"));
                    struct TableEntry { const char* label; };
                    static const TableEntry tables[17] = {
                        {"100"}, {"200"}, {"1k"}, {"2.5k"}, {"10k"}, {"50k"}, {"100k"}, {"500k"},
                        {"1M"}, {"2M"}, {"5M"}, {"8M"}, {"10M"}, {"20M"}, {"30M"}, {"50M"}, {"200M"}
                    };
                    int& selected = persistent_int["iAutoQueue_FixTable"];
                    int cols = 4;
                    for (int i = 0; i < 17; i++) {
                        if (i % cols != 0) ImGui::SameLine();
                        if (selected == i) {
                            ImGui::PushStyleColor(ImGuiCol_Button, accent);
                            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, accent);
                            ImGui::PushStyleColor(ImGuiCol_ButtonActive, accent);
                        }
                        char btnId[32];
                        snprintf(btnId, sizeof(btnId), "%s##ft%d", tables[i].label, i);
                        if (ImGui::Button(btnId, ImVec2(60, 35))) {
                            selected = i;
                            need_save = true;
                        }
                        if (selected == i) ImGui::PopStyleColor(3);
                    }
                }
                break;
            }
            case 3: {
                TabHeader(user_tex, O("SYSTEM"), O("Configure options."));
                static char s_manufacturer[PROP_VALUE_MAX] = {};
                static char s_model[PROP_VALUE_MAX] = {};
                static bool s_props_loaded = false;
                
                if (!s_props_loaded) {
                    __system_property_get("ro.product.manufacturer", s_manufacturer);
                    __system_property_get("ro.product.model", s_model);
                    s_props_loaded = true;
                }
                ImGui::TextDisabled(O("ManuFacturer: %s"), s_manufacturer);
                ImGui::TextDisabled(O("Model: %s"), s_model);
                ImGui::TextDisabled(O("Key: %s"), persistent_string["key"].c_str());

                ImGui::Separator();
                ImGui::Separator();
                ImGui::Spacing();
                ImGui::TextColored(accent, "--- CUSTOMIZATION ---");
                ImGui::Spacing();
                int oldTheme = currentTheme;
                ImGui::Combo(O("Layout Color Theme"), &currentTheme, Themes, IM_ARRAYSIZE(Themes));
                if (oldTheme != currentTheme) ApplyTheme(currentTheme);
                ImGui::ColorEdit4(O("Accent color"), (float*)&accent, ImGuiColorEditFlags_NoInputs);
                
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                
                float safeWidth = ImGui::GetContentRegionAvail().x - 15.0f;
                
                ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(30, 144, 255, 255));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(50, 164, 255, 255));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(10, 124, 235, 255));
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 255));
                if (ImGui::Button(O("Join Telegram Update Group"), ImVec2(safeWidth, 52))) {
                    OpenTelegramURL();
                }
                ImGui::PopStyleColor(4);
                
                ImGui::Spacing();
                
                ImGui::PushStyleColor(ImGuiCol_Button, accent);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(accent.x + 0.1f, accent.y + 0.1f, accent.z + 0.1f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(accent.x - 0.1f, accent.y - 0.1f, accent.z - 0.1f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 255));
                if (ImGui::Button(O("Save Config"), ImVec2(safeWidth, 52))) {
                    svConfig_Save();
                }
                ImGui::PopStyleColor(4);
                
                ImGui::Dummy(ImVec2(0, 70.0f));
                
                break;
            }
        }
        
        if (need_save) save_persistence();
        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild(O("StatusPanel"), ImVec2(0, 0), true);
        ImGui::PushStyleColor(ImGuiCol_Text, accent);
        ImGui::Text(O("STATUS INFO"));
        ImGui::PopStyleColor();
        ImGui::Separator();
        
        const char* tabNames[] = { O("Draw"), O("Auto Play"), O("Queue"), O("User") };
        ImGui::TextDisabled(O("Tab:")); ImGui::SameLine(); ImGui::Text("%s", tabNames[g_menu.currentTab]);
        
        ImGui::TextDisabled(O("Theme:")); ImGui::SameLine(); ImGui::Text("%s", Themes[currentTheme]);
        
        ImGui::EndChild();

        ImGui::End();
    }
}

// ــــــــــــــــــــــــــــــــــــــــــــــــــــــــــــــــــــــــــــــــ //

static void DrawToggleButton(bool cancelMode) {
    if (g_menu.isOpen) return;

    ImGuiIO& io = GetIO();

    float button_size = 110.f;
    float windowWidth = button_size + GetStyle().WindowPadding.x * 2.0f;
    float windowHeight = button_size + GetStyle().WindowPadding.y * 2.0f;
    const float rightMargin = 20.0f;

    float fixedX = io.DisplaySize.x - rightMargin - windowWidth;

    SetNextWindowSize(ImVec2(windowWidth, windowHeight), ImGuiCond_Always);
    SetNextWindowPos(ImVec2(fixedX, g_sideBtnsY), ImGuiCond_Always);

    PushStyleColor(ImGuiCol_WindowBg, IM_COL32(0, 0, 0, 0));
    PushStyleColor(ImGuiCol_Border,   IM_COL32(0, 0, 0, 0));
    PushStyleVar(ImGuiStyleVar_WindowRounding, 99.0f);

    if (Begin(O("##ToggleBtn"), nullptr,
              ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
              ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove)) {

        ImVec2 pos = GetCursorScreenPos();
        ImVec2 size(button_size, button_size);
        ImVec2 center(pos.x + size.x * 0.5f, pos.y + size.y * 0.5f);

        // جعل الزر يتفاعل مع الضغط فقط وليس السحب
        if (InvisibleButton(O("##TglBtnHit"), size)) {
             if (cancelMode) {
                persistent_bool[O("bAutoQueue")] = false;
                g_aqCounting = false;
            } else {
                AutoPlay::bAutoPlaying = !AutoPlay::bAutoPlaying;
                if (AutoPlay::bAutoPlaying) AutoPlay::ClearState();
            }
        }

        const char* text = nullptr;
        ImU32 bgColor;

        if (cancelMode) {
            text = "CANCEL";
            bgColor = IM_COL32(220, 40, 40, 230); // RED
        } else if (AutoPlay::bAutoPlaying) {
            bgColor = IM_COL32(40, 200, 40, 230); // GREEN (Active)
        } else {
            bgColor = IM_COL32(220, 40, 40, 230); // RED (Inactive)
        }

        ImDrawList* dl = GetWindowDrawList();
        dl->AddCircleFilled(center, size.x * 0.45f, bgColor);
        dl->AddCircle(center, size.x * 0.45f, IM_COL32(255, 255, 255, 100), 0, 3.0f);

        if (cancelMode) {
            ImGui::SetWindowFontScale(1.2f);
            ImVec2 tSize = ImGui::CalcTextSize(text);
            ImVec2 textPos = ImVec2(center.x - tSize.x * 0.5f, center.y - tSize.y * 0.5f);
            dl->AddText(textPos, IM_COL32_WHITE, text);
            ImGui::SetWindowFontScale(1.0f);
        } else {
            float r = size.x * 0.15f;
            if (!AutoPlay::bAutoPlaying) {
                // Play Icon (Triangle pointing right)
                ImVec2 p1 = ImVec2(center.x - r * 0.6f, center.y - r);
                ImVec2 p2 = ImVec2(center.x - r * 0.6f, center.y + r);
                ImVec2 p3 = ImVec2(center.x + r * 1.1f, center.y);
                dl->AddTriangleFilled(p1, p2, p3, IM_COL32_WHITE);
            } else {
                // Pause Icon (Two vertical bars)
                float bar_w = r * 0.6f;
                dl->AddRectFilled(ImVec2(center.x - r, center.y - r), ImVec2(center.x - r + bar_w, center.y + r), IM_COL32_WHITE, 2.0f);
                dl->AddRectFilled(ImVec2(center.x + r - bar_w, center.y - r), ImVec2(center.x + r, center.y + r), IM_COL32_WHITE, 2.0f);
            }
        }
    }
    End();
    PopStyleVar();
    PopStyleColor(2);
}

// ــــــــــــــــــــــــــــــــــــــــــــــــــــــــــــــــــــــــــــــــ //


static void DrawFloatingButton(ImGuiIO& io) {
    if (g_menu.isOpen) return;

    float buttonSize = 150.0f;
    const float rightMargin = 20.0f;
    if (g_sideBtnsY <= 0.0f) g_sideBtnsY = io.DisplaySize.y - 150.0f;

    float fixedX = io.DisplaySize.x - rightMargin - buttonSize;
    float posY = g_sideBtnsY - 140.0f;

    SetNextWindowPos(ImVec2(fixedX, posY), ImGuiCond_Always);
    SetNextWindowSize(ImVec2(buttonSize, buttonSize), ImGuiCond_Always);
    
    PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
    PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    if (Begin(O("MX_FLOAT_BUTTON"), nullptr,
              ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar |
              ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground)) {

        ImDrawList* dl = GetWindowDrawList();
        ImVec2 p = GetWindowPos();
        
        static GLuint logo_tex = LoadTextureFromMemory(logo_jpg, logo_jpg_len);
        dl->AddImage((void*)(intptr_t)logo_tex, p, ImVec2(p.x + buttonSize, p.y + buttonSize), ImVec2(0, 0), ImVec2(1, 1), IM_COL32_WHITE);

        if (InvisibleButton(O("##FloatBtnHit"), ImVec2(buttonSize, buttonSize))) {
            // Click handler handled in drag logic to avoid conflicts
        }

        if (IsItemActive() && IsMouseDragging(ImGuiMouseButton_Left)) {
            g_sideBtnsY += io.MouseDelta.y;
            g_sideBtnsY = ImClamp(g_sideBtnsY, 160.0f, io.DisplaySize.y - 150.0f);
        }
        if (IsItemHovered() && IsMouseReleased(0) && ImGui::GetMouseDragDelta(0).y == 0) {
            g_menu.isOpen = true;
        }
    }
    End();
    PopStyleVar(2);
    PopStyleColor();
}

// ــــــــــــــــــــــــــــــــــــــــــــــــــــــــــــــــــــــــــــــــ //

static bool first_time = true;
INLINE void DrawLogin(ImGuiIO& io) {
    if (logged_in) return DrawMenu(io);

    SetNextWindowPos(ImVec2(0, 0));
    SetNextWindowSize(io.DisplaySize);
    PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.04f, 0.04f, 0.06f, 0.96f));
    Begin(O("##Overlay"), nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoBringToFrontOnFocus);
    PopStyleColor();

    float cardW = 680;
    float cardH = 480;

    SetNextWindowSize(ImVec2(cardW, cardH), ImGuiCond_Always);
    SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

    PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.11f, 0.11f, 0.14f, 1.0f));
    PushStyleVar(ImGuiStyleVar_WindowRounding, 20.0f);
    PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    Begin(O("##LoginCard"), nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar);

    ImDrawList* dl = GetWindowDrawList();
    ImVec2 winPos = GetWindowPos();
    
    DrawGradientRect(dl, winPos, ImVec2(winPos.x + cardW, winPos.y + 110), IM_COL32(170, 35, 35, 255), IM_COL32(200, 55, 55, 255), true);
    dl->AddRectFilled(winPos, ImVec2(winPos.x + cardW, winPos.y + 20), IM_COL32(170, 35, 35, 255), 20.0f, ImDrawFlags_RoundCornersTop);

    SetWindowFontScale(1.4f);
    ImVec2 titleSize = CalcTextSize(O("Gtf 8ball"));
    dl->AddText(ImVec2(winPos.x + (cardW - titleSize.x) * 0.5f, winPos.y + 30), IM_COL32(255, 255, 255, 255), O("Gtf 8ball"));
    SetWindowFontScale(1.0f);
    
    ImVec2 subSize = CalcTextSize(O("Premium Mod"));
    dl->AddText(ImVec2(winPos.x + (cardW - subSize.x) * 0.5f, winPos.y + 70), IM_COL32(200, 220, 255, 200), O("Premium Mod"));

    SetCursorPosY(130);

    if (!ERROR_MESSAGE.empty()) {
        SetCursorPosX(30);
        PushTextWrapPos(cardW - 30);
        TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", ERROR_MESSAGE.c_str());
        PopTextWrapPos();
        Dummy(ImVec2(0, 15));
    }

    if (is_logging_in) {
        SetCursorPosY(180);
        
        static float spinner_angle = 0.0f;
        spinner_angle += io.DeltaTime * 5.0f;

        float spinner_size = 40.0f;
        ImVec2 spinnerCenter = ImVec2(winPos.x + cardW * 0.5f, winPos.y + 220);

        for (int i = 0; i < 12; i++) {
            float angle = spinner_angle + (i * PI * 2.0f / 12.0f);
            float alpha = (float)(12 - i) / 12.0f;
            ImVec2 dotPos = ImVec2(
                spinnerCenter.x + cosf(angle) * spinner_size,
                spinnerCenter.y + sinf(angle) * spinner_size
            );
            dl->AddCircleFilled(dotPos, 6.0f, IM_COL32(100, 180, 255, (int)(alpha * 255)));
        }

        ImVec2 loadingSize = CalcTextSize(O("Authenticating..."));
        SetCursorPosX((cardW - loadingSize.x) * 0.5f);
        SetCursorPosY(290);
        TextColored(ImVec4(0.6f, 0.6f, 0.65f, 1.0f), O("Authenticating..."));
    } else {
        SetCursorPosY(160);
        
        ImVec2 infoSize = CalcTextSize(O("Copy your license key and tap login"));
        SetCursorPosX((cardW - infoSize.x) * 0.5f);
        TextColored(ImVec4(0.55f, 0.55f, 0.6f, 1.0f), O("Copy your license key and tap login"));
        
        Dummy(ImVec2(0, 50));
        
        bool AutoLogin = first_time && !persistent_string["key"].empty();
        
        SetCursorPosX(40);
        PushStyleColor(ImGuiCol_Button, ImVec4(0.80f, 0.20f, 0.20f, 1.0f));
        PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.90f, 0.25f, 0.25f, 1.0f));
        PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.72f, 0.18f, 0.18f, 1.0f));
        PushStyleVar(ImGuiStyleVar_FrameRounding, 14.0f);
        
     if (AutoLogin || Button("LGNBTN", ImVec2(cardW - 80, 65))) {
    if (DEBUG_BYPASS_LOGIN) {
        // Debug bypass: open menu immediately
        logged_in = true;
        g_menu.isOpen = true;
    } else {
        JNIEnv* env;
        jint getEnvResult = VM->GetEnv((void**)&env, JNI_VERSION_1_6);
        if (getEnvResult == JNI_EDETACHED) {
            if (VM->AttachCurrentThread(&env, nullptr) != 0) ERROR_MESSAGE = O("Failed to attach thread to JVM");
        } else if (getEnvResult != JNI_OK) {
            ERROR_MESSAGE = O("Failed to get JNIEnv");
        } else {
            std::thread([](std::string androidId, std::string key) {
                Login(androidId, key);
            }, getAndroidID(env), AutoLogin ? persistent_string["key"] : getClipboard(env)).detach();
        }
        first_time = false;
    }
}
        
        PopStyleVar();
        PopStyleColor(3);
        
        Dummy(ImVec2(0, 35));
        
        ImVec2 helpSize = CalcTextSize(O("Your will read"));
        SetCursorPosX((cardW - helpSize.x) * 0.5f);
        TextColored(ImVec4(0.42f, 0.42f, 0.48f, 1.0f), O("Your will read"));
    }

    End();
    PopStyleVar(3);
    PopStyleColor();
    
    End();
}


INLINE void SetupImgui() {
    PACKAGE_NAME = string(getcmdline());

    ImGui::CreateContext();

    auto& style = ImGui::GetStyle();
    auto& io = ImGui::GetIO();

    io.ConfigFlags |= ImGuiConfigFlags_IsTouchScreen;

    switch_theme(current_theme);

    load_persistence();
    svConfig_Load();
    load_imgui_style();

    static string INI_PATH = O("/data/user_de/0/") + PACKAGE_NAME + O("/no_backup/.ini");
    io.IniFilename = persistent_bool["bImguiAutoSave"] ? INI_PATH.c_str() : nullptr;
    io.ConfigWindowsMoveFromTitleBarOnly = persistent_bool["bMoveOnlyWithTitleBar"];

    ImFontConfig font_cfg;
    font_cfg.SizePixels = persistent_float["fFontScale"];
    io.Fonts->AddFontDefault(&font_cfg);

    ImGui_ImplAndroid_Init();
    ImGui_ImplOpenGL3_Init(O("#version 300 es"));

    bImguiSetup = true;
}

DEFINES(EGLBoolean, Draw, EGLDisplay dpy, EGLSurface surface) {
    eglQuerySurface(dpy, surface, EGL_WIDTH, &Width);
    eglQuerySurface(dpy, surface, EGL_HEIGHT, &Height);

    if (Width <= 0 || Height <= 0) return _Draw(dpy, surface);

    screenCenter = Vector2(Width / 2, Height / 2);

    if (!bImguiSetup) SetupImgui();

    ImGuiIO& io = ImGui::GetIO();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplAndroid_NewFrame(Width, Height);
    ImGui::NewFrame();

    if (!is_segv_handler_active()) setup_global_segv_handler();
    if (IsExpired()) {
        DrawExpired(io);
    } else if ((!g_Token.empty() && !g_Auth.empty() && g_Token == g_Auth) || DEBUG_BYPASS_LOGIN) {
        DrawFloatingButton(io);
        DrawMenu(io);



        if (g_autoPlayCalculating) DrawCalculating(io);
    } else {
        DrawLogin(io);
    }
    ImGui::EndFrame();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    ImGui_ClearHoverEffect();

    return _Draw(dpy, surface);
}

void __IMGUI__() {
    create_directory_recursive(CONC(O("/data/user_de/0/"), PACKAGE_NAME.c_str(), O("/no_backup")));
}
