/**************************************************************************/
/*  caret_word_behavior.h                                                 */
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

#pragma once

#include "core/io/resource.h"

class CaretWordBehavior : public Resource {
	GDCLASS(CaretWordBehavior, Resource)

public:
	enum BreakMode {
		BREAK_MODE_WORD,
		BREAK_MODE_WORD_AND_PUNCTUATION,
		BREAK_MODE_WORD_OR_PUNCTUATION,
	};

	enum JumpFlags {
		JUMP_FLAG_NONE = 0b0,
		JUMP_FLAG_SINGLE_WHITESPACE = 0b1 << 0,
		JUMP_FLAG_WHITESPACE = 0b1 << 1,
		JUMP_FLAG_SINGLE_PUNCTUATION = 0b1 << 2,
	};

	enum NewlineMode {
		NEWLINE_MODE_NEVER,
		NEWLINE_MODE_ON_WHITESPACE,
		NEWLINE_MODE_ALWAYS,
	};

	struct LineBehavior {
		BreakMode break_mode = BREAK_MODE_WORD;
		uint32_t jump_flags = JUMP_FLAG_NONE;
	};

	struct FullBehavior {
		LineBehavior normal_behavior = {};
		NewlineMode newline_mode = NEWLINE_MODE_NEVER;
		LineBehavior newline_behavior = {};
	};

private:
	FullBehavior move_left = {
		.normal_behavior = { .break_mode = BREAK_MODE_WORD, .jump_flags = JUMP_FLAG_WHITESPACE },
		.newline_mode = NEWLINE_MODE_NEVER,
	};
	FullBehavior move_right = {
		.normal_behavior = { .break_mode = BREAK_MODE_WORD, .jump_flags = JUMP_FLAG_WHITESPACE },
		.newline_mode = NEWLINE_MODE_NEVER,
	};
	FullBehavior remove_left = {
		.normal_behavior = { .break_mode = BREAK_MODE_WORD, .jump_flags = JUMP_FLAG_WHITESPACE },
		.newline_mode = NEWLINE_MODE_NEVER,
	};
	FullBehavior remove_right = {
		.normal_behavior = { .break_mode = BREAK_MODE_WORD, .jump_flags = JUMP_FLAG_WHITESPACE },
		.newline_mode = NEWLINE_MODE_NEVER,
	};

	enum NextCaretBehavior {
		NEXT_CARET_BEHAVIOR_BREAK,
		NEXT_CARET_BEHAVIOR_CONTINUE,
		NEXT_CARET_BEHAVIOR_INCREMENT,
	};
	NextCaretBehavior _get_next_word_caret_behavior(const String &p_line, int p_start_column, int p_next_column, const LineBehavior &p_behavior, bool p_at_start) const;
	PackedInt32Array _get_word_break_carets(RID p_text_shaped, BreakMode p_mode) const;
	int _get_next_word_caret_directional(RID p_text_shaped, int p_column, bool p_is_newline, const FullBehavior &p_behavior, int p_direction) const;

protected:
	static void _bind_methods();

public:
	static const CaretWordBehavior &get_fallback_caret_word_behavior();

	int get_next_word_caret_left(RID p_text_shaped, int p_column, bool p_is_newline, bool p_is_remove) const;
	int get_next_word_caret_right(RID p_text_shaped, int p_column, bool p_is_newline, bool p_is_remove) const;

	// Move left.
	void set_move_left_normal_break_mode(BreakMode p_break_mode);
	BreakMode get_move_left_normal_break_mode() const;
	void set_move_left_normal_jump_flags(uint32_t p_jump_flags);
	uint32_t get_move_left_normal_jump_flags() const;
	void set_move_left_newline_mode(NewlineMode p_newline_mode);
	NewlineMode get_move_left_newline_mode() const;
	void set_move_left_newline_break_mode(BreakMode p_break_mode);
	BreakMode get_move_left_newline_break_mode() const;
	void set_move_left_newline_jump_flags(uint32_t p_jump_flags);
	uint32_t get_move_left_newline_jump_flags() const;

	// Move right.
	void set_move_right_normal_break_mode(BreakMode p_break_mode);
	BreakMode get_move_right_normal_break_mode() const;
	void set_move_right_normal_jump_flags(uint32_t p_jump_flags);
	uint32_t get_move_right_normal_jump_flags() const;
	void set_move_right_newline_mode(NewlineMode p_newline_mode);
	NewlineMode get_move_right_newline_mode() const;
	void set_move_right_newline_break_mode(BreakMode p_break_mode);
	BreakMode get_move_right_newline_break_mode() const;
	void set_move_right_newline_jump_flags(uint32_t p_jump_flags);
	uint32_t get_move_right_newline_jump_flags() const;

	// Remove left.
	void set_remove_left_normal_break_mode(BreakMode p_break_mode);
	BreakMode get_remove_left_normal_break_mode() const;
	void set_remove_left_normal_jump_flags(uint32_t p_jump_flags);
	uint32_t get_remove_left_normal_jump_flags() const;
	void set_remove_left_newline_mode(NewlineMode p_newline_mode);
	NewlineMode get_remove_left_newline_mode() const;
	void set_remove_left_newline_break_mode(BreakMode p_break_mode);
	BreakMode get_remove_left_newline_break_mode() const;
	void set_remove_left_newline_jump_flags(uint32_t p_jump_flags);
	uint32_t get_remove_left_newline_jump_flags() const;

	// Remove right.
	void set_remove_right_normal_break_mode(BreakMode p_break_mode);
	BreakMode get_remove_right_normal_break_mode() const;
	void set_remove_right_normal_jump_flags(uint32_t p_jump_flags);
	uint32_t get_remove_right_normal_jump_flags() const;
	void set_remove_right_newline_mode(NewlineMode p_newline_mode);
	NewlineMode get_remove_right_newline_mode() const;
	void set_remove_right_newline_break_mode(BreakMode p_break_mode);
	BreakMode get_remove_right_newline_break_mode() const;
	void set_remove_right_newline_jump_flags(uint32_t p_jump_flags);
	uint32_t get_remove_right_newline_jump_flags() const;
};
VARIANT_ENUM_CAST(CaretWordBehavior::BreakMode);
VARIANT_ENUM_CAST(CaretWordBehavior::JumpFlags);
VARIANT_ENUM_CAST(CaretWordBehavior::NewlineMode);
