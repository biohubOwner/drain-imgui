#include "../settings/functions.h"

void c_notify::add(const std::string& title, const std::string& message, float time, notify_pos_t pos)
{
    notifications.push_back({ title, message, time, time, pos });
}

void c_notify::render()
{
    if (notifications.empty())
        return;

    ImDrawList* draw_list = ImGui::GetForegroundDrawList();
    ImVec2 screen_size = ImGui::GetIO().DisplaySize;
    float padding = 10.0f;
    float spacing = 10.0f;

    // Separate notifications by position
    std::vector<notification_t*> tl, tr, bl, br;
    for (auto& n : notifications)
    {
        if (n.pos == notify_pos_t::top_left) tl.push_back(&n);
        else if (n.pos == notify_pos_t::top_right) tr.push_back(&n);
        else if (n.pos == notify_pos_t::bottom_left) bl.push_back(&n);
        else if (n.pos == notify_pos_t::bottom_right) br.push_back(&n);
    }

    auto render_group = [&](std::vector<notification_t*>& group, notify_pos_t pos) {
        float y_offset = padding;
        if (pos == notify_pos_t::bottom_left || pos == notify_pos_t::bottom_right)
            y_offset = screen_size.y - padding;

        for (auto it = group.begin(); it != group.end(); ++it)
        {
            notification_t& n = **it;
            
            ImGui::PushFont(var->font.tahoma);
            ImVec2 title_size = ImGui::CalcTextSize(n.title.c_str());
            ImVec2 message_size = ImGui::CalcTextSize(n.message.c_str());
            ImGui::PopFont();

            // Alpha and Slide animation
            float alpha = 1.0f;
            float slide = 1.0f;
            float anim_time = 0.3f;

            if (n.max_time - n.time < anim_time) 
            {
                alpha = (n.max_time - n.time) / anim_time;
                slide = alpha;
            }
            else if (n.time < anim_time)
            {
                alpha = n.time / anim_time;
                slide = alpha;
            }

            // Ease out cubic for slide
            slide = 1.0f - powf(1.0f - slide, 3.0f);

            float width = ImMax(title_size.x, message_size.x) + 20.0f;
            float height = title_size.y + message_size.y + 15.0f;

            float x_slide_offset = (1.0f - slide) * (width + padding);
            
            ImVec2 box_pos;
            if (pos == notify_pos_t::top_left)
            {
                box_pos = ImVec2(padding - x_slide_offset, y_offset);
                y_offset += height + spacing;
            }
            else if (pos == notify_pos_t::top_right)
            {
                box_pos = ImVec2(screen_size.x - width - padding + x_slide_offset, y_offset);
                y_offset += height + spacing;
            }
            else if (pos == notify_pos_t::bottom_left)
            {
                box_pos = ImVec2(padding - x_slide_offset, y_offset - height);
                y_offset -= height + spacing;
            }
            else if (pos == notify_pos_t::bottom_right)
            {
                box_pos = ImVec2(screen_size.x - width - padding + x_slide_offset, y_offset - height);
                y_offset -= height + spacing;
            }

            ImU32 bg_col = IM_COL32(20, 20, 20, (int)(255 * alpha));
            ImU32 accent_col = draw->get_clr(clr->accent, alpha);
            ImU32 text_col = IM_COL32(255, 255, 255, (int)(255 * alpha));
            ImU32 subtext_col = IM_COL32(180, 180, 180, (int)(255 * alpha));

            // Shadow
            draw->shadow_rect(draw_list, box_pos, box_pos + ImVec2(width, height), IM_COL32(0, 0, 0, (int)(150 * alpha)), 15.0f, ImVec2(0, 0), 0, 0.0f);

            // Background box
            draw_list->AddRectFilled(box_pos, box_pos + ImVec2(width, height), bg_col, 0.0f);
            
            // Top accent line
            draw_list->AddRectFilled(box_pos, box_pos + ImVec2(width, 2.0f), accent_col, 0.0f);

            // Text
            ImGui::PushFont(var->font.tahoma);
            draw_list->AddText(box_pos + ImVec2(10, 5), text_col, n.title.c_str());
            draw_list->AddText(box_pos + ImVec2(10, 5 + title_size.y + 2), subtext_col, n.message.c_str());
            ImGui::PopFont();

            n.time -= ImGui::GetIO().DeltaTime;
        }
    };

    render_group(tl, notify_pos_t::top_left);
    render_group(tr, notify_pos_t::top_right);
    render_group(bl, notify_pos_t::bottom_left);
    render_group(br, notify_pos_t::bottom_right);

    // Remove expired notifications
    notifications.erase(std::remove_if(notifications.begin(), notifications.end(),
        [](const notification_t& n) { return n.time <= 0.0f; }), notifications.end());
}
