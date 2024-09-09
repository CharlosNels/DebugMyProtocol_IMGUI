#include "ImGui_CustomWidgets.h"
namespace ImGui{
    bool SelectableInput(const char* str_id, bool selected, ImGuiSelectableFlags flags, char* buf, size_t buf_size)
    {
        using namespace ImGui;
        ImGuiContext& g = *GImGui;
        ImGuiWindow* window = g.CurrentWindow;
        ImVec2 pos = window->DC.CursorPos;

        PushID(str_id);
        PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(g.Style.ItemSpacing.x, g.Style.FramePadding.y * 2.0f));
        bool is_selected = Selectable("##Selectable", selected, flags | ImGuiSelectableFlags_AllowDoubleClick | ImGuiSelectableFlags_AllowOverlap);
        PopStyleVar();

        ImGuiID id = window->GetID("##Input");
        bool ret = false;
        bool temp_input_is_active = TempInputIsActive(id);
        bool temp_input_start = is_selected ? IsMouseDoubleClicked(0) : false;
        if (temp_input_is_active || temp_input_start)
        {
            ret = TempInputText(g.LastItemData.Rect, id, "##Input", buf, (int)buf_size, ImGuiInputTextFlags_None);
            KeepAliveID(id);
        }
        else
        {
            window->DrawList->AddText(pos, GetColorU32(ImGuiCol_Text), buf);
        }

        PopID();
        return ret;
    }
}
