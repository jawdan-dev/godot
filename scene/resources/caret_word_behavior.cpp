/**************************************************************************/
/*  caret_word_behavior.cpp                                               */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "caret_word_behavior.h"

#include "servers/text/text_server.h"

CaretWordBehavior::FullBehavior CaretWordBehavior::_get_preset_godot() {
	return (FullBehavior){
		.move_left = {
				.normal_behavior = { .break_mode = BREAK_MODE_WORD, .jump_flags = JUMP_FLAG_WHITESPACE },
				.newline_mode = NEWLINE_MODE_NEVER,
		},
		.move_right = {
				.normal_behavior = { .break_mode = BREAK_MODE_WORD, .jump_flags = JUMP_FLAG_WHITESPACE },
				.newline_mode = NEWLINE_MODE_NEVER,
		},
		.remove_left = {
				.normal_behavior = { .break_mode = BREAK_MODE_WORD, .jump_flags = JUMP_FLAG_WHITESPACE },
				.newline_mode = NEWLINE_MODE_NEVER,
		},
		.remove_right = {
				.normal_behavior = { .break_mode = BREAK_MODE_WORD, .jump_flags = JUMP_FLAG_WHITESPACE },
				.newline_mode = NEWLINE_MODE_NEVER,
		},
	};
}
CaretWordBehavior::FullBehavior CaretWordBehavior::_get_preset_vscode() {
	return (FullBehavior){
		.move_left = {
				.normal_behavior = { .break_mode = BREAK_MODE_WORD_OR_PUNCTUATION, .jump_flags = JUMP_FLAG_WHITESPACE | JUMP_FLAG_SINGLE_PUNCTUATION },
				.newline_mode = NEWLINE_MODE_ALWAYS,
				.newline_behavior = { .break_mode = BREAK_MODE_WORD_OR_PUNCTUATION, .jump_flags = JUMP_FLAG_WHITESPACE | JUMP_FLAG_SINGLE_PUNCTUATION },
		},
		.move_right = {
				.normal_behavior = { .break_mode = BREAK_MODE_WORD_OR_PUNCTUATION, .jump_flags = JUMP_FLAG_WHITESPACE | JUMP_FLAG_SINGLE_PUNCTUATION },
				.newline_mode = NEWLINE_MODE_ALWAYS,
				.newline_behavior = { .break_mode = BREAK_MODE_WORD_OR_PUNCTUATION, .jump_flags = JUMP_FLAG_WHITESPACE | JUMP_FLAG_SINGLE_PUNCTUATION },
		},
		.remove_left = {
				.normal_behavior = { .break_mode = BREAK_MODE_WORD_OR_PUNCTUATION, .jump_flags = JUMP_FLAG_SINGLE_WHITESPACE },
				.newline_mode = NEWLINE_MODE_NEVER,
		},
		.remove_right = {
				.normal_behavior = { .break_mode = BREAK_MODE_WORD_OR_PUNCTUATION, .jump_flags = JUMP_FLAG_SINGLE_WHITESPACE },
				.newline_mode = NEWLINE_MODE_ON_WHITESPACE,
				.newline_behavior = { .break_mode = BREAK_MODE_WORD_OR_PUNCTUATION, .jump_flags = JUMP_FLAG_NONE },
		},
	};
}
CaretWordBehavior::FullBehavior CaretWordBehavior::_get_preset(Preset p_preset) const {
	switch (p_preset) {
		case PRESET_GODOT:
			return _get_preset_godot();
		case PRESET_VSCODE:
			return _get_preset_vscode();
		case PRESET_CUSTOM:
			return behavior;
	}
	return behavior;
}

CaretWordBehavior::NextCaretBehavior CaretWordBehavior::_get_next_word_caret_behavior(const String &p_line, int p_start_column, int p_next_column, const LineBehavior &p_behavior, bool p_at_start) const {
	bool start_column_valid = 0 <= p_start_column && p_start_column < p_line.length();
	bool next_column_valid = 0 <= p_next_column && p_next_column < p_line.length();

	int length_to_last = (p_next_column > p_start_column) ? (p_next_column - p_start_column) : (p_start_column - p_next_column);
	bool at_whitespace = !start_column_valid || is_whitespace(p_line[p_start_column]);
	bool at_whitespace_next = !next_column_valid || is_whitespace(p_line[p_next_column]);
	bool at_punctuation = start_column_valid && is_punct(p_line[p_start_column]);
	bool at_punctuation_next = next_column_valid && is_punct(p_line[p_next_column]);

	switch (p_behavior.break_mode) {
		case BREAK_MODE_WORD: {
			// Punctuation is considered as <whitespace>.
			at_whitespace = at_whitespace || at_punctuation;
			at_whitespace_next = at_whitespace_next || at_punctuation_next;
			at_punctuation = false;
			at_punctuation_next = false;
		} break;
		case BREAK_MODE_WORD_AND_PUNCTUATION: {
			// Punctuation is considered as <word>.
			at_punctuation = false;
			at_punctuation_next = false;
		} break;
		case BREAK_MODE_WORD_OR_PUNCTUATION:
			// Punctuation is considered as <punctuation>.
			break;
	}

	if ((p_behavior.jump_flags & JUMP_FLAG_SINGLE_WHITESPACE) && p_at_start && at_whitespace && length_to_last == 1) {
		return NEXT_CARET_BEHAVIOR_INCREMENT;
	}
	if ((p_behavior.jump_flags & JUMP_FLAG_WHITESPACE) && p_at_start && at_whitespace) {
		return NEXT_CARET_BEHAVIOR_INCREMENT;
	}
	if ((p_behavior.jump_flags & JUMP_FLAG_SINGLE_PUNCTUATION) && p_at_start && at_punctuation && !at_whitespace_next && length_to_last == 1) {
		return NEXT_CARET_BEHAVIOR_CONTINUE;
	}

	return NEXT_CARET_BEHAVIOR_BREAK;
}
PackedInt32Array CaretWordBehavior::_get_word_break_carets(RID p_text_shaped, BreakMode p_mode) const {
	PackedInt32Array breaks = { 0, TS->shaped_get_text(p_text_shaped).length() };
	switch (p_mode) {
		case BREAK_MODE_WORD: {
			// Break <text> on <punctuation> and <whitespace>.
			breaks.append_array(TS->shaped_text_get_word_breaks(p_text_shaped, TextServer::GRAPHEME_IS_SPACE | TextServer::GRAPHEME_IS_PUNCTUATION));
		} break;
		case BREAK_MODE_WORD_AND_PUNCTUATION: {
			// Consider <text> and <punctuation> as one, break on <whitespace>.
			breaks.append_array(TS->shaped_text_get_word_breaks(p_text_shaped, TextServer::GRAPHEME_IS_SPACE));
		} break;
		case BREAK_MODE_WORD_OR_PUNCTUATION: {
			// Break individually for <text>, <whitespace>, and <punctuation>.
			breaks.append_array(TS->shaped_text_get_word_breaks(p_text_shaped, TextServer::GRAPHEME_IS_SPACE));
			breaks.append_array(TS->shaped_text_get_word_breaks(p_text_shaped, TextServer::GRAPHEME_IS_PUNCTUATION));
		} break;
	}

	// Sort & remove duplicate breaks.
	breaks.sort();
	for (int j = breaks.size() - 1; j > 0; j--) {
		if (breaks[j - 1] == breaks[j]) {
			breaks.remove_at(j);
		}
	}

	return breaks;
}
int CaretWordBehavior::_get_next_word_caret_directional(RID p_text_shaped, int p_column, bool p_is_newline, const ContextBehavior &p_behavior, int p_direction) const {
	DEV_ASSERT(p_direction == 1 || p_direction == -1);

	String line = TS->shaped_get_text(p_text_shaped);

	// Handle newline mode.
	int newline_check_column = p_direction > 0 ? p_column : p_column - 1;
	if (p_is_newline && !((p_behavior.newline_mode & NEWLINE_MODE_ALWAYS) || ((p_behavior.newline_mode & NEWLINE_MODE_ON_WHITESPACE) && newline_check_column >= 0 && newline_check_column < line.length() && is_whitespace(line[newline_check_column])))) {
		// No behavior to happen.
		return p_column;
	}

	const LineBehavior &line_behavior = p_is_newline ? p_behavior.newline_behavior : p_behavior.normal_behavior;
	const PackedInt32Array word_breaks = _get_word_break_carets(p_text_shaped, line_behavior.break_mode);

	// Find starting word break index.
	int start = 0;
	if (p_direction > 0) {
		while (0 <= start + p_direction && start + p_direction < word_breaks.size() && word_breaks[start + p_direction] <= p_column) {
			start += p_direction;
		}
	} else {
		start = word_breaks.size() - 1;
		while (0 <= start + p_direction && start + p_direction < word_breaks.size() && word_breaks[start + p_direction] >= p_column) {
			start += p_direction;
		}
	}

	// Handle behavior.
	int end = start;
	bool first = true;
	NextCaretBehavior nextCaretBehavior = NEXT_CARET_BEHAVIOR_CONTINUE;
	for (; 0 <= end + p_direction && end + p_direction < word_breaks.size() && nextCaretBehavior != NEXT_CARET_BEHAVIOR_BREAK; end += p_direction) {
		// Get column jump locations.
		int start_column = first ? p_column : word_breaks[end];
		int next_column = word_breaks[end + p_direction];
		first = false;

		// If going left, look at left of caret.
		if (p_direction < 0) {
			start_column -= 1;
			next_column -= 1;
		}

		// Handle specific word-to-word behavior.
		nextCaretBehavior = _get_next_word_caret_behavior(line, start_column, next_column, line_behavior, start == end);
		switch (nextCaretBehavior) {
			default:
				break;
			case NEXT_CARET_BEHAVIOR_INCREMENT: {
				start += p_direction;
			} break;
		}
	}

	// Out of bounds check.
	if (0 > end) {
		return 0;
	}
	if (end >= word_breaks.size()) {
		return line.length();
	}

	// Jump to start/end of 'word'.
	return word_breaks[end];
}

void CaretWordBehavior::_bind_methods() {
	BIND_ENUM_CONSTANT(PRESET_GODOT);
	BIND_ENUM_CONSTANT(PRESET_VSCODE);
	BIND_ENUM_CONSTANT(PRESET_CUSTOM);

	BIND_ENUM_CONSTANT(BREAK_MODE_WORD);
	BIND_ENUM_CONSTANT(BREAK_MODE_WORD_AND_PUNCTUATION);
	BIND_ENUM_CONSTANT(BREAK_MODE_WORD_OR_PUNCTUATION);

	BIND_ENUM_CONSTANT(JUMP_FLAG_NONE);
	BIND_ENUM_CONSTANT(JUMP_FLAG_SINGLE_WHITESPACE);
	BIND_ENUM_CONSTANT(JUMP_FLAG_WHITESPACE);
	BIND_ENUM_CONSTANT(JUMP_FLAG_SINGLE_PUNCTUATION);

	BIND_ENUM_CONSTANT(NEWLINE_MODE_NEVER);
	BIND_ENUM_CONSTANT(NEWLINE_MODE_ON_WHITESPACE);
	BIND_ENUM_CONSTANT(NEWLINE_MODE_ALWAYS);

	ClassDB::bind_method(D_METHOD("get_next_word_caret_left", "text_shaped", "start_column", "is_newline", "is_remove"), &CaretWordBehavior::get_next_word_caret_left);
	ClassDB::bind_method(D_METHOD("get_next_word_caret_right", "text_shaped", "start_column", "is_newline", "is_remove"), &CaretWordBehavior::get_next_word_caret_right);

	ClassDB::bind_method(D_METHOD("set_preset", "preset"), &CaretWordBehavior::set_preset);
	ClassDB::bind_method(D_METHOD("get_preset"), &CaretWordBehavior::get_preset);
	ClassDB::bind_method(D_METHOD("set_move_left_preset", "preset"), &CaretWordBehavior::set_move_left_preset);
	ClassDB::bind_method(D_METHOD("get_move_left_preset"), &CaretWordBehavior::get_move_left_preset);
	ClassDB::bind_method(D_METHOD("set_move_right_preset", "preset"), &CaretWordBehavior::set_move_right_preset);
	ClassDB::bind_method(D_METHOD("get_move_right_preset"), &CaretWordBehavior::get_move_right_preset);
	ClassDB::bind_method(D_METHOD("set_remove_left_preset", "preset"), &CaretWordBehavior::set_remove_left_preset);
	ClassDB::bind_method(D_METHOD("get_remove_left_preset"), &CaretWordBehavior::get_remove_left_preset);
	ClassDB::bind_method(D_METHOD("set_remove_right_preset", "preset"), &CaretWordBehavior::set_remove_right_preset);
	ClassDB::bind_method(D_METHOD("get_remove_right_preset"), &CaretWordBehavior::get_remove_right_preset);

	ClassDB::bind_method(D_METHOD("set_move_left_normal_break_mode", "break_mode"), &CaretWordBehavior::set_move_left_normal_break_mode);
	ClassDB::bind_method(D_METHOD("get_move_left_normal_break_mode"), &CaretWordBehavior::get_move_left_normal_break_mode);
	ClassDB::bind_method(D_METHOD("set_move_left_normal_jump_flags", "jump_flags"), &CaretWordBehavior::set_move_left_normal_jump_flags);
	ClassDB::bind_method(D_METHOD("get_move_left_normal_jump_flags"), &CaretWordBehavior::get_move_left_normal_jump_flags);
	ClassDB::bind_method(D_METHOD("set_move_left_newline_mode", "newline_mode"), &CaretWordBehavior::set_move_left_newline_mode);
	ClassDB::bind_method(D_METHOD("get_move_left_newline_mode"), &CaretWordBehavior::get_move_left_newline_mode);
	ClassDB::bind_method(D_METHOD("set_move_left_newline_break_mode", "break_mode"), &CaretWordBehavior::set_move_left_newline_break_mode);
	ClassDB::bind_method(D_METHOD("get_move_left_newline_break_mode"), &CaretWordBehavior::get_move_left_newline_break_mode);
	ClassDB::bind_method(D_METHOD("set_move_left_newline_jump_flags", "jump_flags"), &CaretWordBehavior::set_move_left_newline_jump_flags);
	ClassDB::bind_method(D_METHOD("get_move_left_newline_jump_flags"), &CaretWordBehavior::get_move_left_newline_jump_flags);

	ClassDB::bind_method(D_METHOD("set_move_right_normal_break_mode", "break_mode"), &CaretWordBehavior::set_move_right_normal_break_mode);
	ClassDB::bind_method(D_METHOD("get_move_right_normal_break_mode"), &CaretWordBehavior::get_move_right_normal_break_mode);
	ClassDB::bind_method(D_METHOD("set_move_right_normal_jump_flags", "jump_flags"), &CaretWordBehavior::set_move_right_normal_jump_flags);
	ClassDB::bind_method(D_METHOD("get_move_right_normal_jump_flags"), &CaretWordBehavior::get_move_right_normal_jump_flags);
	ClassDB::bind_method(D_METHOD("set_move_right_newline_mode", "newline_mode"), &CaretWordBehavior::set_move_right_newline_mode);
	ClassDB::bind_method(D_METHOD("get_move_right_newline_mode"), &CaretWordBehavior::get_move_right_newline_mode);
	ClassDB::bind_method(D_METHOD("set_move_right_newline_break_mode", "break_mode"), &CaretWordBehavior::set_move_right_newline_break_mode);
	ClassDB::bind_method(D_METHOD("get_move_right_newline_break_mode"), &CaretWordBehavior::get_move_right_newline_break_mode);
	ClassDB::bind_method(D_METHOD("set_move_right_newline_jump_flags", "jump_flags"), &CaretWordBehavior::set_move_right_newline_jump_flags);
	ClassDB::bind_method(D_METHOD("get_move_right_newline_jump_flags"), &CaretWordBehavior::get_move_right_newline_jump_flags);

	ClassDB::bind_method(D_METHOD("set_remove_left_normal_break_mode", "break_mode"), &CaretWordBehavior::set_remove_left_normal_break_mode);
	ClassDB::bind_method(D_METHOD("get_remove_left_normal_break_mode"), &CaretWordBehavior::get_remove_left_normal_break_mode);
	ClassDB::bind_method(D_METHOD("set_remove_left_normal_jump_flags", "jump_flags"), &CaretWordBehavior::set_remove_left_normal_jump_flags);
	ClassDB::bind_method(D_METHOD("get_remove_left_normal_jump_flags"), &CaretWordBehavior::get_remove_left_normal_jump_flags);
	ClassDB::bind_method(D_METHOD("set_remove_left_newline_mode", "newline_mode"), &CaretWordBehavior::set_remove_left_newline_mode);
	ClassDB::bind_method(D_METHOD("get_remove_left_newline_mode"), &CaretWordBehavior::get_remove_left_newline_mode);
	ClassDB::bind_method(D_METHOD("set_remove_left_newline_break_mode", "break_mode"), &CaretWordBehavior::set_remove_left_newline_break_mode);
	ClassDB::bind_method(D_METHOD("get_remove_left_newline_break_mode"), &CaretWordBehavior::get_remove_left_newline_break_mode);
	ClassDB::bind_method(D_METHOD("set_remove_left_newline_jump_flags", "jump_flags"), &CaretWordBehavior::set_remove_left_newline_jump_flags);
	ClassDB::bind_method(D_METHOD("get_remove_left_newline_jump_flags"), &CaretWordBehavior::get_remove_left_newline_jump_flags);

	ClassDB::bind_method(D_METHOD("set_remove_right_normal_break_mode", "break_mode"), &CaretWordBehavior::set_remove_right_normal_break_mode);
	ClassDB::bind_method(D_METHOD("get_remove_right_normal_break_mode"), &CaretWordBehavior::get_remove_right_normal_break_mode);
	ClassDB::bind_method(D_METHOD("set_remove_right_normal_jump_flags", "jump_flags"), &CaretWordBehavior::set_remove_right_normal_jump_flags);
	ClassDB::bind_method(D_METHOD("get_remove_right_normal_jump_flags"), &CaretWordBehavior::get_remove_right_normal_jump_flags);
	ClassDB::bind_method(D_METHOD("set_remove_right_newline_mode", "newline_mode"), &CaretWordBehavior::set_remove_right_newline_mode);
	ClassDB::bind_method(D_METHOD("get_remove_right_newline_mode"), &CaretWordBehavior::get_remove_right_newline_mode);
	ClassDB::bind_method(D_METHOD("set_remove_right_newline_break_mode", "break_mode"), &CaretWordBehavior::set_remove_right_newline_break_mode);
	ClassDB::bind_method(D_METHOD("get_remove_right_newline_break_mode"), &CaretWordBehavior::get_remove_right_newline_break_mode);
	ClassDB::bind_method(D_METHOD("set_remove_right_newline_jump_flags", "jump_flags"), &CaretWordBehavior::set_remove_right_newline_jump_flags);
	ClassDB::bind_method(D_METHOD("get_remove_right_newline_jump_flags"), &CaretWordBehavior::get_remove_right_newline_jump_flags);

	const char *preset_hint = "Godot,VSCode,Custom";
	const char *break_mode_hint = "Word,Word and Punctuation,Word or Punctuation";
	const char *jump_flag_hint = "Single Whitespace,Whitespace,Single Punctuation";
	const char *newline_mode_hint = "Never,On Whitespace,Always";

	ADD_PROPERTY(PropertyInfo(Variant::INT, "preset", PROPERTY_HINT_ENUM, preset_hint), "set_preset", "get_preset");
	ADD_GROUP("Move Left Behavior", "move_left_");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "move_left_preset", PROPERTY_HINT_ENUM, preset_hint), "set_move_left_preset", "get_move_left_preset");
	ADD_SUBGROUP("Normal Behavior", "move_left_normal_");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "move_left_normal_break_mode", PROPERTY_HINT_ENUM, break_mode_hint), "set_move_left_normal_break_mode", "get_move_left_normal_break_mode");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "move_left_normal_jump_flags", PROPERTY_HINT_FLAGS, jump_flag_hint), "set_move_left_normal_jump_flags", "get_move_left_normal_jump_flags");
	ADD_SUBGROUP("Newline Behavior", "move_left_newline_");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "move_left_newline_mode", PROPERTY_HINT_ENUM, newline_mode_hint), "set_move_left_newline_mode", "get_move_left_newline_mode");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "move_left_newline_break_mode", PROPERTY_HINT_ENUM, break_mode_hint), "set_move_left_newline_break_mode", "get_move_left_newline_break_mode");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "move_left_newline_jump_flags", PROPERTY_HINT_FLAGS, jump_flag_hint), "set_move_left_newline_jump_flags", "get_move_left_newline_jump_flags");

	ADD_GROUP("Move Right Behavior", "move_right_");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "move_right_preset", PROPERTY_HINT_ENUM, preset_hint), "set_move_right_preset", "get_move_right_preset");
	ADD_SUBGROUP("Normal Behavior", "move_right_normal_");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "move_right_normal_break_mode", PROPERTY_HINT_ENUM, break_mode_hint), "set_move_right_normal_break_mode", "get_move_right_normal_break_mode");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "move_right_normal_jump_flags", PROPERTY_HINT_FLAGS, jump_flag_hint), "set_move_right_normal_jump_flags", "get_move_right_normal_jump_flags");
	ADD_SUBGROUP("Newline Behavior", "move_right_newline_");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "move_right_newline_mode", PROPERTY_HINT_ENUM, newline_mode_hint), "set_move_right_newline_mode", "get_move_right_newline_mode");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "move_right_newline_break_mode", PROPERTY_HINT_ENUM, break_mode_hint), "set_move_right_newline_break_mode", "get_move_right_newline_break_mode");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "move_right_newline_jump_flags", PROPERTY_HINT_FLAGS, jump_flag_hint), "set_move_right_newline_jump_flags", "get_move_right_newline_jump_flags");

	ADD_GROUP("Remove Left Behavior", "remove_left_");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "remove_left_preset", PROPERTY_HINT_ENUM, preset_hint), "set_remove_left_preset", "get_remove_left_preset");
	ADD_SUBGROUP("Normal Behavior", "remove_left_normal_");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "remove_left_normal_break_mode", PROPERTY_HINT_ENUM, break_mode_hint), "set_remove_left_normal_break_mode", "get_remove_left_normal_break_mode");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "remove_left_normal_jump_flags", PROPERTY_HINT_FLAGS, jump_flag_hint), "set_remove_left_normal_jump_flags", "get_remove_left_normal_jump_flags");
	ADD_SUBGROUP("Newline Behavior", "remove_left_newline_");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "remove_left_newline_mode", PROPERTY_HINT_ENUM, newline_mode_hint), "set_remove_left_newline_mode", "get_remove_left_newline_mode");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "remove_left_newline_break_mode", PROPERTY_HINT_ENUM, break_mode_hint), "set_remove_left_newline_break_mode", "get_remove_left_newline_break_mode");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "remove_left_newline_jump_flags", PROPERTY_HINT_FLAGS, jump_flag_hint), "set_remove_left_newline_jump_flags", "get_remove_left_newline_jump_flags");

	ADD_GROUP("Remove Right Behavior", "remove_right_");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "remove_right_preset", PROPERTY_HINT_ENUM, preset_hint), "set_remove_right_preset", "get_remove_right_preset");
	ADD_SUBGROUP("Normal Behavior", "remove_right_normal_");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "remove_right_normal_break_mode", PROPERTY_HINT_ENUM, break_mode_hint), "set_remove_right_normal_break_mode", "get_remove_right_normal_break_mode");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "remove_right_normal_jump_flags", PROPERTY_HINT_FLAGS, jump_flag_hint), "set_remove_right_normal_jump_flags", "get_remove_right_normal_jump_flags");
	ADD_SUBGROUP("Newline Behavior", "remove_right_newline_");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "remove_right_newline_mode", PROPERTY_HINT_ENUM, newline_mode_hint), "set_remove_right_newline_mode", "get_remove_right_newline_mode");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "remove_right_newline_break_mode", PROPERTY_HINT_ENUM, break_mode_hint), "set_remove_right_newline_break_mode", "get_remove_right_newline_break_mode");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "remove_right_newline_jump_flags", PROPERTY_HINT_FLAGS, jump_flag_hint), "set_remove_right_newline_jump_flags", "get_remove_right_newline_jump_flags");


	ADD_LINKED_PROPERTY("preset", "move_left_preset");
	ADD_LINKED_PROPERTY("preset", "move_right_preset");
	ADD_LINKED_PROPERTY("preset", "remove_left_preset");
	ADD_LINKED_PROPERTY("preset", "remove_right_preset");

#define ADD_PRESET_LINK(category_preset, property) \
	ADD_LINKED_PROPERTY("preset", property); \
	ADD_LINKED_PROPERTY(category_preset, property);

	ADD_PRESET_LINK("move_left_preset", "move_left_normal_break_mode");
	ADD_PRESET_LINK("move_left_preset", "move_left_normal_jump_flags");
	ADD_PRESET_LINK("move_left_preset", "move_left_newline_mode");
	ADD_PRESET_LINK("move_left_preset", "move_left_newline_break_mode");
	ADD_PRESET_LINK("move_left_preset", "move_left_newline_jump_flags");

	ADD_PRESET_LINK("move_right_preset", "move_right_normal_break_mode");
	ADD_PRESET_LINK("move_right_preset", "move_right_normal_jump_flags");
	ADD_PRESET_LINK("move_right_preset", "move_right_newline_mode");
	ADD_PRESET_LINK("move_right_preset", "move_right_newline_break_mode");
	ADD_PRESET_LINK("move_right_preset", "move_right_newline_jump_flags");

	ADD_PRESET_LINK("remove_left_preset", "remove_left_normal_break_mode");
	ADD_PRESET_LINK("remove_left_preset", "remove_left_normal_jump_flags");
	ADD_PRESET_LINK("remove_left_preset", "remove_left_newline_mode");
	ADD_PRESET_LINK("remove_left_preset", "remove_left_newline_break_mode");
	ADD_PRESET_LINK("remove_left_preset", "remove_left_newline_jump_flags");

	ADD_PRESET_LINK("remove_right_preset", "remove_right_normal_break_mode");
	ADD_PRESET_LINK("remove_right_preset", "remove_right_normal_jump_flags");
	ADD_PRESET_LINK("remove_right_preset", "remove_right_newline_mode");
	ADD_PRESET_LINK("remove_right_preset", "remove_right_newline_break_mode");
	ADD_PRESET_LINK("remove_right_preset", "remove_right_newline_jump_flags");

#undef ADD_PRESET_LINK
}

Ref<CaretWordBehavior> CaretWordBehavior::get_fallback_caret_word_behavior() {
	// FIXME: Not 100% on this being the best way to do a fallback...
	static Ref<CaretWordBehavior> fallback;
	if (fallback.is_null()) {
		fallback.instantiate();
	}
	return fallback;
}

int CaretWordBehavior::get_next_word_caret_left(RID p_text_shaped, int p_column, bool p_is_newline, bool p_is_remove) const {
	return _get_next_word_caret_directional(p_text_shaped, p_column, p_is_newline, p_is_remove ? behavior.remove_left : behavior.move_left, -1);
}
int CaretWordBehavior::get_next_word_caret_right(RID p_text_shaped, int p_column, bool p_is_newline, bool p_is_remove) const {
	return _get_next_word_caret_directional(p_text_shaped, p_column, p_is_newline, p_is_remove ? behavior.remove_right : behavior.move_right, 1);
}

void CaretWordBehavior::set_preset(Preset p_preset) {
	switch (p_preset) {
		default: {
			behavior = _get_preset(p_preset);
		} break;
		case PRESET_CUSTOM:
			break;
	}
}
CaretWordBehavior::Preset CaretWordBehavior::get_preset() const {
	Preset preset = get_move_left_preset();
	if (get_move_right_preset() == preset && get_remove_left_preset() == preset && get_remove_right_preset() == preset) {
		return preset;
	}
	return PRESET_CUSTOM;
}
void CaretWordBehavior::set_move_left_preset(Preset p_preset) {
	behavior.move_left = _get_preset(p_preset).move_left;
}
CaretWordBehavior::Preset CaretWordBehavior::get_move_left_preset() const {
	for (int i = 0; i < PRESET_CUSTOM; i++) {
		Preset preset = (Preset)i;
		if (behavior.move_left == _get_preset(preset).move_left) {
			return preset;
		}
	}
	return PRESET_CUSTOM;
}
void CaretWordBehavior::set_move_right_preset(Preset p_preset) {
	behavior.move_right = _get_preset(p_preset).move_right;
}
CaretWordBehavior::Preset CaretWordBehavior::get_move_right_preset() const {
	for (int i = 0; i < PRESET_CUSTOM; i++) {
		Preset preset = (Preset)i;
		if (behavior.move_right == _get_preset(preset).move_right) {
			return preset;
		}
	}
	return PRESET_CUSTOM;
}
void CaretWordBehavior::set_remove_left_preset(Preset p_preset) {
	behavior.remove_left = _get_preset(p_preset).remove_left;
}
CaretWordBehavior::Preset CaretWordBehavior::get_remove_left_preset() const {
	for (int i = 0; i < PRESET_CUSTOM; i++) {
		Preset preset = (Preset)i;
		if (behavior.remove_left == _get_preset(preset).remove_left) {
			return preset;
		}
	}
	return PRESET_CUSTOM;
}
void CaretWordBehavior::set_remove_right_preset(Preset p_preset) {
	behavior.remove_right = _get_preset(p_preset).remove_right;
}
CaretWordBehavior::Preset CaretWordBehavior::get_remove_right_preset() const {
	for (int i = 0; i < PRESET_CUSTOM; i++) {
		Preset preset = (Preset)i;
		if (behavior.remove_right == _get_preset(preset).remove_right) {
			return preset;
		}
	}
	return PRESET_CUSTOM;
}

void CaretWordBehavior::set_move_left_normal_break_mode(BreakMode p_break_mode) {
	behavior.move_left.normal_behavior.break_mode = p_break_mode;
}
CaretWordBehavior::BreakMode CaretWordBehavior::get_move_left_normal_break_mode() const {
	return behavior.move_left.normal_behavior.break_mode;
}
void CaretWordBehavior::set_move_left_normal_jump_flags(uint32_t p_jump_flags) {
	behavior.move_left.normal_behavior.jump_flags = p_jump_flags;
}
uint32_t CaretWordBehavior::get_move_left_normal_jump_flags() const {
	return behavior.move_left.normal_behavior.jump_flags;
}
void CaretWordBehavior::set_move_left_newline_mode(NewlineMode p_newline_mode) {
	behavior.move_left.newline_mode = p_newline_mode;
}
CaretWordBehavior::NewlineMode CaretWordBehavior::get_move_left_newline_mode() const {
	return behavior.move_left.newline_mode;
}
void CaretWordBehavior::set_move_left_newline_break_mode(BreakMode p_break_mode) {
	behavior.move_left.newline_behavior.break_mode = p_break_mode;
}
CaretWordBehavior::BreakMode CaretWordBehavior::get_move_left_newline_break_mode() const {
	return behavior.move_left.newline_behavior.break_mode;
}
void CaretWordBehavior::set_move_left_newline_jump_flags(uint32_t p_jump_flags) {
	behavior.move_left.newline_behavior.jump_flags = p_jump_flags;
}
uint32_t CaretWordBehavior::get_move_left_newline_jump_flags() const {
	return behavior.move_left.newline_behavior.jump_flags;
}

void CaretWordBehavior::set_move_right_normal_break_mode(BreakMode p_break_mode) {
	behavior.move_right.normal_behavior.break_mode = p_break_mode;
}
CaretWordBehavior::BreakMode CaretWordBehavior::get_move_right_normal_break_mode() const {
	return behavior.move_right.normal_behavior.break_mode;
}
void CaretWordBehavior::set_move_right_normal_jump_flags(uint32_t p_jump_flags) {
	behavior.move_right.normal_behavior.jump_flags = p_jump_flags;
}
uint32_t CaretWordBehavior::get_move_right_normal_jump_flags() const {
	return behavior.move_right.normal_behavior.jump_flags;
}
void CaretWordBehavior::set_move_right_newline_mode(NewlineMode p_newline_mode) {
	behavior.move_right.newline_mode = p_newline_mode;
}
CaretWordBehavior::NewlineMode CaretWordBehavior::get_move_right_newline_mode() const {
	return behavior.move_right.newline_mode;
}
void CaretWordBehavior::set_move_right_newline_break_mode(BreakMode p_break_mode) {
	behavior.move_right.newline_behavior.break_mode = p_break_mode;
}
CaretWordBehavior::BreakMode CaretWordBehavior::get_move_right_newline_break_mode() const {
	return behavior.move_right.newline_behavior.break_mode;
}
void CaretWordBehavior::set_move_right_newline_jump_flags(uint32_t p_jump_flags) {
	behavior.move_right.newline_behavior.jump_flags = p_jump_flags;
}
uint32_t CaretWordBehavior::get_move_right_newline_jump_flags() const {
	return behavior.move_right.newline_behavior.jump_flags;
}

void CaretWordBehavior::set_remove_left_normal_break_mode(BreakMode p_break_mode) {
	behavior.remove_left.normal_behavior.break_mode = p_break_mode;
}
CaretWordBehavior::BreakMode CaretWordBehavior::get_remove_left_normal_break_mode() const {
	return behavior.remove_left.normal_behavior.break_mode;
}
void CaretWordBehavior::set_remove_left_normal_jump_flags(uint32_t p_jump_flags) {
	behavior.remove_left.normal_behavior.jump_flags = p_jump_flags;
}
uint32_t CaretWordBehavior::get_remove_left_normal_jump_flags() const {
	return behavior.remove_left.normal_behavior.jump_flags;
}
void CaretWordBehavior::set_remove_left_newline_mode(NewlineMode p_newline_mode) {
	behavior.remove_left.newline_mode = p_newline_mode;
}
CaretWordBehavior::NewlineMode CaretWordBehavior::get_remove_left_newline_mode() const {
	return behavior.remove_left.newline_mode;
}
void CaretWordBehavior::set_remove_left_newline_break_mode(BreakMode p_break_mode) {
	behavior.remove_left.newline_behavior.break_mode = p_break_mode;
}
CaretWordBehavior::BreakMode CaretWordBehavior::get_remove_left_newline_break_mode() const {
	return behavior.remove_left.newline_behavior.break_mode;
}
void CaretWordBehavior::set_remove_left_newline_jump_flags(uint32_t p_jump_flags) {
	behavior.remove_left.newline_behavior.jump_flags = p_jump_flags;
}
uint32_t CaretWordBehavior::get_remove_left_newline_jump_flags() const {
	return behavior.remove_left.newline_behavior.jump_flags;
}

void CaretWordBehavior::set_remove_right_normal_break_mode(BreakMode p_break_mode) {
	behavior.remove_right.normal_behavior.break_mode = p_break_mode;
}
CaretWordBehavior::BreakMode CaretWordBehavior::get_remove_right_normal_break_mode() const {
	return behavior.remove_right.normal_behavior.break_mode;
}
void CaretWordBehavior::set_remove_right_normal_jump_flags(uint32_t p_jump_flags) {
	behavior.remove_right.normal_behavior.jump_flags = p_jump_flags;
}
uint32_t CaretWordBehavior::get_remove_right_normal_jump_flags() const {
	return behavior.remove_right.normal_behavior.jump_flags;
}
void CaretWordBehavior::set_remove_right_newline_mode(NewlineMode p_newline_mode) {
	behavior.remove_right.newline_mode = p_newline_mode;
}
CaretWordBehavior::NewlineMode CaretWordBehavior::get_remove_right_newline_mode() const {
	return behavior.remove_right.newline_mode;
}
void CaretWordBehavior::set_remove_right_newline_break_mode(BreakMode p_break_mode) {
	behavior.remove_right.newline_behavior.break_mode = p_break_mode;
}
CaretWordBehavior::BreakMode CaretWordBehavior::get_remove_right_newline_break_mode() const {
	return behavior.remove_right.newline_behavior.break_mode;
}
void CaretWordBehavior::set_remove_right_newline_jump_flags(uint32_t p_jump_flags) {
	behavior.remove_right.newline_behavior.jump_flags = p_jump_flags;
}
uint32_t CaretWordBehavior::get_remove_right_newline_jump_flags() const {
	return behavior.remove_right.newline_behavior.jump_flags;
}

const bool operator==(const CaretWordBehavior::ContextBehavior &a, const CaretWordBehavior::ContextBehavior &b) {
	return (a.normal_behavior.break_mode == b.normal_behavior.break_mode &&
			a.normal_behavior.jump_flags == b.normal_behavior.jump_flags &&
			a.newline_mode == b.newline_mode &&
			a.newline_behavior.break_mode == b.newline_behavior.break_mode &&
			a.newline_behavior.jump_flags == b.newline_behavior.jump_flags);
}