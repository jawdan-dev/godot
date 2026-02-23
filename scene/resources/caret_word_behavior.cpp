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
int CaretWordBehavior::_get_next_word_caret_directional(RID p_text_shaped, int p_column, bool p_is_newline, const FullBehavior &p_behavior, int p_direction) const {
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

	const char *break_mode_hint = "Word,Word and Punctuation,Word or Punctuation";
	const char *jump_flag_hint = "Single Whitespace,Whitespace,Single Punctuation";
	const char *newline_mode_hint = "Never,On Whitespace,Always";

	ADD_GROUP("Caret Move Left Behavior", "move_left_");
	ADD_SUBGROUP("Normal Behavior", "move_left_normal_");
	ClassDB::bind_method(D_METHOD("set_move_left_normal_break_mode", "break_mode"), &CaretWordBehavior::set_move_left_normal_break_mode);
	ClassDB::bind_method(D_METHOD("get_move_left_normal_break_mode"), &CaretWordBehavior::get_move_left_normal_break_mode);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "move_left_normal_break_mode", PROPERTY_HINT_ENUM, break_mode_hint), "set_move_left_normal_break_mode", "get_move_left_normal_break_mode");
	ClassDB::bind_method(D_METHOD("set_move_left_normal_jump_flags", "jump_flags"), &CaretWordBehavior::set_move_left_normal_jump_flags);
	ClassDB::bind_method(D_METHOD("get_move_left_normal_jump_flags"), &CaretWordBehavior::get_move_left_normal_jump_flags);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "move_left_normal_jump_flags", PROPERTY_HINT_FLAGS, jump_flag_hint), "set_move_left_normal_jump_flags", "get_move_left_normal_jump_flags");
	ADD_SUBGROUP("Newline Behavior", "move_left_newline_");
	ClassDB::bind_method(D_METHOD("set_move_left_newline_mode", "newline_mode"), &CaretWordBehavior::set_move_left_newline_mode);
	ClassDB::bind_method(D_METHOD("get_move_left_newline_mode"), &CaretWordBehavior::get_move_left_newline_mode);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "move_left_newline_mode", PROPERTY_HINT_ENUM, newline_mode_hint), "set_move_left_newline_mode", "get_move_left_newline_mode");
	ClassDB::bind_method(D_METHOD("set_move_left_newline_break_mode", "break_mode"), &CaretWordBehavior::set_move_left_newline_break_mode);
	ClassDB::bind_method(D_METHOD("get_move_left_newline_break_mode"), &CaretWordBehavior::get_move_left_newline_break_mode);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "move_left_newline_break_mode", PROPERTY_HINT_ENUM, break_mode_hint), "set_move_left_newline_break_mode", "get_move_left_newline_break_mode");
	ClassDB::bind_method(D_METHOD("set_move_left_newline_jump_flags", "jump_flags"), &CaretWordBehavior::set_move_left_newline_jump_flags);
	ClassDB::bind_method(D_METHOD("get_move_left_newline_jump_flags"), &CaretWordBehavior::get_move_left_newline_jump_flags);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "move_left_newline_jump_flags", PROPERTY_HINT_FLAGS, jump_flag_hint), "set_move_left_newline_jump_flags", "get_move_left_newline_jump_flags");

	ADD_GROUP("Caret Move Right Behavior", "move_right_");
	ADD_SUBGROUP("Normal Behavior", "move_right_normal_");
	ClassDB::bind_method(D_METHOD("set_move_right_normal_break_mode", "break_mode"), &CaretWordBehavior::set_move_right_normal_break_mode);
	ClassDB::bind_method(D_METHOD("get_move_right_normal_break_mode"), &CaretWordBehavior::get_move_right_normal_break_mode);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "move_right_normal_break_mode", PROPERTY_HINT_ENUM, break_mode_hint), "set_move_right_normal_break_mode", "get_move_right_normal_break_mode");
	ClassDB::bind_method(D_METHOD("set_move_right_normal_jump_flags", "jump_flags"), &CaretWordBehavior::set_move_right_normal_jump_flags);
	ClassDB::bind_method(D_METHOD("get_move_right_normal_jump_flags"), &CaretWordBehavior::get_move_right_normal_jump_flags);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "move_right_normal_jump_flags", PROPERTY_HINT_FLAGS, jump_flag_hint), "set_move_right_normal_jump_flags", "get_move_right_normal_jump_flags");
	ADD_SUBGROUP("Newline Behavior", "move_right_newline_");
	ClassDB::bind_method(D_METHOD("set_move_right_newline_mode", "newline_mode"), &CaretWordBehavior::set_move_right_newline_mode);
	ClassDB::bind_method(D_METHOD("get_move_right_newline_mode"), &CaretWordBehavior::get_move_right_newline_mode);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "move_right_newline_mode", PROPERTY_HINT_ENUM, newline_mode_hint), "set_move_right_newline_mode", "get_move_right_newline_mode");
	ClassDB::bind_method(D_METHOD("set_move_right_newline_break_mode", "break_mode"), &CaretWordBehavior::set_move_right_newline_break_mode);
	ClassDB::bind_method(D_METHOD("get_move_right_newline_break_mode"), &CaretWordBehavior::get_move_right_newline_break_mode);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "move_right_newline_break_mode", PROPERTY_HINT_ENUM, break_mode_hint), "set_move_right_newline_break_mode", "get_move_right_newline_break_mode");
	ClassDB::bind_method(D_METHOD("set_move_right_newline_jump_flags", "jump_flags"), &CaretWordBehavior::set_move_right_newline_jump_flags);
	ClassDB::bind_method(D_METHOD("get_move_right_newline_jump_flags"), &CaretWordBehavior::get_move_right_newline_jump_flags);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "move_right_newline_jump_flags", PROPERTY_HINT_FLAGS, jump_flag_hint), "set_move_right_newline_jump_flags", "get_move_right_newline_jump_flags");

	ADD_GROUP("Caret Remove Left Behavior", "remove_left_");
	ADD_SUBGROUP("Normal Behavior", "remove_left_normal_");
	ClassDB::bind_method(D_METHOD("set_remove_left_normal_break_mode", "break_mode"), &CaretWordBehavior::set_remove_left_normal_break_mode);
	ClassDB::bind_method(D_METHOD("get_remove_left_normal_break_mode"), &CaretWordBehavior::get_remove_left_normal_break_mode);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "remove_left_normal_break_mode", PROPERTY_HINT_ENUM, break_mode_hint), "set_remove_left_normal_break_mode", "get_remove_left_normal_break_mode");
	ClassDB::bind_method(D_METHOD("set_remove_left_normal_jump_flags", "jump_flags"), &CaretWordBehavior::set_remove_left_normal_jump_flags);
	ClassDB::bind_method(D_METHOD("get_remove_left_normal_jump_flags"), &CaretWordBehavior::get_remove_left_normal_jump_flags);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "remove_left_normal_jump_flags", PROPERTY_HINT_FLAGS, jump_flag_hint), "set_remove_left_normal_jump_flags", "get_remove_left_normal_jump_flags");
	ADD_SUBGROUP("Newline Behavior", "remove_left_newline_");
	ClassDB::bind_method(D_METHOD("set_remove_left_newline_mode", "newline_mode"), &CaretWordBehavior::set_remove_left_newline_mode);
	ClassDB::bind_method(D_METHOD("get_remove_left_newline_mode"), &CaretWordBehavior::get_remove_left_newline_mode);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "remove_left_newline_mode", PROPERTY_HINT_ENUM, newline_mode_hint), "set_remove_left_newline_mode", "get_remove_left_newline_mode");
	ClassDB::bind_method(D_METHOD("set_remove_left_newline_break_mode", "break_mode"), &CaretWordBehavior::set_remove_left_newline_break_mode);
	ClassDB::bind_method(D_METHOD("get_remove_left_newline_break_mode"), &CaretWordBehavior::get_remove_left_newline_break_mode);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "remove_left_newline_break_mode", PROPERTY_HINT_ENUM, break_mode_hint), "set_remove_left_newline_break_mode", "get_remove_left_newline_break_mode");
	ClassDB::bind_method(D_METHOD("set_remove_left_newline_jump_flags", "jump_flags"), &CaretWordBehavior::set_remove_left_newline_jump_flags);
	ClassDB::bind_method(D_METHOD("get_remove_left_newline_jump_flags"), &CaretWordBehavior::get_remove_left_newline_jump_flags);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "remove_left_newline_jump_flags", PROPERTY_HINT_FLAGS, jump_flag_hint), "set_remove_left_newline_jump_flags", "get_remove_left_newline_jump_flags");

	ADD_GROUP("Caret Remove Right Behavior", "remove_right_");
	ADD_SUBGROUP("Normal Behavior", "remove_right_normal_");
	ClassDB::bind_method(D_METHOD("set_remove_right_normal_break_mode", "break_mode"), &CaretWordBehavior::set_remove_right_normal_break_mode);
	ClassDB::bind_method(D_METHOD("get_remove_right_normal_break_mode"), &CaretWordBehavior::get_remove_right_normal_break_mode);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "remove_right_normal_break_mode", PROPERTY_HINT_ENUM, break_mode_hint), "set_remove_right_normal_break_mode", "get_remove_right_normal_break_mode");
	ClassDB::bind_method(D_METHOD("set_remove_right_normal_jump_flags", "jump_flags"), &CaretWordBehavior::set_remove_right_normal_jump_flags);
	ClassDB::bind_method(D_METHOD("get_remove_right_normal_jump_flags"), &CaretWordBehavior::get_remove_right_normal_jump_flags);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "remove_right_normal_jump_flags", PROPERTY_HINT_FLAGS, jump_flag_hint), "set_remove_right_normal_jump_flags", "get_remove_right_normal_jump_flags");
	ADD_SUBGROUP("Newline Behavior", "remove_right_newline_");
	ClassDB::bind_method(D_METHOD("set_remove_right_newline_mode", "newline_mode"), &CaretWordBehavior::set_remove_right_newline_mode);
	ClassDB::bind_method(D_METHOD("get_remove_right_newline_mode"), &CaretWordBehavior::get_remove_right_newline_mode);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "remove_right_newline_mode", PROPERTY_HINT_ENUM, newline_mode_hint), "set_remove_right_newline_mode", "get_remove_right_newline_mode");
	ClassDB::bind_method(D_METHOD("set_remove_right_newline_break_mode", "break_mode"), &CaretWordBehavior::set_remove_right_newline_break_mode);
	ClassDB::bind_method(D_METHOD("get_remove_right_newline_break_mode"), &CaretWordBehavior::get_remove_right_newline_break_mode);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "remove_right_newline_break_mode", PROPERTY_HINT_ENUM, break_mode_hint), "set_remove_right_newline_break_mode", "get_remove_right_newline_break_mode");
	ClassDB::bind_method(D_METHOD("set_remove_right_newline_jump_flags", "jump_flags"), &CaretWordBehavior::set_remove_right_newline_jump_flags);
	ClassDB::bind_method(D_METHOD("get_remove_right_newline_jump_flags"), &CaretWordBehavior::get_remove_right_newline_jump_flags);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "remove_right_newline_jump_flags", PROPERTY_HINT_FLAGS, jump_flag_hint), "set_remove_right_newline_jump_flags", "get_remove_right_newline_jump_flags");
}

const CaretWordBehavior &CaretWordBehavior::get_fallback_caret_word_behavior() {
	// FIXME: Not 100% on this being the best way to do a fallback...
	static CaretWordBehavior fallback = CaretWordBehavior();
	return fallback;
}

int CaretWordBehavior::get_next_word_caret_left(RID p_text_shaped, int p_column, bool p_is_newline, bool p_is_remove) const {
	return _get_next_word_caret_directional(p_text_shaped, p_column, p_is_newline, p_is_remove ? remove_left : move_left, -1);
}
int CaretWordBehavior::get_next_word_caret_right(RID p_text_shaped, int p_column, bool p_is_newline, bool p_is_remove) const {
	return _get_next_word_caret_directional(p_text_shaped, p_column, p_is_newline, p_is_remove ? remove_right : move_right, 1);
}

void CaretWordBehavior::set_move_left_normal_break_mode(BreakMode p_break_mode) {
	move_left.normal_behavior.break_mode = p_break_mode;
}
CaretWordBehavior::BreakMode CaretWordBehavior::get_move_left_normal_break_mode() const {
	return move_left.normal_behavior.break_mode;
}
void CaretWordBehavior::set_move_left_normal_jump_flags(uint32_t p_jump_flags) {
	move_left.normal_behavior.jump_flags = p_jump_flags;
}
uint32_t CaretWordBehavior::get_move_left_normal_jump_flags() const {
	return move_left.normal_behavior.jump_flags;
}
void CaretWordBehavior::set_move_left_newline_mode(NewlineMode p_newline_mode) {
	move_left.newline_mode = p_newline_mode;
}
CaretWordBehavior::NewlineMode CaretWordBehavior::get_move_left_newline_mode() const {
	return move_left.newline_mode;
}
void CaretWordBehavior::set_move_left_newline_break_mode(BreakMode p_break_mode) {
	move_left.newline_behavior.break_mode = p_break_mode;
}
CaretWordBehavior::BreakMode CaretWordBehavior::get_move_left_newline_break_mode() const {
	return move_left.newline_behavior.break_mode;
}
void CaretWordBehavior::set_move_left_newline_jump_flags(uint32_t p_jump_flags) {
	move_left.newline_behavior.jump_flags = p_jump_flags;
}
uint32_t CaretWordBehavior::get_move_left_newline_jump_flags() const {
	return move_left.newline_behavior.jump_flags;
}

void CaretWordBehavior::set_move_right_normal_break_mode(BreakMode p_break_mode) {
	move_right.normal_behavior.break_mode = p_break_mode;
}
CaretWordBehavior::BreakMode CaretWordBehavior::get_move_right_normal_break_mode() const {
	return move_right.normal_behavior.break_mode;
}
void CaretWordBehavior::set_move_right_normal_jump_flags(uint32_t p_jump_flags) {
	move_right.normal_behavior.jump_flags = p_jump_flags;
}
uint32_t CaretWordBehavior::get_move_right_normal_jump_flags() const {
	return move_right.normal_behavior.jump_flags;
}
void CaretWordBehavior::set_move_right_newline_mode(NewlineMode p_newline_mode) {
	move_right.newline_mode = p_newline_mode;
}
CaretWordBehavior::NewlineMode CaretWordBehavior::get_move_right_newline_mode() const {
	return move_right.newline_mode;
}
void CaretWordBehavior::set_move_right_newline_break_mode(BreakMode p_break_mode) {
	move_right.newline_behavior.break_mode = p_break_mode;
}
CaretWordBehavior::BreakMode CaretWordBehavior::get_move_right_newline_break_mode() const {
	return move_right.newline_behavior.break_mode;
}
void CaretWordBehavior::set_move_right_newline_jump_flags(uint32_t p_jump_flags) {
	move_right.newline_behavior.jump_flags = p_jump_flags;
}
uint32_t CaretWordBehavior::get_move_right_newline_jump_flags() const {
	return move_right.newline_behavior.jump_flags;
}

void CaretWordBehavior::set_remove_left_normal_break_mode(BreakMode p_break_mode) {
	remove_left.normal_behavior.break_mode = p_break_mode;
}
CaretWordBehavior::BreakMode CaretWordBehavior::get_remove_left_normal_break_mode() const {
	return remove_left.normal_behavior.break_mode;
}
void CaretWordBehavior::set_remove_left_normal_jump_flags(uint32_t p_jump_flags) {
	remove_left.normal_behavior.jump_flags = p_jump_flags;
}
uint32_t CaretWordBehavior::get_remove_left_normal_jump_flags() const {
	return remove_left.normal_behavior.jump_flags;
}
void CaretWordBehavior::set_remove_left_newline_mode(NewlineMode p_newline_mode) {
	remove_left.newline_mode = p_newline_mode;
}
CaretWordBehavior::NewlineMode CaretWordBehavior::get_remove_left_newline_mode() const {
	return remove_left.newline_mode;
}
void CaretWordBehavior::set_remove_left_newline_break_mode(BreakMode p_break_mode) {
	remove_left.newline_behavior.break_mode = p_break_mode;
}
CaretWordBehavior::BreakMode CaretWordBehavior::get_remove_left_newline_break_mode() const {
	return remove_left.newline_behavior.break_mode;
}
void CaretWordBehavior::set_remove_left_newline_jump_flags(uint32_t p_jump_flags) {
	remove_left.newline_behavior.jump_flags = p_jump_flags;
}
uint32_t CaretWordBehavior::get_remove_left_newline_jump_flags() const {
	return remove_left.newline_behavior.jump_flags;
}

void CaretWordBehavior::set_remove_right_normal_break_mode(BreakMode p_break_mode) {
	remove_right.normal_behavior.break_mode = p_break_mode;
}
CaretWordBehavior::BreakMode CaretWordBehavior::get_remove_right_normal_break_mode() const {
	return remove_right.normal_behavior.break_mode;
}
void CaretWordBehavior::set_remove_right_normal_jump_flags(uint32_t p_jump_flags) {
	remove_right.normal_behavior.jump_flags = p_jump_flags;
}
uint32_t CaretWordBehavior::get_remove_right_normal_jump_flags() const {
	return remove_right.normal_behavior.jump_flags;
}
void CaretWordBehavior::set_remove_right_newline_mode(NewlineMode p_newline_mode) {
	remove_right.newline_mode = p_newline_mode;
}
CaretWordBehavior::NewlineMode CaretWordBehavior::get_remove_right_newline_mode() const {
	return remove_right.newline_mode;
}
void CaretWordBehavior::set_remove_right_newline_break_mode(BreakMode p_break_mode) {
	remove_right.newline_behavior.break_mode = p_break_mode;
}
CaretWordBehavior::BreakMode CaretWordBehavior::get_remove_right_newline_break_mode() const {
	return remove_right.newline_behavior.break_mode;
}
void CaretWordBehavior::set_remove_right_newline_jump_flags(uint32_t p_jump_flags) {
	remove_right.newline_behavior.jump_flags = p_jump_flags;
}
uint32_t CaretWordBehavior::get_remove_right_newline_jump_flags() const {
	return remove_right.newline_behavior.jump_flags;
}
