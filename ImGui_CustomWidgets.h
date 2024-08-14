#ifndef __IMGUI_CUSTOMWIDGETS_H__
#define __IMGUI_CUSTOMWIDGETS_H__


#include "imgui_internal.h"

namespace ImGui{
    bool SelectableInput(const char* str_id, bool selected, ImGuiSelectableFlags flags, char* buf, size_t buf_size);
}
#endif // __IMGUI_CUSTOMWIDGETS_H__