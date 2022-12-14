// Symphony of Empires
// Copyright (C) 2021, Symphony of Empires contributors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
// ----------------------------------------------------------------------------
// Name:
//      eng3d/ui/input.cpp
//
// Abstract:
//      Does some important stuff.
// ----------------------------------------------------------------------------

#include <cstdlib>
#include <cstring>
#include <string>
#include <algorithm>
#include <codecvt>

#include <glm/vec2.hpp>

#include "eng3d/ui/widget.hpp"
#include "eng3d/ui/input.hpp"
#include "eng3d/ui/ui.hpp"
#include "eng3d/texture.hpp"
#include "eng3d/rectangle.hpp"
#include "eng3d/state.hpp"

UI::Input::Input(int _x, int _y, unsigned w, unsigned h, Widget* _parent)
    : Widget(_parent, _x, _y, w, h, UI::WidgetType::INPUT)
{
    this->on_textinput = ([](UI::Input& w, const char* input) {
        // Carefully convert into UTF32 then back into UTF8 so we don't have to
        // use a dedicated library for UTF8 handling (for now)
        std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> conv_utf8_utf32;
        std::u32string unicode_text = conv_utf8_utf32.from_bytes(w.buffer);
        if(input != nullptr) {
            std::u32string unicode_input = conv_utf8_utf32.from_bytes(input);
            unicode_text.insert(w.curpos, unicode_input);
            w.curpos += unicode_input.length();
        } else if(!unicode_text.empty() && w.curpos) {
            unicode_text.erase(w.curpos - 1, 1);
            w.curpos--;
        }
        w.buffer = conv_utf8_utf32.to_bytes(unicode_text);
        w.text(w.buffer.empty() ? " " : w.buffer);
    });
    this->set_on_click((UI::Callback)&UI::Input::on_click_default);
    this->on_click_outside = (UI::Callback)&UI::Input::on_click_outside_default;
    this->on_update = (UI::Callback)&UI::Input::on_update_default;
}

void UI::Input::set_buffer(const std::string& _buffer) {
    buffer = _buffer;
    this->text(buffer);

    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> conv_utf8_utf32;
    std::u32string unicode_text = conv_utf8_utf32.from_bytes(this->buffer);
    this->curpos = unicode_text.length();
}

std::string UI::Input::get_buffer() const {
    return buffer;
}

void UI::Input::on_click_default(UI::Input& w) {
    w.is_selected = true;
}

void UI::Input::on_click_outside_default(UI::Input& w) {
    if(w.is_selected)
        w.text(w.buffer);
    w.is_selected = false;
}

void UI::Input::on_update_default(UI::Input& w) {
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> conv_utf8_utf32;
    std::u32string unicode_text = conv_utf8_utf32.from_bytes(w.buffer);
    if(w.curpos == unicode_text.length()) {
        w.timer = (w.timer + 1) % 30;
        const std::string cursor = w.timer >= 10 ? "_" : "";
        if(w.is_selected && w.timer % 30 == 0) {
            if(!w.buffer.empty()) {
                w.text(w.buffer + cursor);
            } else if(!cursor.empty()) {
                w.text(cursor);
            }
        }
    }
}
