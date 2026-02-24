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
	enum Preset {
		PRESET_GODOT,
		PRESET_VSCODE,
		PRESET_VISUAL_STUDIO,
		PRESET_ATOM,
		PRESET_SUBLIME,
		PRESET_RIDER,
		PRESET_GEANY,
		PRESET_KATE,
		PRESET_CUSTOM,
	};

	enum BreakFlags {
		BREAK_FLAG_WORDS = 0b0,
		BREAK_FLAG_WORD_PASCAL = 0b1 << 0,
		BREAK_FLAG_WORD_SNAKE = 0b1 << 1,
		BREAK_FLAG_PUNCTUATION_AS_WORD = 0b1 << 2,
		BREAK_FLAG_SINGLE_PUNCTUATION = 0b1 << 3,
		BREAK_FLAG_SEPARATE_PUNCTUATION = 0b1 << 4,
		BREAK_FLAG_SINGLE_BRACKET = 0b1 << 5,
		BREAK_FLAG_SEPARATE_BRACKET = 0b1 << 6,
		BREAK_FLAG_DIRECTIONAL_BRACKETS = 0b1 << 7,
		BREAK_FLAG_STRINGS = 0b1 << 8,
	};

	enum JumpFlags {
		JUMP_FLAG_NONE = 0b0,
		JUMP_FLAG_SINGLE_WHITESPACE = 0b1 << 0,
		JUMP_FLAG_WHITESPACE = 0b1 << 1,
		JUMP_FLAG_TRAILING_WHITESPACE = 0b1 << 2,
		JUMP_FLAG_SINGLE_PUNCTUATION = 0b1 << 3,
		JUMP_FLAG_PUNCTUATION = 0b1 << 4,
	};

	enum NewlineMode {
		NEWLINE_MODE_NEVER,
		NEWLINE_MODE_ON_WHITESPACE,
		NEWLINE_MODE_ALWAYS,
	};

	struct LineBehavior {
		uint32_t break_flags = BREAK_FLAG_WORDS;
		uint32_t jump_flags = JUMP_FLAG_NONE;
	};

	struct ContextBehavior {
		LineBehavior normal_behavior = {};
		NewlineMode newline_mode = NEWLINE_MODE_NEVER;
		LineBehavior newline_behavior = {};
	};

	struct FullBehavior {
		ContextBehavior move_left;
		ContextBehavior move_right;
		ContextBehavior remove_left;
		ContextBehavior remove_right;
	};

private:
	static FullBehavior _get_preset_godot();
	static FullBehavior _get_preset_vscode();
	static FullBehavior _get_preset_visual_studio();
	static FullBehavior _get_preset_atom();
	static FullBehavior _get_preset_sublime();
	static FullBehavior _get_preset_rider();
	static FullBehavior _get_preset_geany();
	static FullBehavior _get_preset_kate();
	FullBehavior _get_preset(Preset p_preset) const;

	FullBehavior behavior = _get_preset_godot();

	bool is_bracket(char32_t p_char) const;
	bool is_directional_bracket(char32_t p_char, int p_direction) const;
	bool is_punct(char32_t p_char) const;

	enum NextCaretBehavior {
		NEXT_CARET_BEHAVIOR_BREAK,
		NEXT_CARET_BEHAVIOR_CONTINUE,
		NEXT_CARET_BEHAVIOR_INCREMENT,
	};
	NextCaretBehavior _get_next_word_caret_behavior(const String &p_line, int p_start_column, int p_next_column, const LineBehavior &p_behavior, bool p_at_start) const;
	PackedInt32Array _get_word_break_carets(RID p_text_shaped, const LineBehavior &p_behavior, int p_direction) const;
	int _get_next_word_caret_directional(RID p_text_shaped, int p_column, bool p_is_newline, const ContextBehavior &p_behavior, int p_direction) const;

protected:
	static void _bind_methods();

public:
	static Ref<CaretWordBehavior> get_fallback_caret_word_behavior();

	int get_next_word_caret_left(RID p_text_shaped, int p_column, bool p_is_newline, bool p_is_remove) const;
	int get_next_word_caret_right(RID p_text_shaped, int p_column, bool p_is_newline, bool p_is_remove) const;

	// Presets.
	void set_preset(Preset p_preset);
	Preset get_preset() const;
	void set_move_left_preset(Preset p_preset);
	Preset get_move_left_preset() const;
	void set_move_right_preset(Preset p_preset);
	Preset get_move_right_preset() const;
	void set_remove_left_preset(Preset p_preset);
	Preset get_remove_left_preset() const;
	void set_remove_right_preset(Preset p_preset);
	Preset get_remove_right_preset() const;

	// Move left.
	void set_move_left_normal_break_flags(uint32_t p_break_flags);
	uint32_t get_move_left_normal_break_flags() const;
	void set_move_left_normal_jump_flags(uint32_t p_jump_flags);
	uint32_t get_move_left_normal_jump_flags() const;
	void set_move_left_newline_mode(NewlineMode p_newline_mode);
	NewlineMode get_move_left_newline_mode() const;
	void set_move_left_newline_break_flags(uint32_t p_break_flags);
	uint32_t get_move_left_newline_break_flags() const;
	void set_move_left_newline_jump_flags(uint32_t p_jump_flags);
	uint32_t get_move_left_newline_jump_flags() const;

	// Move right.
	void set_move_right_normal_break_flags(uint32_t p_break_flags);
	uint32_t get_move_right_normal_break_flags() const;
	void set_move_right_normal_jump_flags(uint32_t p_jump_flags);
	uint32_t get_move_right_normal_jump_flags() const;
	void set_move_right_newline_mode(NewlineMode p_newline_mode);
	NewlineMode get_move_right_newline_mode() const;
	void set_move_right_newline_break_flags(uint32_t p_break_flags);
	uint32_t get_move_right_newline_break_flags() const;
	void set_move_right_newline_jump_flags(uint32_t p_jump_flags);
	uint32_t get_move_right_newline_jump_flags() const;

	// Remove left.
	void set_remove_left_normal_break_flags(uint32_t p_break_flags);
	uint32_t get_remove_left_normal_break_flags() const;
	void set_remove_left_normal_jump_flags(uint32_t p_jump_flags);
	uint32_t get_remove_left_normal_jump_flags() const;
	void set_remove_left_newline_mode(NewlineMode p_newline_mode);
	NewlineMode get_remove_left_newline_mode() const;
	void set_remove_left_newline_break_flags(uint32_t p_break_flags);
	uint32_t get_remove_left_newline_break_flags() const;
	void set_remove_left_newline_jump_flags(uint32_t p_jump_flags);
	uint32_t get_remove_left_newline_jump_flags() const;

	// Remove right.
	void set_remove_right_normal_break_flags(uint32_t p_break_flags);
	uint32_t get_remove_right_normal_break_flags() const;
	void set_remove_right_normal_jump_flags(uint32_t p_jump_flags);
	uint32_t get_remove_right_normal_jump_flags() const;
	void set_remove_right_newline_mode(NewlineMode p_newline_mode);
	NewlineMode get_remove_right_newline_mode() const;
	void set_remove_right_newline_break_flags(uint32_t p_break_flags);
	uint32_t get_remove_right_newline_break_flags() const;
	void set_remove_right_newline_jump_flags(uint32_t p_jump_flags);
	uint32_t get_remove_right_newline_jump_flags() const;
};
VARIANT_ENUM_CAST(CaretWordBehavior::Preset);
VARIANT_ENUM_CAST(CaretWordBehavior::BreakFlags);
VARIANT_ENUM_CAST(CaretWordBehavior::JumpFlags);
VARIANT_ENUM_CAST(CaretWordBehavior::NewlineMode);

const bool operator==(const CaretWordBehavior::ContextBehavior &a, const CaretWordBehavior::ContextBehavior &b);